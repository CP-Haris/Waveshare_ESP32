import React, { useState, useEffect, useCallback, useMemo, useRef } from 'react';
import {
  View,
  Text,
  ScrollView,
  StyleSheet,
  TouchableOpacity,
  Modal,
  ActivityIndicator,
} from 'react-native';
import { MaterialIcons } from '@expo/vector-icons';
import { colors, spacing, fontSize, radius } from '../utils/theme';
import bleService from '../services/bleService';
import canGatewayService from '../services/canGatewayService';
import ScreenHeader from '../components/ScreenHeader';

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
    key: 'acout', label: 'AC Output', icon: 'power', settings: [
      { key: '50-0', label: 'Inverter Cutoff', block: 50, id: 0, prefix: PREFIX.PERCENT, decimals: 0, unit: '%', step: 655 },
      { key: '50-1', label: 'Auto Off Delay', block: 50, id: 1, prefix: PREFIX.TIME_HHMMSS, decimals: 0, unit: '', step: 1092 },
      { key: '50-2', label: 'Auto Off Load', block: 50, id: 2, prefix: PREFIX.POWER, decimals: 0, unit: 'W', step: 65536 },
    ],
  },
  {
    key: 'acin', label: 'AC Input', icon: 'input', settings: [
      { key: '60-2', label: 'Max Current', block: 60, id: 2, prefix: PREFIX.CURRENT, decimals: 0, unit: 'A', step: 65536 },
    ],
  },
  {
    key: 'dcout', label: 'DC Output', icon: 'electrical-services', settings: [
      { key: '40-0', label: 'Shutdown Delay', block: 40, id: 0, prefix: PREFIX.TIME_HHMMSS, decimals: 0, unit: '', step: 1092 },
      { key: '40-1', label: 'Saver Time', block: 40, id: 1, prefix: PREFIX.TIME_HHMMSS, decimals: 0, unit: '', step: 1092 },
      { key: '40-2', label: 'Saver Current', block: 40, id: 2, prefix: PREFIX.CURRENT, decimals: 0, unit: 'A', step: 65536 },
    ],
  },
  {
    key: 'dcin', label: 'DC Input', icon: 'ev-station', settings: [
      { key: '30-1', label: 'Operating Voltage', block: 30, id: 1, prefix: PREFIX.ENUM, decimals: 0, unit: '', step: 65536, enumLabels: ENUM_OP_VOLT },
      { key: '30-7', label: 'Charge Current', block: 30, id: 7, prefix: PREFIX.CURRENT, decimals: 0, unit: 'A', step: 65536 },
      { key: '30-12', label: 'Start Voltage', block: 30, id: 12, prefix: PREFIX.VOLTAGE, decimals: 2, unit: 'V', step: 6553 },
      { key: '30-13', label: 'Stop Voltage', block: 30, id: 13, prefix: PREFIX.VOLTAGE, decimals: 2, unit: 'V', step: 6553 },
    ],
  },
  {
    key: 'solar', label: 'Solar', icon: 'wb-sunny', settings: [
      { key: '70-0', label: 'Operation', block: 70, id: 0, prefix: PREFIX.ENUM, decimals: 0, unit: '', step: 65536, enumLabels: ENUM_SOLAR_OP },
    ],
  },
  {
    key: 'general', label: 'General', icon: 'tune', settings: [
      { key: '1-1', label: 'Jumpstart Timer', block: 1, id: 1, prefix: PREFIX.TIME_HHMMSS, decimals: 0, unit: '', step: 5461 },
      { key: '7-0', label: 'Config Select', block: 7, id: 0, prefix: PREFIX.ENUM, decimals: 0, unit: '', step: 65536, enumLabels: ENUM_CONFIG },
    ],
  },
];

