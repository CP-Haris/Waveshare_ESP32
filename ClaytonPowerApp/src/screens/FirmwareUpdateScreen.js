import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
} from 'react-native';
import StatusBar from '../components/StatusBar';
import { Button, Notice, Row, ScreenTitle, Section } from '../components/Carbon';
import { colors, font, spacing, type } from '../utils/theme';
import firmwareUpdateService from '../devices/display/firmwareUpdateService';
import deviceSession from '../devices/deviceSession';

const DEFAULT_API_BASE = 'http://49.12.206.181/firmware-api';
const DEFAULT_API_KEY = 'ff871ffebf04c37e60bafbc9dfcca0fdaec9d82b20d0febf351bf0819b457f10';

const MODULE_NAMES_BY_PART_PREFIX = {
  CL: {
    1: 'Control',
    2: 'Power',
    3: 'Display',
    4: 'DC/DC',
  },
  CB: {
    1: 'Control',
  },
};

function getModuleName(partNumber, bridgeId) {
  const prefix = String(partNumber || '').trim().toUpperCase().slice(0, 2);
  return MODULE_NAMES_BY_PART_PREFIX[prefix]?.[Number(bridgeId)] || `Bridge ${bridgeId}`;
}

export default function FirmwareUpdateScreen({ route }) {
  const serialFromRoute = route?.params?.serial || '';
  const partFromRoute = route?.params?.partNumber || '';
  const canIdFromRoute = route?.params?.canId;

  const [connected, setConnected] = useState(deviceSession.isConnected);
  const [target, setTarget] = useState(null);
  const [targetLoading, setTargetLoading] = useState(false);
  const [targetError, setTargetError] = useState('');
  const [updatePlan, setUpdatePlan] = useState([]);
  const [planLoading, setPlanLoading] = useState(false);
  const [planError, setPlanError] = useState('');

  const [running, setRunning] = useState(false);
  const [transferCurrent, setTransferCurrent] = useState(0);
  const [transferTotal, setTransferTotal] = useState(0);
  const [result, setResult] = useState(null);
  const abortRef = useRef(null);
  const selectedUnitIndexRef = useRef(deviceSession.getActiveUnitInfo()?.index ?? null);

  const transferPercent = useMemo(() => {
    if (transferTotal <= 0) return 0;
    return Math.max(0, Math.min(100, Math.round((transferCurrent / transferTotal) * 100)));
  }, [transferCurrent, transferTotal]);

  const updatesToRun = useMemo(
    () => updatePlan.filter((item) => item.updateAvailable && item.latestVersionString),
    [updatePlan],
  );

  const planSummary = useMemo(() => {
    let updatable = 0;
    let upToDate = 0;

    for (const item of updatePlan) {
      if (item.status === 'updatable') updatable += 1;
      else if (item.status === 'unavailable') continue;
      else upToDate += 1;
    }

    const unavailable = updatePlan.filter((item) => item.status === 'unavailable').length;
    return { updatable, upToDate, unavailable };
  }, [updatePlan]);

  const canStart = useMemo(() => {
    return !running && connected && !targetLoading && !planLoading && !!target && updatesToRun.length > 0;
  }, [running, connected, targetLoading, planLoading, target, updatesToRun]);

  const updateProgress = useCallback((event) => {
    const message = typeof event === 'string' ? event : event?.message;
    const blockMatch = String(message || '').match(/^Uploading block\s+(\d+)\/(\d+)$/i);
    if (blockMatch) {
      setTransferCurrent(Number(blockMatch[1]) || 0);
      setTransferTotal(Number(blockMatch[2]) || 0);
    }
  }, []);

  const getTargetPreferences = useCallback(() => {
    const activeUnit = deviceSession.getActiveUnitInfo();
    const activeCanId = Number.isFinite(activeUnit?.addr) ? activeUnit.addr : null;
    return {
      preferredCanId: canIdFromRoute ?? activeCanId ?? null,
      preferredPartNumber: partFromRoute || '',
      preferredSerialNumber: serialFromRoute || activeUnit?.serial || '',
    };
  }, [canIdFromRoute, partFromRoute, serialFromRoute]);

  const detectTarget = useCallback(async () => {
    if (!deviceSession.isConnected) {
      setTarget(null);
      setUpdatePlan([]);
      setPlanError('');
      setTargetError('BLE not connected');
      return;
    }

    setTargetLoading(true);
    setTargetError('');
    setPlanError('');

    try {
      const detected = await firmwareUpdateService.detectTarget(getTargetPreferences());

      setTarget(detected);

      setPlanLoading(true);
      try {
        const plan = await firmwareUpdateService.getTargetUpdatePlan({
          apiBaseUrl: DEFAULT_API_BASE,
          apiKey: DEFAULT_API_KEY,
          partNumber: detected.partNumber || '',
          bridgeFirmwareVersions: detected.bridgeFirmwareVersions || {},
        });

        setUpdatePlan(plan);
      } catch (planErr) {
        setUpdatePlan([]);
        setPlanError(planErr?.message || 'Failed to load firmware plan');
      } finally {
        setPlanLoading(false);
      }
    } catch (e) {
      setTarget(null);
      setUpdatePlan([]);
      setPlanError('');
      setTargetError(e?.message || 'Failed to auto-detect target');
    } finally {
      setTargetLoading(false);
    }
  }, [getTargetPreferences]);

  useEffect(() => {
    const syncSelectedUnit = () => {
      const nextIndex = deviceSession.getActiveUnitInfo()?.index ?? null;
      if (selectedUnitIndexRef.current === nextIndex) return;

      selectedUnitIndexRef.current = nextIndex;
      setTarget(null);
      setUpdatePlan([]);
      setPlanError('');

      if (!deviceSession.isConnected) return;
      if (nextIndex == null) {
        setTargetError('Select a unit in the header before scanning');
        return;
      }
      if (!running && !targetLoading) detectTarget();
    };

    const unsub = deviceSession.onNotification((message) => {
      if (message.type === 'unitInfo' || message.type === 'dashboard') syncSelectedUnit();
    });

    syncSelectedUnit();
    return unsub;
  }, [detectTarget, running, targetLoading]);

  useEffect(() => {
    const unsub = deviceSession.onConnectionChange((isConnected) => {
      setConnected(isConnected);
      if (!isConnected) {
        setTarget(null);
        setUpdatePlan([]);
        setPlanError('');
        setTargetError('BLE not connected');
      } else {
        detectTarget();
      }
    });

    if (deviceSession.isConnected) {
      detectTarget();
    } else {
      setTargetError('BLE not connected');
    }

    return unsub;
  }, [detectTarget]);

  const runUpdate = async () => {
    if (!canStart) return;
    if (!updatesToRun.length) {
      setResult({ ok: false, text: 'No newer released firmware available for this target' });
      return;
    }

    setRunning(true);
    setTransferCurrent(0);
    setTransferTotal(0);
    setResult(null);

    const abortController = new AbortController();
    abortRef.current = abortController;

    const targetPreferences = getTargetPreferences();
    const resolvedCanId = canIdFromRoute ?? target?.applicationCanId ?? targetPreferences.preferredCanId ?? null;
    const resolvedPartNumber = (partFromRoute || target?.partNumber || '').trim();
    const resolvedSerial = (serialFromRoute || target?.serialNumber || targetPreferences.preferredSerialNumber || '').trim();

    try {
      let successCount = 0;
      let failCount = 0;
      let lastErrorMessage = '';

      for (const update of updatesToRun) {
        setTransferCurrent(0);
        setTransferTotal(0);

        try {
          await firmwareUpdateService.runFirmwareUpdate({
            applicationCanId: resolvedCanId,
            serialNumber: resolvedSerial,
            partNumber: resolvedPartNumber,
            bridgeId: update.bridgeId,
            apiBaseUrl: DEFAULT_API_BASE,
            apiKey: DEFAULT_API_KEY,
            versionString: update.latestVersionString,
            onProgress: updateProgress,
            signal: abortController.signal,
          });

          successCount += 1;
        } catch (err) {
          const message = err?.message || 'Firmware update failed';
          if (/cancelled/i.test(message)) throw err;
          failCount += 1;
          lastErrorMessage = message;
        }
      }

      if (transferTotal > 0) {
        setTransferCurrent(transferTotal);
      }

      if (failCount === 0) {
        const moduleName = getModuleName(resolvedPartNumber, updatesToRun[0]?.bridgeId);
        setResult({
          ok: true,
          text:
            updatesToRun.length === 1
              ? `${moduleName} updated to ${updatesToRun[0].latestVersionString}`
              : `${successCount} module updates completed`,
        });
      } else {
        setResult({
          ok: false,
          text: lastErrorMessage && failCount === 1
            ? lastErrorMessage
            : `${successCount} module updated, ${failCount} failed`,
        });
      }

      await detectTarget();
    } catch (e) {
      setResult({ ok: false, text: e?.message || 'Firmware update failed' });
    } finally {
      setRunning(false);
      abortRef.current = null;
    }
  };

  const cancelUpdate = () => {
    abortRef.current?.abort();
  };


  return (
    <View style={styles.screen}>
      <StatusBar />
      <ScreenTitle
        title="UPDATE"
        right={(
          <Button
            compact
            label="RESCAN"
            icon="refresh"
            loading={targetLoading}
            disabled={!connected || running}
            onPress={detectTarget}
          />
        )}
      />

      <ScrollView contentContainerStyle={styles.content}>
        {!target && (
          <Section>
            <Text style={[type.small, styles.hint]}>
              {targetLoading ? 'Detecting unit…' : connected ? 'No unit detected yet' : 'Not connected'}
            </Text>
            {!!targetError && connected && <Notice text={targetError} />}
          </Section>
        )}

        {target && (
          <Section title="MODULES">
            {planLoading ? (
              <Text style={[type.small, styles.hint]}>Loading firmware plan…</Text>
            ) : updatePlan.length > 0 ? (
              updatePlan.map((item, idx) => {
                const isUpdatable = item.status === 'updatable';
                const isUnavailable = item.status === 'unavailable';
                const moduleName = getModuleName(target.partNumber || partFromRoute, item.bridgeId);
                const versions = isUnavailable
                  ? 'No response'
                  : isUpdatable
                    ? `v${item.currentVersionString}  →  v${item.targetVersionString || item.latestVersionString}`
                    : `v${item.currentVersionString}`;
                return (
                  <Row
                    key={`bridge-${item.bridgeId}`}
                    kind="item"
                    icon={isUpdatable ? 'update' : isUnavailable ? 'minus' : 'check'}
                    iconColor={isUpdatable ? colors.blue : isUnavailable ? colors.dim : colors.ink}
                    label={moduleName}
                    tag={versions}
                    tagColor={isUpdatable ? colors.blue : colors.dim}
                    last={idx === updatePlan.length - 1}
                  />
                );
              })
            ) : (
              <Text style={[type.small, styles.hint]}>No module firmware versions reported.</Text>
            )}
            {!!planError && <Notice text={planError} />}
          </Section>
        )}

        <Section title="PROGRESS">
          <View style={styles.progressHead}>
            <View style={styles.pctRow}>
              <Text style={type.total}>{transferPercent}</Text>
              <Text style={styles.pctUnit}>%</Text>
            </View>
            <Text style={type.small}>
              {running && transferTotal === 0
                ? 'PREPARING BOOTLOADER'
                : transferTotal > 0
                  ? `PACKAGE ${transferCurrent}/${transferTotal}`
                  : updatesToRun.length > 0
                    ? `${updatesToRun.length} MODULE${updatesToRun.length > 1 ? 'S' : ''} READY`
                    : 'UP TO DATE'}
            </Text>
          </View>
          <View style={styles.track}>
            <View style={[styles.fill, { width: `${transferPercent}%` }]} />
          </View>

          {result && <Notice text={result.text} color={result.ok ? colors.blue : colors.red} />}

          <View style={styles.actions}>
            {!running ? (
              <Button
                variant="primary"
                icon="update"
                label={updatesToRun.length > 1 ? `START UPDATE · ${updatesToRun.length}` : 'START UPDATE'}
                disabled={!canStart}
                onPress={runUpdate}
              />
            ) : (
              <Button icon="stop" label="CANCEL" onPress={cancelUpdate} />
            )}
          </View>
        </Section>
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: colors.bg },
  content: { paddingBottom: spacing.lg },
  hint: { paddingVertical: spacing.md },

  progressHead: { flexDirection: 'row', alignItems: 'flex-end', justifyContent: 'space-between', marginTop: spacing.xs },
  pctRow: { flexDirection: 'row', alignItems: 'flex-start', gap: 4 },
  pctUnit: { fontFamily: font.semibold, fontSize: 18, color: colors.dim, marginTop: 4 },
  track: { height: 6, backgroundColor: colors.track, marginTop: spacing.sm },
  fill: { height: 6, backgroundColor: colors.blue },

  actions: { marginTop: spacing.md },
});
