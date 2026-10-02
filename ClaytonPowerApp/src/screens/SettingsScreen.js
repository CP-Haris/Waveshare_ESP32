import React, { useState, useEffect, useCallback, useMemo, useRef } from 'react';
import {
  View,
  Text,
  ScrollView,
  StyleSheet,
  Pressable,
} from 'react-native';
import StatusBar from '../components/StatusBar';
import CarbonIcon from '../components/CarbonIcon';
import { openErrorList, worstColor } from '../components/ErrorCenter';
import { Button, Row, ScreenTitle, Section, Sheet } from '../components/Carbon';
import { colors, font, spacing, type } from '../utils/theme';
import { activeErrorDefinitions } from '../utils/errorCodes';
import { unitFamily } from '../utils/units';
import bleService from '../services/bleService';
import canGatewayService from '../services/canGatewayService';

const PREFIX = {
  VOLTAGE: 1,
  CURRENT: 2,
  TEMP: 3,
  PERCENT: 4,
  TIME_HHMMSS: 7,
  POWER: 9,
  ENUM: 20,
};

const ENUM_SOLAR_OP = ['Off', 'Auto', 'On'];
const ENUM_OP_VOLT = ['Auto', '12V', '24V'];
const ENUM_CONFIG = ['None', 'Extension'];

const LPS_CATEGORIES = [
  {
    key: 'acout', label: 'AC Output', icon: 'socket', settings: [
      { key: '50-0', label: 'Inverter Cutoff', block: 50, id: 0, prefix: PREFIX.PERCENT, decimals: 0, unit: '%', step: 655 },
      { key: '50-1', label: 'Auto Off Delay', block: 50, id: 1, prefix: PREFIX.TIME_HHMMSS, decimals: 0, unit: '', step: 1092 },
      { key: '50-2', label: 'Auto Off Load', block: 50, id: 2, prefix: PREFIX.POWER, decimals: 0, unit: 'W', step: 65536 },
    ],
  },
  {
    key: 'acin', label: 'AC Input', icon: 'plug', settings: [
      { key: '60-2', label: 'Max Current', block: 60, id: 2, prefix: PREFIX.CURRENT, decimals: 0, unit: 'A', step: 65536 },
    ],
  },
  {
    key: 'dcout', label: 'DC Output', icon: 'dc', settings: [
      { key: '40-0', label: 'Shutdown Delay', block: 40, id: 0, prefix: PREFIX.TIME_HHMMSS, decimals: 0, unit: '', step: 1092 },
      { key: '40-1', label: 'Saver Time', block: 40, id: 1, prefix: PREFIX.TIME_HHMMSS, decimals: 0, unit: '', step: 1092 },
      { key: '40-2', label: 'Saver Current', block: 40, id: 2, prefix: PREFIX.CURRENT, decimals: 0, unit: 'A', step: 65536 },
    ],
  },
  {
    key: 'dcin', label: 'DC Input', icon: 'car', settings: [
      { key: '30-1', label: 'Operating Voltage', block: 30, id: 1, prefix: PREFIX.ENUM, decimals: 0, unit: '', step: 65536, enumLabels: ENUM_OP_VOLT },
      { key: '30-7', label: 'Charge Current', block: 30, id: 7, prefix: PREFIX.CURRENT, decimals: 0, unit: 'A', step: 65536 },
      { key: '30-12', label: 'Start Voltage', block: 30, id: 12, prefix: PREFIX.VOLTAGE, decimals: 2, unit: 'V', step: 6553 },
      { key: '30-13', label: 'Stop Voltage', block: 30, id: 13, prefix: PREFIX.VOLTAGE, decimals: 2, unit: 'V', step: 6553 },
    ],
  },
  {
    key: 'solar', label: 'Solar', icon: 'sun', settings: [
      { key: '70-0', label: 'Operation', block: 70, id: 0, prefix: PREFIX.ENUM, decimals: 0, unit: '', step: 65536, enumLabels: ENUM_SOLAR_OP },
    ],
  },
  {
    key: 'general', label: 'General', icon: 'gear', settings: [
      { key: '1-1', label: 'Jumpstart Timer', block: 1, id: 1, prefix: PREFIX.TIME_HHMMSS, decimals: 0, unit: '', step: 5461 },
      { key: '7-0', label: 'Config Select', block: 7, id: 0, prefix: PREFIX.ENUM, decimals: 0, unit: '', step: 65536, enumLabels: ENUM_CONFIG },
    ],
  },
];