const BMS_CATEGORIES = [
  {
    key: 'battery', label: 'Battery', icon: 'battery-full', settings: [
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
  const pn = (partNumber || '').toUpperCase();
  if (pn.startsWith('CL2')) return UNIT_KIND.LPS;
  if (pn.startsWith('CB2')) return UNIT_KIND.BMS;
  if (type === 2) return UNIT_KIND.BMS;
  if (type === 1) return UNIT_KIND.LPS;
  return UNIT_KIND.LPS;
}

export default function SettingsScreen() {
  const [connected, setConnected] = useState(bleService.isConnected);
  const [units, setUnits] = useState([]);
  const [activeUnitIndex, setActiveUnitIndex] = useState(0);
  const [activeUnitKind, setActiveUnitKind] = useState(UNIT_KIND.LPS);
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

  const categories = useMemo(
    () => (activeUnitKind === UNIT_KIND.BMS ? BMS_CATEGORIES : LPS_CATEGORIES),
    [activeUnitKind]
  );
  const selectedCategory = categories.find((c) => c.key === selectedCategoryKey) || null;

  useEffect(() => {
    const unsubConn = bleService.onConnectionChange(setConnected);
    const unsubNotif = canGatewayService.onNotification((msg) => {
      if (msg.type === 'unitInfo') {
        const normalized = {
          ...msg.data,
          kind: normalizeUnitKind(msg.data.type, msg.data.partNumber),
        };
        setUnits((prev) => {
          const existing = prev.findIndex((u) => u.index === normalized.index);
          if (existing >= 0) {
            const next = [...prev];
            next[existing] = normalized;
            return next;
          }
          return [...prev, normalized];
        });
      }

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
      if (requestPumpRef.current) {
        clearTimeout(requestPumpRef.current);
        requestPumpRef.current = null;
      }
      requestQueueRef.current = [];
      Object.values(loadTimersRef.current).forEach((timer) => clearTimeout(timer));
      Object.values(saveTimersRef.current).forEach((timer) => clearTimeout(timer));
    };
  }, []);

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

  const refreshUnits = useCallback(() => {
    setUnits([]);
    canGatewayService.requestUnits();
  }, []);

  const refreshErrors = useCallback(() => {
    canGatewayService.requestErrors();
  }, []);

  useEffect(() => {
    if (connected) {
      refreshUnits();
      refreshErrors();
    }
  }, [connected, refreshUnits, refreshErrors]);

  useEffect(() => {
    if (units.length === 0) return;
    const active = units.find((u) => u.index === activeUnitIndex) || units[0];
    if (active.index !== activeUnitIndex) {
      setActiveUnitIndex(active.index);
      canGatewayService.selectUnit(active.index);
    }
    setActiveUnitKind(active.kind || normalizeUnitKind(active.type, active.partNumber));
  }, [units, activeUnitIndex]);

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

  const selectUnit = useCallback((index, kind) => {
    clearPendingSettingActivity();
    setActiveUnitIndex(index);
    setActiveUnitKind(kind || UNIT_KIND.LPS);
    setSelectedCategoryKey(null);
    setScreen('categories');
    setEditorVisible(false);
    setSettingValues({});
    setSettingRanges({});
    setLoadingSettings({});
    setSaveStatus({});
    lastValueRequestRef.current = {};
    lastValueResponseRef.current = {};
    canGatewayService.selectUnit(index);
  }, [clearPendingSettingActivity]);

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
    setEditorDraft(clampValue(editorDraft + delta, range?.min, range?.max));
  }, [editorDef, editorDraft, settingRanges]);

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
    if (loadingSettings[key]) return { text: 'Loading', color: colors.textMuted, icon: 'hourglass-top' };
    switch (saveStatus[key]) {
      case 'saving':
      case 'queued':
        return { text: 'Saving', color: colors.solar, icon: 'sync' };
      case 'saved':
        return { text: 'Saved', color: colors.green, icon: 'check-circle' };
      case 'timeout':
        return { text: 'No reply', color: colors.solar, icon: 'schedule' };
      case 'error':
        return { text: 'Failed', color: colors.red, icon: 'error-outline' };
      default:
        return null;
    }
  }, [loadingSettings, saveStatus]);

  const deviceTypeLabel = (unit) => unit?.kind || normalizeUnitKind(unit?.type, unit?.partNumber);
  const editorEnumIndex = editorDef?.prefix === PREFIX.ENUM ? editorDraft >> 16 : null;
  const editorFastStep = editorDef ? getFastStep(editorDef) : 65536;

  if (!connected) {
    return (
      <View style={styles.center}>
        <MaterialIcons name="bluetooth-disabled" size={40} color={colors.textGhost} />
        <Text style={styles.centerHint}>Connect a device first</Text>
      </View>
    );
  }

  return (
    <>
      <ScrollView style={styles.scroll} contentContainerStyle={styles.content}>
        <ScreenHeader />

        <Text style={styles.pageTitle}>Settings</Text>
        <Text style={styles.pageSubtitle}>
          {screen === 'categories' ? 'Choose a settings category to edit.' : selectedCategory?.label || 'Settings'}
        </Text>

        <View style={styles.sectionRow}>
          <Text style={styles.sectionLabel}>ACTIVE UNIT</Text>
          <TouchableOpacity onPress={refreshUnits} hitSlop={{ top: 8, bottom: 8, left: 8, right: 8 }}>
            <MaterialIcons name="refresh" size={18} color={colors.accent} />
          </TouchableOpacity>
        </View>

        {units.length === 0 ? (
          <View style={styles.emptyCard}>
            <Text style={styles.emptyText}>Tap refresh to discover units</Text>
          </View>
        ) : (
          <View style={styles.unitGrid}>
            {units.map((u) => {
              const isActive = u.index === activeUnitIndex;
              return (
                <TouchableOpacity
                  key={u.index}
                  style={[styles.unitCard, isActive && styles.unitCardActive]}
                  onPress={() => selectUnit(u.index, u.kind)}
                >
                  <View style={styles.unitCardInner}>
                    <View style={styles.unitCardTop}>
                      <Text style={styles.unitType}>{deviceTypeLabel(u)}</Text>
                      <MaterialIcons
                        name={isActive ? 'radio-button-checked' : 'radio-button-unchecked'}
                        size={20}
                        color={isActive ? colors.accent : colors.textGhost}
                      />
                    </View>
                    <Text style={styles.unitIndex}>INDEX {u.index}</Text>
                  </View>
                </TouchableOpacity>
              );
            })}
          </View>
        )}

        <Text style={styles.sectionLabel}>DIAGNOSTICS</Text>
        <TouchableOpacity style={styles.diagCard} onPress={refreshErrors}>
          <MaterialIcons
            name={errors.length > 0 ? 'error-outline' : 'check-circle'}
            size={24}
            color={errors.length > 0 ? colors.red : colors.green}
          />
          <View style={styles.diagInfo}>
            <Text style={styles.diagTitle}>Active Error Logs</Text>
            <Text style={[styles.diagSub, { color: errors.length > 0 ? colors.red : colors.green }]}>
              {errors.length > 0 ? errors.map((c) => `Error #${c}`).join(', ') : 'System operating normally'}
            </Text>
          </View>
          <View style={styles.errorBadge}>
            <Text style={styles.errorBadgeText}>{errors.length}</Text>
          </View>
          <MaterialIcons name="chevron-right" size={20} color={colors.textGhost} />
        </TouchableOpacity>

        {screen === 'categories' && (
          <>
            <Text style={[styles.sectionLabel, { marginTop: spacing.sm }]}>CONFIGURATION PROFILES</Text>
            <View style={styles.categoryList}>
              {categories.map((cat, idx) => (
                <TouchableOpacity
                  key={cat.key}
                  style={[styles.categoryRow, idx === categories.length - 1 && { borderBottomWidth: 0 }]}
                  onPress={() => openCategory(cat)}
                >
                  <View style={styles.categoryIconCircle}>
                    <MaterialIcons name={cat.icon} size={16} color={colors.accent} />
                  </View>
                  <Text style={styles.categoryLabel}>{cat.label}</Text>
                  <MaterialIcons name="chevron-right" size={20} color={colors.textGhost} />
                </TouchableOpacity>
              ))}
            </View>
          </>
        )}

        {screen === 'detail' && selectedCategory && (
          <>
            <View style={[styles.sectionRow, { marginTop: spacing.sm }]}>
              <Text style={styles.sectionLabel}>{selectedCategory.label.toUpperCase()}</Text>
              <TouchableOpacity onPress={() => { clearPendingSettingActivity(); setScreen('categories'); }}>
                <Text style={styles.backText}>Back</Text>
              </TouchableOpacity>
            </View>

            <View style={styles.detailCard}>
              {selectedCategory.settings.map((s, idx) => {
                const valueText = formatSettingValue(s, settingValues[s.key]);
                const statusMeta = getStatusMeta(s.key);
                return (
                  <TouchableOpacity
                    key={s.key}
                    style={[styles.settingRow, idx === selectedCategory.settings.length - 1 && { borderBottomWidth: 0 }]}
                    onPress={() => openEditor(s)}
                  >
                    <View style={{ flex: 1 }}>
                      <Text style={styles.settingName}>{s.label}</Text>
                      <Text style={styles.settingSub}>Block {s.block} / ID {s.id}</Text>
                      {statusMeta && (
                        <View style={styles.inlineStatus}>
                          {loadingSettings[s.key] ? (
                            <ActivityIndicator size="small" color={statusMeta.color} />
                          ) : (
                            <MaterialIcons name={statusMeta.icon} size={14} color={statusMeta.color} />
                          )}
                          <Text style={[styles.inlineStatusText, { color: statusMeta.color }]}>{statusMeta.text}</Text>
                        </View>
                      )}
                    </View>
                    <View style={styles.settingValueWrap}>
                      <Text style={styles.settingValue}>{valueText}</Text>
                      <MaterialIcons name="edit" size={16} color={colors.textFaint} />
                    </View>
                  </TouchableOpacity>
                );
              })}
            </View>
          </>
        )}

        <View style={{ height: spacing.lg }} />
      </ScrollView>

      <Modal visible={editorVisible} transparent animationType="fade" onRequestClose={() => setEditorVisible(false)}>
        <View style={styles.modalOverlay}>
          <View style={styles.modalCard}>
            <Text style={styles.modalTitle}>{editorDef?.label || 'Setting'}</Text>
            <Text style={styles.modalValue}>{editorDef ? formatSettingValue(editorDef, editorDraft) : '--'}</Text>

            {editorDef && (
              <Text style={styles.modalRange}>
                Range: {formatSettingValue(editorDef, settingRanges[editorDef.key]?.min)} - {formatSettingValue(editorDef, settingRanges[editorDef.key]?.max)}
              </Text>
            )}

            {editorStatus && (
              <View style={styles.modalStatusRow}>
                {editorStatus === 'saving' || editorStatus === 'queued' ? (
                  <ActivityIndicator size="small" color={colors.solar} />
                ) : (
                  <MaterialIcons
                    name={editorStatus === 'saved' ? 'check-circle' : editorStatus === 'error' ? 'error-outline' : 'schedule'}
                    size={16}
                    color={editorStatus === 'saved' ? colors.green : editorStatus === 'error' ? colors.red : colors.solar}
                  />
                )}
                <Text
                  style={[
                    styles.modalStatusText,
                    editorStatus === 'saved' && { color: colors.green },
                    editorStatus === 'error' && { color: colors.red },
                    (editorStatus === 'saving' || editorStatus === 'queued' || editorStatus === 'timeout') && { color: colors.solar },
                  ]}
                >
                  {editorStatus === 'saving' || editorStatus === 'queued'
                    ? 'Saving to device...'
                    : editorStatus === 'saved'
                      ? 'Saved successfully'
                      : editorStatus === 'error'
                        ? 'Save failed'
                        : 'Device did not confirm yet'}
                </Text>
              </View>
            )}

            {editorDef?.prefix === PREFIX.ENUM && editorDef.enumLabels ? (
              <View style={styles.enumGrid}>
                {editorDef.enumLabels.map((label, index) => {
                  const selected = index === editorEnumIndex;
                  return (
                    <TouchableOpacity
                      key={`${editorDef.key}-${label}`}
                      style={[styles.enumChip, selected && styles.enumChipActive]}
                      onPress={() => setEnumDraft(index)}
                    >
                      <Text style={[styles.enumChipText, selected && styles.enumChipTextActive]}>{label}</Text>
                    </TouchableOpacity>
                  );
                })}
              </View>
            ) : (
              <>
                <View style={styles.stepLegendRow}>
                  <Text style={styles.stepLegend}>Quick</Text>
                  <Text style={styles.stepLegend}>Fine</Text>
                  <Text style={styles.stepLegend}>Fine</Text>
                  <Text style={styles.stepLegend}>Quick</Text>
                </View>
                <View style={styles.adjustRowFour}>
                  <TouchableOpacity style={styles.adjustBtn} onPress={() => adjustDraft(-editorFastStep)}>
                    <Text style={styles.adjustBtnLabel}>- -</Text>
                  </TouchableOpacity>
                  <TouchableOpacity style={styles.adjustBtn} onPress={() => adjustDraft(-(editorDef?.step || 65536))}>
                    <Text style={styles.adjustBtnLabel}>-</Text>
                  </TouchableOpacity>
                  <TouchableOpacity style={styles.adjustBtn} onPress={() => adjustDraft(editorDef?.step || 65536)}>
                    <Text style={styles.adjustBtnLabel}>+</Text>
                  </TouchableOpacity>
                  <TouchableOpacity style={styles.adjustBtn} onPress={() => adjustDraft(editorFastStep)}>
                    <Text style={styles.adjustBtnLabel}>+ +</Text>
                  </TouchableOpacity>
                </View>
              </>
            )}

            <View style={styles.modalActions}>
              <TouchableOpacity style={styles.modalSecondary} onPress={() => setEditorVisible(false)}>
                <Text style={styles.modalSecondaryText}>Cancel</Text>
              </TouchableOpacity>
              <TouchableOpacity style={styles.modalPrimary} onPress={saveEditor}>
                <Text style={styles.modalPrimaryText}>Save</Text>
              </TouchableOpacity>
            </View>
          </View>
        </View>
      </Modal>
    </>
  );
}

