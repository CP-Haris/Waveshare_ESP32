import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  ActivityIndicator,
} from 'react-native';
import { MaterialIcons } from '@expo/vector-icons';
import { colors, spacing, fontSize, radius } from '../utils/theme';
import ScreenHeader from '../components/ScreenHeader';
import firmwareUpdateService from '../services/firmwareUpdateService';
import bleService from '../services/bleService';

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

  const [connected, setConnected] = useState(bleService.isConnected);
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
      else upToDate += 1;
    }

    return { updatable, upToDate };
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

  const detectTarget = useCallback(async () => {
    if (!bleService.isConnected) {
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
      const detected = await firmwareUpdateService.detectTarget({
        preferredCanId: canIdFromRoute ?? null,
        preferredPartNumber: partFromRoute || '',
        preferredSerialNumber: serialFromRoute || '',
      });

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
  }, [canIdFromRoute, partFromRoute, serialFromRoute]);

  useEffect(() => {
    const unsub = bleService.onConnectionChange((isConnected) => {
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

    if (bleService.isConnected) {
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

    const resolvedCanId = canIdFromRoute ?? target?.applicationCanId ?? null;
    const resolvedPartNumber = (partFromRoute || target?.partNumber || '').trim();
    const resolvedSerial = (serialFromRoute || target?.serialNumber || '').trim();

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
    <ScrollView style={styles.scroll} contentContainerStyle={styles.content}>
      <ScreenHeader />

      <View style={styles.topRow}>
        <View>
          <Text style={styles.title}>Firmware Update</Text>
          <Text style={styles.subtitle}>CAN bootloader over BLE gateway</Text>
        </View>
      </View>

      <View style={styles.card}>
        <View style={styles.sectionRow}>
          <Text style={styles.section}>Target</Text>
          <TouchableOpacity
            style={[styles.refreshBtn, (!connected || targetLoading || running) && styles.refreshBtnDisabled]}
            onPress={detectTarget}
            disabled={!connected || targetLoading || running}
          >
            {targetLoading ? (
              <ActivityIndicator size="small" color={colors.text} />
            ) : (
              <MaterialIcons name="refresh" size={16} color={colors.text} />
            )}
            <Text style={styles.refreshBtnText}>Rescan</Text>
          </TouchableOpacity>
        </View>

        <View style={styles.targetStatusRow}>
          <MaterialIcons
            name={connected ? 'bluetooth-connected' : 'bluetooth-disabled'}
            size={16}
            color={connected ? colors.green : colors.red}
          />
          <Text style={styles.targetStatusText}>{connected ? 'BLE connected' : 'BLE disconnected'}</Text>
        </View>

        {target ? (
          <>
            <Text style={styles.targetLine}>Part Number: {target.partNumber || '-'}</Text>
            <Text style={styles.targetLine}>Serial: {target.serialNumber || '-'}</Text>

            {planLoading ? (
              <Text style={styles.targetHint}>Loading firmware plan...</Text>
            ) : (
              <>
                {updatePlan.length > 0 ? (
                  <>
                    <View style={styles.targetPlanSummaryRow}>
                      <Text style={[styles.targetPlanSummary, { color: colors.accent }]}>Can update: {planSummary.updatable}</Text>
                      <Text style={styles.targetPlanSummarySep}>|</Text>
                      <Text style={[styles.targetPlanSummary, { color: colors.green }]}>Latest installed: {planSummary.upToDate}</Text>
                    </View>

                    {updatePlan.map((item) => {
                      const isUpdatable = item.status === 'updatable';
                      const moduleName = getModuleName(target.partNumber || partFromRoute, item.bridgeId);
                      const statusLabel = isUpdatable ? 'Can update' : 'Latest installed';
                      const statusColor = isUpdatable
                        ? colors.accent
                        : colors.green;
                      const statusIcon = isUpdatable
                        ? 'system-update'
                        : 'check-circle';

                      return (
                        <View key={`bridge-${item.bridgeId}`} style={styles.bridgeRow}>
                          <View style={styles.bridgeRowTop}>
                            <Text style={styles.bridgeTitle}>{moduleName}</Text>
                            <View style={styles.bridgeStatusWrap}>
                              <MaterialIcons name={statusIcon} size={14} color={statusColor} />
                              <Text style={[styles.bridgeStatusText, { color: statusColor }]}>{statusLabel}</Text>
                            </View>
                          </View>

                          <Text style={styles.bridgeLine}>Current version: v{item.currentVersionString}</Text>
                          <Text style={styles.bridgeLine}>
                            {isUpdatable
                              ? `Target version: v${item.targetVersionString || item.latestVersionString}`
                              : `Target version: v${item.targetVersionString || item.currentVersionString}`}
                          </Text>
                          <Text style={styles.bridgeHint}>
                            {isUpdatable
                              ? 'This module can be updated now.'
                              : 'Latest installed.'}
                          </Text>
                        </View>
                      );
                    })}
                  </>
                ) : (
                  <Text style={styles.targetHint}>No module firmware versions reported from target.</Text>
                )}

                {!planError && updatePlan.length > 0 ? (
                  <Text style={styles.targetHint}>
                    {updatesToRun.length > 0
                      ? `${updatesToRun.length} module update(s) ready`
                      : 'No newer released firmware available'}
                  </Text>
                ) : null}
              </>
            )}
          </>
        ) : (
          <Text style={styles.logEmpty}>{targetLoading ? 'Detecting target...' : 'No target detected yet'}</Text>
        )}

        {targetError ? <Text style={styles.targetError}>{targetError}</Text> : null}
        {planError ? <Text style={styles.targetError}>{planError}</Text> : null}

        <Text style={styles.targetHint}>Target and module updates are selected automatically at update start.</Text>
      </View>

      <View style={styles.card}>
        <Text style={styles.section}>Progress</Text>

        <View style={styles.progressWrap}>
          <View style={[styles.progressFill, { width: `${transferPercent}%` }]} />
        </View>
        <Text style={styles.progressText}>
          {running
            ? (transferTotal > 0
                ? `${transferPercent}%  •  Package ${transferCurrent}/${transferTotal}`
                : 'Preparing bootloader...')
            : (transferTotal > 0
                ? `${transferPercent}%  •  Package ${transferCurrent}/${transferTotal}`
                : 'Not started')}
        </Text>

        {result && (
          <View style={[styles.resultBox, result.ok ? styles.resultOk : styles.resultErr]}>
            <MaterialIcons name={result.ok ? 'check-circle' : 'error-outline'} size={18} color={result.ok ? colors.green : colors.red} />
            <Text style={[styles.resultText, { color: result.ok ? colors.green : colors.red }]}>{result.text}</Text>
          </View>
        )}

        <View style={styles.actions}>
          {!running ? (
            <TouchableOpacity
              style={[styles.primaryBtn, !canStart && styles.primaryBtnDisabled]}
              disabled={!canStart}
              onPress={runUpdate}
            >
              <MaterialIcons name="system-update" size={18} color={colors.text} />
              <Text style={styles.primaryBtnText}>
                {updatesToRun.length > 1 ? `Start Update (${updatesToRun.length} modules)` : 'Start Update'}
              </Text>
            </TouchableOpacity>
          ) : (
            <>
              <View style={styles.runningBox}>
                <ActivityIndicator size="small" color={colors.solar} />
                <Text style={styles.runningText}>Update running...</Text>
              </View>
              <TouchableOpacity style={styles.secondaryBtn} onPress={cancelUpdate}>
                <MaterialIcons name="stop" size={16} color={colors.red} />
                <Text style={styles.secondaryBtnText}>Cancel</Text>
              </TouchableOpacity>
            </>
          )}
        </View>
      </View>

      <View style={{ height: spacing.lg }} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  scroll: { flex: 1, backgroundColor: colors.bg },
  content: { paddingHorizontal: spacing.md, paddingTop: spacing.lg, paddingBottom: spacing.md },

  topRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginBottom: spacing.md,
  },
  title: { color: colors.text, fontSize: 30, fontWeight: '800' },
  subtitle: { color: colors.textMuted, fontSize: fontSize.sm, marginTop: 2 },

  card: {
    backgroundColor: colors.bgElevated,
    borderWidth: 1,
    borderColor: colors.border,
    borderRadius: radius.md,
    padding: spacing.md,
    marginBottom: spacing.sm,
  },

  section: {
    color: colors.text,
    fontSize: fontSize.md,
    fontWeight: '800',
    marginBottom: spacing.sm,
  },
  sectionRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginBottom: spacing.sm,
  },
  refreshBtn: {
    backgroundColor: colors.accent,
    borderRadius: radius.sm,
    paddingHorizontal: spacing.sm,
    paddingVertical: 6,
    flexDirection: 'row',
    alignItems: 'center',
    gap: 5,
  },
  refreshBtnDisabled: { opacity: 0.5 },
  refreshBtnText: { color: colors.text, fontWeight: '800', fontSize: fontSize.xs },

  targetStatusRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    marginBottom: spacing.sm,
  },
  targetStatusText: { color: colors.textMuted, fontSize: fontSize.sm, fontWeight: '700' },
  targetLine: { color: colors.textLight, fontSize: fontSize.sm, marginBottom: 4 },
  targetPlanSummary: {
    fontSize: fontSize.sm,
    fontWeight: '700',
  },
  targetPlanSummaryRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    marginBottom: spacing.sm,
  },
  targetPlanSummarySep: {
    color: colors.textMuted,
    fontSize: fontSize.sm,
    fontWeight: '700',
  },
  bridgeRow: {
    borderWidth: 1,
    borderColor: colors.borderSubtle,
    borderRadius: radius.sm,
    backgroundColor: colors.bgCard,
    padding: spacing.sm,
    marginBottom: spacing.sm,
  },
  bridgeRowTop: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginBottom: 4,
  },
  bridgeTitle: { color: colors.text, fontSize: fontSize.sm, fontWeight: '800' },
  bridgeStatusWrap: { flexDirection: 'row', alignItems: 'center', gap: 4 },
  bridgeStatusText: { fontSize: fontSize.xs, fontWeight: '800' },
  bridgeLine: { color: colors.textLight, fontSize: fontSize.sm, marginBottom: 2 },
  bridgeHint: { color: colors.textMuted, fontSize: fontSize.xs, marginTop: 2 },
  targetError: { color: colors.red, fontSize: fontSize.xs, marginTop: 6, fontWeight: '700' },
  targetHint: { color: colors.textFaint, fontSize: fontSize.xs, marginTop: 8 },

  progressWrap: {
    height: 10,
    borderRadius: 6,
    overflow: 'hidden',
    backgroundColor: colors.bgInset,
    borderWidth: 1,
    borderColor: colors.border,
  },
  progressFill: {
    height: 10,
    backgroundColor: colors.accent,
  },
  progressText: {
    color: colors.textLight,
    fontSize: fontSize.sm,
    marginTop: 6,
    fontWeight: '700',
  },

  resultBox: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    borderWidth: 1,
    borderRadius: radius.sm,
    padding: spacing.sm,
    marginTop: spacing.sm,
  },
  resultOk: { borderColor: colors.greenBorder, backgroundColor: colors.greenDeep },
  resultErr: { borderColor: colors.redBg, backgroundColor: colors.redDeep },
  resultText: { fontWeight: '700', fontSize: fontSize.sm },

  actions: { marginTop: spacing.sm, gap: spacing.sm },
  primaryBtn: {
    borderRadius: radius.md,
    backgroundColor: colors.accent,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 6,
    paddingVertical: spacing.sm,
  },
  primaryBtnDisabled: { opacity: 0.45 },
  primaryBtnText: { color: colors.text, fontWeight: '800' },

  runningBox: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 8,
    paddingVertical: spacing.sm,
  },
  runningText: { color: colors.solar, fontWeight: '700' },

  secondaryBtn: {
    borderWidth: 1,
    borderColor: colors.redBg,
    backgroundColor: colors.redDeep,
    borderRadius: radius.md,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 6,
    paddingVertical: spacing.sm,
  },
  secondaryBtnText: { color: colors.red, fontWeight: '800' },

  logEmpty: { color: colors.textFaint, fontSize: fontSize.sm },
});