const BMS_CATEGORIES = [
  {
    key: 'battery', label: 'Battery', icon: 'batt', settings: [
      { key: '10-0', label: 'Battery Capacity', block: 10, id: 0, prefix: PREFIX.CURRENT, decimals: 0, unit: 'Ah', step: 65536 },
      { key: '10-1', label: 'DOD Capacity', block: 10, id: 1, prefix: PREFIX.PERCENT, decimals: 0, unit: '%', step: 655 },
    ],
  },
];

const REQUEST_SPACING_MS = 120;
const SETTING_VALUE_TIMEOUT_MS = 2500;
const SETTING_REFRESH_FRESH_MS = 6000;
const DETAIL_AUTO_REFRESH_MS = 8000;
const MAX_RETRIES = 2;

const UNIT_KIND = { LPS: 'LPS', BMS: 'BMS' };

function q16ToFloat(v) {
  return v / 65536;
}

function formatQ16Time(val) {
  const absVal = val < 0 ? -val : val;
  const totalSecsQ16 = absVal * 3600;
  let totalSecs = Math.floor(totalSecsQ16 / 65536);
  if ((totalSecsQ16 & 0xffff) > 0x8000) totalSecs += 1;
  const h = Math.floor(totalSecs / 3600);
  const m = Math.floor((totalSecs % 3600) / 60);
  if (totalSecs === 0) return 'OFF';
  if (h > 0) return `${h}h ${String(m).padStart(2, '0')}m`;
  return `${m} min`;
}

function formatSettingValue(def, rawValue) {
  if (rawValue == null) return '--';
  const fval = q16ToFloat(rawValue);
  switch (def.prefix) {
    case PREFIX.VOLTAGE:
      return `${fval.toFixed(def.decimals)} V`;
    case PREFIX.CURRENT:
      return `${fval.toFixed(def.decimals)} ${def.unit || 'A'}`;
    case PREFIX.POWER:
      return `${Math.round(fval)} W`;
    case PREFIX.PERCENT:
      return `${Math.round(fval * 100)} %`;
    case PREFIX.TIME_HHMMSS:
      return formatQ16Time(rawValue);
    case PREFIX.ENUM: {
      const idx = rawValue >> 16;
      if (def.enumLabels && idx >= 0 && idx < def.enumLabels.length) return def.enumLabels[idx];
      return String(idx);
    }
    default:
      return `${fval.toFixed(def.decimals || 0)} ${def.unit || ''}`.trim();
  }
}

function clampValue(v, min, max) {
  let out = v;
  if (min != null && out < min) out = min;
  if (max != null && out > max) out = max;
  return out;
}

function getFastStep(def) {
  if (!def?.step) return 65536;
  if (def.prefix === PREFIX.TIME_HHMMSS) return def.step * 5;
  if (def.prefix === PREFIX.VOLTAGE) return def.step * 10;
  return def.step * 10;
}

function normalizeUnitKind(type, partNumber) {
  return unitFamily({ type, partNumber }) === 'bms' ? UNIT_KIND.BMS : UNIT_KIND.LPS;
}