const styles = StyleSheet.create({
  scroll: { flex: 1, backgroundColor: colors.bg },
  content: { paddingHorizontal: spacing.md, paddingTop: spacing.lg, paddingBottom: spacing.md },
  center: { flex: 1, backgroundColor: colors.bg, alignItems: 'center', justifyContent: 'center', gap: spacing.sm },
  centerHint: { color: colors.textMuted, fontSize: fontSize.md, fontWeight: '700' },

  pageTitle: { color: colors.text, fontSize: 28, fontWeight: '800', marginBottom: 4 },
  pageSubtitle: { color: colors.textMuted, fontSize: fontSize.sm, marginBottom: spacing.lg },

  sectionLabel: {
    color: colors.textMuted,
    fontSize: 11,
    fontWeight: '700',
    letterSpacing: 1.2,
    textTransform: 'uppercase',
    marginBottom: spacing.sm,
  },
  sectionRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: spacing.sm },
  backText: { color: colors.accent, fontWeight: '700', fontSize: fontSize.sm },

  unitGrid: { flexDirection: 'row', gap: spacing.sm, marginBottom: spacing.lg },
  unitCard: { flex: 1, borderRadius: radius.md, borderWidth: 1, borderColor: colors.border, overflow: 'hidden' },
  unitCardActive: { borderColor: colors.accent, borderLeftWidth: 3 },
  unitCardInner: { backgroundColor: colors.bgElevated, padding: spacing.md },
  unitCardTop: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 4 },
  unitType: { color: colors.text, fontSize: 18, fontWeight: '800' },
  unitIndex: { color: colors.accent, fontSize: 11, fontWeight: '700', letterSpacing: 0.8 },

  emptyCard: {
    backgroundColor: colors.bgElevated,
    borderRadius: radius.md,
    borderWidth: 1,
    borderColor: colors.border,
    padding: spacing.lg,
    alignItems: 'center',
    marginBottom: spacing.lg,
  },
  emptyText: { color: colors.textFaint, fontSize: fontSize.sm },

  diagCard: {
    backgroundColor: colors.bgElevated,
    borderRadius: radius.md,
    borderWidth: 1,
    borderColor: colors.border,
    padding: spacing.md,
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: spacing.lg,
    gap: spacing.sm,
  },
  diagInfo: { flex: 1 },
  diagTitle: { color: colors.text, fontSize: fontSize.md, fontWeight: '700' },
  diagSub: { fontSize: fontSize.sm, marginTop: 2 },
  errorBadge: {
    width: 28,
    height: 28,
    borderRadius: 14,
    backgroundColor: colors.bgInset,
    alignItems: 'center',
    justifyContent: 'center',
  },
  errorBadgeText: { color: colors.textLight, fontWeight: '800', fontSize: 13 },

  categoryList: {
    backgroundColor: colors.bgElevated,
    borderRadius: radius.md,
    borderWidth: 1,
    borderColor: colors.border,
    marginBottom: spacing.lg,
    overflow: 'hidden',
  },
  categoryRow: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: spacing.md,
    borderBottomWidth: 1,
    borderBottomColor: colors.borderSubtle,
    gap: spacing.sm,
  },
  categoryIconCircle: {
    width: 36,
    height: 36,
    borderRadius: 18,
    backgroundColor: colors.bgInset,
    alignItems: 'center',
    justifyContent: 'center',
  },
  categoryLabel: { flex: 1, color: colors.text, fontSize: fontSize.md },

  detailCard: {
    backgroundColor: colors.bgElevated,
    borderRadius: radius.md,
    borderWidth: 1,
    borderColor: colors.border,
    overflow: 'hidden',
  },
  settingRow: {
    paddingHorizontal: spacing.md,
    paddingVertical: spacing.md,
    borderBottomWidth: 1,
    borderBottomColor: colors.borderSubtle,
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.sm,
  },
  settingName: { color: colors.text, fontSize: fontSize.md, fontWeight: '700' },
  settingSub: { color: colors.textFaint, fontSize: fontSize.xs, marginTop: 2 },
  inlineStatus: { flexDirection: 'row', alignItems: 'center', gap: 4, marginTop: 6 },
  inlineStatusText: { fontSize: fontSize.xs, fontWeight: '700' },
  settingValueWrap: { flexDirection: 'row', alignItems: 'center', gap: 6 },
  settingValue: { color: colors.accent, fontSize: fontSize.sm, fontWeight: '700' },

  modalOverlay: {
    flex: 1,
    backgroundColor: colors.bgOverlay,
    alignItems: 'center',
    justifyContent: 'center',
    padding: spacing.md,
  },
  modalCard: {
    width: '100%',
    backgroundColor: '#171717',
    borderRadius: radius.lg,
    borderWidth: 1,
    borderColor: colors.border,
    padding: spacing.md,
  },
  modalTitle: { color: colors.text, fontSize: fontSize.lg, fontWeight: '800' },
  modalValue: { color: colors.accent, fontSize: 28, fontWeight: '800', marginTop: 8 },
  modalRange: { color: colors.textMuted, fontSize: fontSize.xs, marginTop: 6 },
  modalStatusRow: { flexDirection: 'row', alignItems: 'center', gap: 6, marginTop: 8 },
  modalStatusText: { fontSize: fontSize.xs, fontWeight: '700' },
  enumGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: spacing.sm, marginTop: spacing.md },
  enumChip: {
    minWidth: '30%',
    borderRadius: radius.md,
    borderWidth: 1,
    borderColor: colors.borderInput,
    backgroundColor: '#232323',
    paddingVertical: spacing.sm,
    paddingHorizontal: spacing.md,
    alignItems: 'center',
  },
  enumChipActive: {
    backgroundColor: colors.accent,
    borderColor: colors.accent,
  },
  enumChipText: { color: '#ddd', fontWeight: '700', fontSize: fontSize.sm },
  enumChipTextActive: { color: colors.text },
  stepLegendRow: { flexDirection: 'row', gap: spacing.sm, marginTop: spacing.md, marginBottom: 6 },
  stepLegend: { flex: 1, textAlign: 'center', color: '#777', fontSize: fontSize.xs, fontWeight: '700' },
  adjustRowFour: { flexDirection: 'row', gap: spacing.sm },
  adjustBtn: {
    flex: 1,
    borderRadius: radius.md,
    backgroundColor: colors.bgInset,
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: spacing.sm,
  },
  adjustBtnLabel: { color: colors.text, fontSize: fontSize.md, fontWeight: '800', letterSpacing: 0.5 },
  modalActions: { flexDirection: 'row', gap: spacing.sm, marginTop: spacing.md },
  modalSecondary: {
    flex: 1,
    borderRadius: radius.md,
    borderWidth: 1,
    borderColor: colors.borderInput,
    alignItems: 'center',
    paddingVertical: spacing.sm,
  },
  modalSecondaryText: { color: colors.textDim, fontWeight: '700' },
  modalPrimary: {
    flex: 1,
    borderRadius: radius.md,
    backgroundColor: colors.accent,
    alignItems: 'center',
    paddingVertical: spacing.sm,
  },
  modalPrimaryText: { color: colors.text, fontWeight: '800' },
});