export default function SettingsScreen() {
  const [connected, setConnected] = useState(bleService.isConnected);
  const initialActiveUnit = canGatewayService.getActiveUnitInfo();
  const [activeUnitKind, setActiveUnitKind] = useState(() => normalizeUnitKind(initialActiveUnit?.type, initialActiveUnit?.partNumber));
  const [errors, setErrors] = useState([]);

  const [screen, setScreen] = useState('categories');
  const [selectedCategoryKey, setSelectedCategoryKey] = useState(null);

  const [settingValues, setSettingValues] = useState({});
  const [settingRanges, setSettingRanges] = useState({});
  const [loadingSettings, setLoadingSettings] = useState({});
  const [saveStatus, setSaveStatus] = useState({});

  const [editorVisible, setEditorVisible] = useState(false);
  const [editorDef, setEditorDef] = useState(null);
  const [editorDraft, setEditorDraft] = useState(0);
  const saveTimersRef = useRef({});
  const loadTimersRef = useRef({});
  const requestQueueRef = useRef([]);
  const requestPumpRef = useRef(null);
  const lastValueRequestRef = useRef({});
  const lastValueResponseRef = useRef({});
  const retryCountRef = useRef({});
  const activeUnitIndexRef = useRef(initialActiveUnit?.index ?? null);

  const categories = useMemo(
    () => (activeUnitKind === UNIT_KIND.BMS ? BMS_CATEGORIES : LPS_CATEGORIES),
    [activeUnitKind]
  );
  const selectedCategory = categories.find((c) => c.key === selectedCategoryKey) || null;

  const enqueueCommand = useCallback((sendRequest) => {
    if (!sendRequest) return;

    requestQueueRef.current.push(sendRequest);
    if (requestPumpRef.current) return;

    const pump = () => {
      const nextRequest = requestQueueRef.current.shift();
      if (!nextRequest) {
        requestPumpRef.current = null;
        return;
      }
      Promise.resolve(nextRequest())
        .catch((error) => console.warn('[Settings] CAN request failed:', error.message))
        .finally(() => {
          requestPumpRef.current = setTimeout(pump, REQUEST_SPACING_MS);
        });
    };

    requestPumpRef.current = setTimeout(pump, 0);
  }, []);

  const startValueLoadTimeout = useCallback((key, block, id) => {
    if (loadTimersRef.current[key]) clearTimeout(loadTimersRef.current[key]);
    loadTimersRef.current[key] = setTimeout(() => {
      delete loadTimersRef.current[key];
      const attempts = retryCountRef.current[key] || 0;
      if (attempts < MAX_RETRIES) {
        retryCountRef.current[key] = attempts + 1;
        lastValueRequestRef.current[key] = Date.now();
        enqueueCommand(() => canGatewayService.getSetting(block, id));
        startValueLoadTimeout(key, block, id);
      } else {
        setLoadingSettings((prev) => ({ ...prev, [key]: false }));
        retryCountRef.current[key] = 0;
      }
    }, SETTING_VALUE_TIMEOUT_MS);
  }, [enqueueCommand]);

  const clearPendingSettingActivity = useCallback(() => {
    if (requestPumpRef.current) {
      clearTimeout(requestPumpRef.current);
      requestPumpRef.current = null;
    }
    requestQueueRef.current = [];
    Object.values(loadTimersRef.current).forEach((timer) => clearTimeout(timer));
    Object.values(saveTimersRef.current).forEach((timer) => clearTimeout(timer));
    loadTimersRef.current = {};
    saveTimersRef.current = {};
    retryCountRef.current = {};
  }, []);

  const resetSettingSession = useCallback(() => {
    clearPendingSettingActivity();
    setSelectedCategoryKey(null);
    setScreen('categories');
    setEditorVisible(false);
    setEditorDef(null);
    setSettingValues({});
    setSettingRanges({});
    setLoadingSettings({});
    setSaveStatus({});
    lastValueRequestRef.current = {};
    lastValueResponseRef.current = {};
  }, [clearPendingSettingActivity]);

  const syncActiveUnit = useCallback(() => {
    const activeUnit = canGatewayService.getActiveUnitInfo();
    if (!activeUnit) {
      activeUnitIndexRef.current = null;
      setActiveUnitKind(UNIT_KIND.LPS);
      return;
    }

    const nextKind = normalizeUnitKind(activeUnit.type, activeUnit.partNumber);
    const previousIndex = activeUnitIndexRef.current;
    activeUnitIndexRef.current = activeUnit.index;
    setActiveUnitKind(nextKind);

    if (previousIndex !== activeUnit.index) resetSettingSession();
  }, [resetSettingSession]);

  const refreshErrors = useCallback(() => {
    canGatewayService.requestErrors();
  }, []);

  useEffect(() => {
    syncActiveUnit();

    const unsubConn = bleService.onConnectionChange((nextConnected) => {
      setConnected(nextConnected);
      if (!nextConnected) {
        activeUnitIndexRef.current = null;
        setErrors([]);
        resetSettingSession();
        return;
      }
      syncActiveUnit();
      canGatewayService.requestErrors();
    });

    const unsubNotif = canGatewayService.onNotification((msg) => {
      if (msg.type === 'unitInfo' || msg.type === 'dashboard') syncActiveUnit();

      if (msg.type === 'errors') setErrors(msg.data);

      if (msg.type === 'settingValue') {
        const key = `${msg.data.block}-${msg.data.id}`;
        lastValueResponseRef.current[key] = Date.now();
        retryCountRef.current[key] = 0;
        setSettingValues((prev) => ({ ...prev, [key]: msg.data.value }));
        setLoadingSettings((prev) => ({ ...prev, [key]: false }));
        setSaveStatus((prev) => {
          const next = { ...prev };
          if (next[key] === 'saving' || next[key] === 'queued') next[key] = 'saved';
          return next;
        });
        if (loadTimersRef.current[key]) {
          clearTimeout(loadTimersRef.current[key]);
          delete loadTimersRef.current[key];
        }
        if (saveTimersRef.current[key]) {
          clearTimeout(saveTimersRef.current[key]);
          delete saveTimersRef.current[key];
        }
      }

      if (msg.type === 'settingRange') {
        const key = `${msg.data.block}-${msg.data.id}`;
        setSettingRanges((prev) => ({ ...prev, [key]: { min: msg.data.min, max: msg.data.max } }));
        setLoadingSettings((prev) => ({ ...prev, [key]: false }));
      }
    });

    return () => {
      unsubConn();
      unsubNotif();
      clearPendingSettingActivity();
    };
  }, [clearPendingSettingActivity, resetSettingSession, syncActiveUnit]);

  useEffect(() => {
    if (connected) {
      syncActiveUnit();
      refreshErrors();
    }
  }, [connected, refreshErrors, syncActiveUnit]);

  const requestCategoryData = useCallback((category, options = {}) => {
    const { force = false } = options;
    const now = Date.now();

    category.settings.forEach((s) => {
      const key = s.key;
      const lastSeen = Math.max(
        lastValueResponseRef.current[key] || 0,
        lastValueRequestRef.current[key] || 0
      );
      if (!force && now - lastSeen < SETTING_REFRESH_FRESH_MS) return;

      setLoadingSettings((prev) => ({ ...prev, [s.key]: true }));
      retryCountRef.current[key] = 0;
      startValueLoadTimeout(key, s.block, s.id);
      lastValueRequestRef.current[key] = now;
      enqueueCommand(() => canGatewayService.getSetting(s.block, s.id));
    });
  }, [enqueueCommand, startValueLoadTimeout]);

  const openCategory = useCallback((category) => {
    clearPendingSettingActivity();
    setSelectedCategoryKey(category.key);
    setScreen('detail');
    requestCategoryData(category, { force: true });
  }, [clearPendingSettingActivity, requestCategoryData]);

  useEffect(() => {
    if (screen !== 'detail' || !selectedCategory) return;
    const timer = setInterval(() => {
      requestCategoryData(selectedCategory, { force: false });
    }, DETAIL_AUTO_REFRESH_MS);
    return () => clearInterval(timer);
  }, [screen, selectedCategory, requestCategoryData]);

  const openEditor = useCallback((def) => {
    if (!settingRanges[def.key]) {
      enqueueCommand(() => canGatewayService.getRange(def.block, def.id));
    }
    const current = settingValues[def.key];
    const range = settingRanges[def.key];
    setEditorDef(def);
    setEditorDraft(current ?? range?.min ?? 0);
    setEditorVisible(true);
  }, [settingValues, settingRanges, enqueueCommand]);

  const adjustDraft = useCallback((delta) => {
    if (!editorDef) return;
    const range = settingRanges[editorDef.key];
    // Functional update: the hold-to-repeat timer calls this from a stale closure.
    setEditorDraft((prev) => clampValue(prev + delta, range?.min, range?.max));
  }, [editorDef, settingRanges]);

  const setEnumDraft = useCallback((index) => {
    if (!editorDef) return;
    const raw = index << 16;
    const range = settingRanges[editorDef.key];
    setEditorDraft(clampValue(raw, range?.min, range?.max));
  }, [editorDef, settingRanges]);

  const saveEditor = useCallback(async () => {
    if (!editorDef) return;
    setSaveStatus((prev) => ({ ...prev, [editorDef.key]: 'saving' }));
    const ok = await canGatewayService.setSetting(editorDef.block, editorDef.id, editorDraft);
    if (!ok) {
      setSaveStatus((prev) => ({ ...prev, [editorDef.key]: 'error' }));
      return;
    }

    setSaveStatus((prev) => ({ ...prev, [editorDef.key]: 'queued' }));
    setLoadingSettings((prev) => ({ ...prev, [editorDef.key]: true }));
    retryCountRef.current[editorDef.key] = 0;
    startValueLoadTimeout(editorDef.key, editorDef.block, editorDef.id);
    lastValueRequestRef.current[editorDef.key] = Date.now();
    enqueueCommand(() => canGatewayService.getSetting(editorDef.block, editorDef.id));

    if (saveTimersRef.current[editorDef.key]) clearTimeout(saveTimersRef.current[editorDef.key]);
    saveTimersRef.current[editorDef.key] = setTimeout(() => {
      setLoadingSettings((prev) => ({ ...prev, [editorDef.key]: false }));
      setSaveStatus((prev) => ({
        ...prev,
        [editorDef.key]: prev[editorDef.key] === 'saved' ? 'saved' : 'timeout',
      }));
      delete saveTimersRef.current[editorDef.key];
    }, SETTING_VALUE_TIMEOUT_MS);

    setSettingValues((prev) => ({ ...prev, [editorDef.key]: editorDraft }));
    if (selectedCategory) {
      setTimeout(() => requestCategoryData(selectedCategory, { force: false }), 350);
    }
    setEditorVisible(false);
  }, [editorDef, editorDraft, requestCategoryData, selectedCategory, enqueueCommand, startValueLoadTimeout]);

  const editorStatus = editorDef ? saveStatus[editorDef.key] : null;

  const getStatusMeta = useCallback((key) => {
    if (loadingSettings[key]) return { text: 'LOADING', color: colors.dim };
    switch (saveStatus[key]) {
      case 'saving':
      case 'queued':
        return { text: 'SAVING', color: colors.blue };
      case 'saved':
        return { text: 'SAVED', color: colors.dim };
      case 'timeout':
        return { text: 'NO REPLY', color: colors.yellow };
      case 'error':
        return { text: 'FAILED', color: colors.red };
      default:
        return null;
    }
  }, [loadingSettings, saveStatus]);

  const editorEnumIndex = editorDef?.prefix === PREFIX.ENUM ? editorDraft >> 16 : null;
  const editorFastStep = editorDef ? getFastStep(editorDef) : 65536;
  const errorDefs = activeErrorDefinitions(errors);
  const range = editorDef ? settingRanges[editorDef.key] : null;
  const rangeFill = range && range.max > range.min
    ? Math.max(0, Math.min(1, (editorDraft - range.min) / (range.max - range.min)))
    : 0;

  if (!connected) {
    return (
      <View style={styles.screen}>
        <StatusBar />
        <ScreenTitle title="SETTINGS" />
        <View style={styles.center}>
          <CarbonIcon name="bt" size={48} color={colors.faint} />
          <Text style={[type.zone, styles.dim]}>NOT CONNECTED</Text>
        </View>
      </View>
    );
  }

  const inDetail = screen === 'detail' && selectedCategory;

  return (
    <View style={styles.screen}>
      <StatusBar />
      <ScreenTitle
        title={inDetail ? selectedCategory.label.toUpperCase() : 'SETTINGS'}
        onBack={inDetail ? () => { clearPendingSettingActivity(); setScreen('categories'); } : undefined}
      />

      <ScrollView contentContainerStyle={styles.content}>
        {!inDetail && (
          <Section>
            <Row
              kind="nav"
              icon="warn"
              iconColor={worstColor(errorDefs)}
              label="Errors"
              height={62}
              onPress={() => { refreshErrors(); openErrorList(); }}
              right={errorDefs.length > 0
                ? <Text style={[type.value, { color: worstColor(errorDefs) }]}>{errorDefs.length}</Text>
                : null}
            />
            {categories.map((cat, idx) => (
              <Row
                key={cat.key}
                kind="nav"
                icon={cat.icon}
                label={cat.label}
                height={62}
                last={idx === categories.length - 1}
                onPress={() => openCategory(cat)}
              />
            ))}
          </Section>
        )}

        {inDetail && (
          <Section title="SETTINGS">
            {selectedCategory.settings.map((s, idx) => {
              const statusMeta = getStatusMeta(s.key);
              return (
                <Row
                  key={s.key}
                  kind="setting"
                  label={s.label}
                  value={formatSettingValue(s, settingValues[s.key])}
                  tag={statusMeta?.text}
                  tagColor={statusMeta?.color}
                  last={idx === selectedCategory.settings.length - 1}
                  onPress={() => openEditor(s)}
                />
              );
            })}
          </Section>
        )}
      </ScrollView>

      <Sheet
        visible={editorVisible}
        onClose={() => setEditorVisible(false)}
        title={editorDef ? `${(selectedCategory?.label || '').toUpperCase()} · ${editorDef.label.toUpperCase()}` : ''}
      >
        {editorStatus && (
          <Text style={[type.micro, styles.editorStatus, { color: getStatusMeta(editorDef.key)?.color || colors.dim }]}>
            {editorStatus === 'saving' || editorStatus === 'queued'
              ? 'SAVING TO DEVICE…'
              : editorStatus === 'saved'
                ? 'SAVED'
                : editorStatus === 'error'
                  ? 'SAVE FAILED'
                  : 'DEVICE DID NOT CONFIRM YET'}
          </Text>
        )}

        {editorDef?.prefix === PREFIX.ENUM && editorDef.enumLabels ? (
          <View style={styles.enumList}>
            {editorDef.enumLabels.map((label, index) => {
              const selected = index === editorEnumIndex;
              return (
                <Pressable
                  key={`${editorDef.key}-${label}`}
                  style={({ pressed }) => [styles.enumRow, selected && styles.enumRowOn, pressed && styles.pressed]}
                  onPress={() => setEnumDraft(index)}
                >
                  <View style={[styles.radio, selected && styles.radioOn]}>
                    {selected && <View style={styles.radioDot} />}
                  </View>
                  <Text style={[type.label, !selected && styles.dim]}>{label}</Text>
                </Pressable>
              );
            })}
          </View>
        ) : (
          <>
            <View style={styles.editRow}>
              <StepButton icon="minus" onStep={(fast) => adjustDraft(-(fast ? editorFastStep : (editorDef?.step || 65536)))} />
              <EditorValue text={editorDef ? formatSettingValue(editorDef, editorDraft) : '--'} />
              <StepButton icon="plus" onStep={(fast) => adjustDraft(fast ? editorFastStep : (editorDef?.step || 65536))} />
            </View>
            <View style={styles.rangeTrack}>
              <View style={[styles.rangeFill, { width: `${rangeFill * 100}%` }]} />
            </View>
            <View style={styles.rangeLabels}>
              <Text style={type.small}>{editorDef ? formatSettingValue(editorDef, range?.min) : '--'}</Text>
              <Text style={type.small}>{editorDef ? formatSettingValue(editorDef, range?.max) : '--'}</Text>
            </View>
          </>
        )}

        <View style={styles.editorActions}>
          <Button label="CANCEL" onPress={() => setEditorVisible(false)} style={styles.flex} />
          <Button label="SAVE" variant="primary" onPress={saveEditor} style={styles.flex} />
        </View>
      </Sheet>
    </View>
  );
}

// Editor value as on the display: large number, small dim unit ("85 %").
// Non-numeric values (enum labels, "1h 05m", "OFF") stay in one piece.
function EditorValue({ text }) {
  const m = String(text).match(/^(-?\d+(?:\.\d+)?)\s+(\S+)$/);
  return (
    <Text style={styles.editValue} numberOfLines={1} adjustsFontSizeToFit>
      {m ? m[1] : text}
      {m && <Text style={styles.editUnit}> {m[2]}</Text>}
    </Text>
  );
}

// Round −/+ (spec §8): tap = fine step; hold = auto-repeat, switching to the
// coarse step after five repeats.
function StepButton({ icon, onStep }) {
  const timerRef = useRef(null);
  const countRef = useRef(0);

  const stop = () => {
    clearTimeout(timerRef.current);
    timerRef.current = null;
  };

  const tick = () => {
    countRef.current += 1;
    onStep(countRef.current > 5);
    timerRef.current = setTimeout(tick, 110);
  };

  useEffect(() => stop, []);

  return (
    <Pressable
      onPressIn={() => {
        countRef.current = 0;
        onStep(false);
        timerRef.current = setTimeout(tick, 450);
      }}
      onPressOut={stop}
      style={({ pressed }) => [styles.stepBtn, pressed && styles.pressed]}
    >
      <CarbonIcon name={icon} size={36} strokeWidth={2} />
    </Pressable>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: colors.bg },
  content: { paddingBottom: spacing.lg },
  center: { flex: 1, alignItems: 'center', justifyContent: 'center', gap: spacing.md },
  dim: { color: colors.dim },
  flex: { flex: 1 },
  pressed: { backgroundColor: colors.panelPressed },

  editorStatus: { marginTop: 2 },
  editRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', gap: spacing.md, marginTop: spacing.md },
  editValue: { flex: 1, textAlign: 'center', fontFamily: font.bold, fontSize: 56, color: colors.ink, fontVariant: ['tabular-nums'] },
  editUnit: { fontFamily: font.semibold, fontSize: 24, color: colors.dim },
  stepBtn: {
    width: 88,
    height: 88,
    borderRadius: 44,
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: colors.panel,
    borderWidth: 1,
    borderColor: colors.edge,
  },
  rangeTrack: { height: 6, backgroundColor: colors.track, marginTop: spacing.lg },
  rangeFill: { height: 6, backgroundColor: colors.blue },
  rangeLabels: { flexDirection: 'row', justifyContent: 'space-between', marginTop: spacing.sm },

  enumList: { marginTop: spacing.sm },
  enumRow: {
    minHeight: 56,
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.md,
    paddingHorizontal: spacing.sm,
    borderBottomWidth: 1,
    borderBottomColor: colors.rowLine,
  },
  enumRowOn: { backgroundColor: colors.panel },
  radio: { width: 22, height: 22, borderRadius: 11, borderWidth: 2, borderColor: colors.dim, alignItems: 'center', justifyContent: 'center' },
  radioOn: { borderColor: colors.blue },
  radioDot: { width: 10, height: 10, borderRadius: 5, backgroundColor: colors.blue },

  editorActions: { flexDirection: 'row', gap: spacing.md, marginTop: spacing.lg },
});
