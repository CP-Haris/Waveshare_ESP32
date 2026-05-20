import React, { useState, useEffect, useRef } from 'react';
import {
  View,
  Text,
  ScrollView,
  StyleSheet,
  Switch,
} from 'react-native';
import { MaterialIcons } from '@expo/vector-icons';
import { colors, spacing, fontSize } from '../utils/theme';
import bleService from '../services/bleService';
import canGatewayService from '../services/canGatewayService';
import ScreenHeader from '../components/ScreenHeader';
import SocRing from '../components/SocRing';

const DEV_LPS = 1;
const DEV_BMS = 2;

function finiteNumber(value, fallback = 0) {
  return Number.isFinite(value) ? value : fallback;
}

function formatPower(watts) {
  const value = finiteNumber(watts);
  const absValue = Math.abs(value);
  if (absValue >= 1000) {
    return { value: (value / 1000).toFixed(absValue >= 10000 ? 0 : 1), unit: 'kW' };
  }
  return { value: String(Math.round(value)), unit: 'W' };
}

function formatFixed(value, decimals) {
  return finiteNumber(value).toFixed(decimals);
}

function productFamily(dashboard) {
  const partNumber = String(dashboard.partNumber || '').toUpperCase();
  if (partNumber.startsWith('CB')) return 'battery';
  if (partNumber.startsWith('CL')) return 'lps';
  if (dashboard.unitType === DEV_BMS) return 'battery';
  return 'lps';
}

function unitTypeLabel(dashboard) {
  const family = productFamily(dashboard);
  if (family === 'battery') return 'Battery';
  if (dashboard.unitType === DEV_LPS || family === 'lps') return 'LPS';
  return 'CAN Unit';
}

function stateLabel(state, failCode) {
  if (failCode > 0) return `Fail ${failCode}`;
  switch (state) {
    case -1: return 'Error';
    case 0: return 'Off';
    case 1: return 'On';
    case 2: return 'Standby';
    case 3: return 'Charge';
    case 4: return 'Float';
    default: return 'Idle';
  }
}

function SystemMetricCard({ icon, label, value, unit, detail, active, fail, stateText, accentColor = colors.accent, wide = false }) {
  const stateColor = fail ? colors.red : active ? accentColor : colors.textMuted;

  return (
    <View style={[styles.systemMetricCard, wide && styles.systemMetricCardWide]}>
      <View style={styles.systemMetricTopRow}>
        <View style={[styles.systemMetricIcon, { borderColor: accentColor }]}>
          <MaterialIcons name={icon} size={18} color={accentColor} />
        </View>
        <View style={styles.systemMetricHeaderCopy}>
          <Text style={styles.systemMetricLabel}>{label}</Text>
          <View style={styles.systemMetricStateRow}>
            <View style={[styles.systemDot, { backgroundColor: stateColor }]} />
            <Text style={[styles.systemMetricState, { color: stateColor }]}>{stateText}</Text>
          </View>
        </View>
      </View>
      <Text style={styles.systemMetricValue}>
        {value} <Text style={styles.systemMetricUnit}>{unit}</Text>
      </Text>
      {!!detail && <Text style={styles.systemMetricDetail}>{detail}</Text>}
    </View>
  );
}

function ToggleControl({ icon, label, value, onValueChange }) {
  return (
    <View style={[styles.controlCard, value && styles.controlCardActive]}>
      <View style={styles.controlTextRow}>
        <View style={[styles.controlIcon, value && styles.controlIconActive]}>
          <MaterialIcons name={icon} size={18} color={value ? colors.green : colors.textMuted} />
        </View>
        <View style={styles.controlCopy}>
          <Text style={styles.controlLabel}>{label}</Text>
          <Text style={[styles.controlState, value && styles.controlStateActive]}>{value ? 'On' : 'Off'}</Text>
        </View>
      </View>
      <Switch
        value={value}
        onValueChange={onValueChange}
        trackColor={{ false: colors.borderInput, true: colors.greenBg }}
        thumbColor={value ? colors.green : colors.textMuted}
      />
    </View>
  );
}

export default function DashboardScreen() {
  const [data, setData] = useState(null);
  const [connected, setConnected] = useState(bleService.isConnected);
  const pollRef = useRef(null);

  useEffect(() => {
    const startPolling = () => {
      if (pollRef.current) clearInterval(pollRef.current);
      if (bleService.isConnected) {
        canGatewayService.requestUnits();
        canGatewayService.requestDashboard();
        pollRef.current = setInterval(() => {
          if (bleService.isConnected) canGatewayService.requestDashboard();
        }, 2000);
      }
    };

    const unsubData = canGatewayService.onNotification((msg) => {
      if (msg.type === 'dashboard') setData(msg.data);
    });

    const unsubConn = bleService.onConnectionChange((c) => {
      setConnected(c);
      if (c) startPolling();
      else {
        clearInterval(pollRef.current);
        pollRef.current = null;
      }
    });

    if (bleService.isConnected) startPolling();

    return () => {
      unsubData();
      unsubConn();
      if (pollRef.current) clearInterval(pollRef.current);
    };
  }, []);

  if (!connected) {
    return (
      <View style={styles.center}>
        <MaterialIcons name="bluetooth-disabled" size={42} color={colors.textGhost} />
        <Text style={styles.centerTitle}>Not Connected</Text>
        <Text style={styles.centerHint}>Go to Connect tab and pair a device</Text>
      </View>
    );
  }

  if (!data) {
    return (
      <View style={styles.center}>
        <Text style={styles.centerHint}>Waiting for data...</Text>
      </View>
    );
  }

  const dashboard = data;
  const family = productFamily(dashboard);
  const isBatteryProduct = family === 'battery';
  const pct = Math.max(0, Math.min(100, dashboard.soc));
  const timeStr = dashboard.socTimeMin > 0
    ? `${Math.floor(dashboard.socTimeMin / 60)}h ${String(dashboard.socTimeMin % 60).padStart(2, '0')}m`
    : '--h --m';
  const inverterOn = dashboard.inverterState >= 1;
  const dcOutOn = dashboard.dcOutState >= 1;
  const statusOk = dashboard.errorCount === 0;
  const batteryPower = dashboard.batteryVoltage * dashboard.batteryCurrent;
  const dcInPower = dashboard.dcInVoltage * dashboard.dcInCurrent;
  const dcOutPower = dashboard.dcOutVoltage * dashboard.dcOutCurrent;
  const batteryPowerText = formatPower(batteryPower);
  const dcInPowerText = formatPower(dcInPower);
  const dcOutPowerText = formatPower(dcOutPower);
  const acInPowerText = formatPower(dashboard.acInPower);
  const acOutPowerText = formatPower(dashboard.acOutPower);
  const chargeMode = dashboard.batteryCurrent >= 0 ? 'Charging' : 'Discharging';
  const healthText = statusOk ? 'Healthy' : 'Attention';

  const batteryCard = {
    key: 'battery',
    label: 'Battery',
    icon: 'battery-full',
    value: batteryPowerText.value,
    unit: batteryPowerText.unit,
    detail: `${formatFixed(dashboard.batteryVoltage, 1)} V | ${formatFixed(dashboard.batteryCurrent, 1)} A`,
    active: statusOk,
    fail: !statusOk,
    stateText: statusOk ? chargeMode : 'Attention',
    accentColor: dashboard.batteryCurrent >= 0 ? colors.green : colors.orange,
    wide: true,
  };

  const systemCards = isBatteryProduct ? [batteryCard] : [
    {
      key: 'dcout',
      label: 'DC Output',
      icon: 'output',
      value: dcOutPowerText.value,
      unit: dcOutPowerText.unit,
      detail: `${formatFixed(dashboard.dcOutVoltage, 2)} V | ${formatFixed(dashboard.dcOutCurrent, 1)} A`,
      active: dashboard.dcOutState >= 1 && dashboard.dcOutFail === 0,
      fail: dashboard.dcOutFail > 0,
      stateText: stateLabel(dashboard.dcOutState, dashboard.dcOutFail),
      accentColor: colors.green,
    },
    {
      key: 'dcin',
      label: 'DC Input',
      icon: 'input',
      value: dcInPowerText.value,
      unit: dcInPowerText.unit,
      detail: `${formatFixed(dashboard.dcInVoltage, 2)} V | ${formatFixed(dashboard.dcInCurrent, 1)} A`,
      active: dashboard.dcInState >= 1 && dashboard.dcInFail === 0,
      fail: dashboard.dcInFail > 0,
      stateText: stateLabel(dashboard.dcInState, dashboard.dcInFail),
      accentColor: colors.accent,
    },
    {
      key: 'inverter',
      label: 'Inverter',
      icon: 'flash-on',
      value: acOutPowerText.value,
      unit: acOutPowerText.unit,
      detail: `${formatFixed(dashboard.acOutVoltage, 1)} V | ${formatFixed(dashboard.acOutCurrent, 2)} A`,
      active: dashboard.inverterState >= 1 && dashboard.inverterFail === 0,
      fail: dashboard.inverterFail > 0,
      stateText: stateLabel(dashboard.inverterState, dashboard.inverterFail),
      accentColor: colors.accent,
    },
    {
      key: 'charger',
      label: 'Charger',
      icon: 'ev-station',
      value: acInPowerText.value,
      unit: acInPowerText.unit,
      detail: `${formatFixed(dashboard.acInVoltage, 1)} V | ${formatFixed(dashboard.acInCurrent, 2)} A`,
      active: dashboard.chargerState >= 1 && dashboard.chargerFail === 0,
      fail: dashboard.chargerFail > 0,
      stateText: stateLabel(dashboard.chargerState, dashboard.chargerFail),
      accentColor: colors.green,
    },
    {
      key: 'solar',
      label: 'Solar',
      icon: 'wb-sunny',
      value: formatFixed(dashboard.solarCurrent, 1),
      unit: 'A',
      detail: 'Solar charge current',
      active: dashboard.solarState >= 1 && dashboard.solarFail === 0,
      fail: dashboard.solarFail > 0,
      stateText: stateLabel(dashboard.solarState, dashboard.solarFail),
      accentColor: colors.solar,
      wide: true,
    },
  ];

  return (
    <ScrollView style={styles.scroll} contentContainerStyle={styles.content}>
      <ScreenHeader />

      <View style={[styles.heroPanel, statusOk ? styles.heroPanelOk : styles.heroPanelAlert]}>
        <View style={styles.heroTopRow}>
          <View>
            <Text style={styles.overline}>{unitTypeLabel(dashboard)}</Text>
            <Text style={styles.heroTitle}>{chargeMode}</Text>
            <Text style={styles.heroSubtitle}>{statusOk ? 'System stable' : 'Attention required'}</Text>
          </View>
          <View style={[styles.healthPill, statusOk ? styles.healthPillOk : styles.healthPillAlert]}>
            <MaterialIcons
              name={statusOk ? 'check-circle' : 'error-outline'}
              size={16}
              color={statusOk ? colors.green : colors.red}
            />
            <Text style={[styles.healthText, statusOk ? styles.healthTextOk : styles.healthTextAlert]}>{healthText}</Text>
          </View>
        </View>

        <View style={styles.heroBody}>
          <SocRing pct={pct} size={146} />
          <View style={styles.heroStats}>
            <View style={styles.statLine}>
              <Text style={styles.statLabel}>Time left</Text>
              <Text style={styles.statValue}>{timeStr}</Text>
            </View>
            <View style={styles.statLine}>
              <Text style={styles.statLabel}>Battery</Text>
              <Text style={styles.statValue}>{formatFixed(dashboard.batteryVoltage, 1)} V</Text>
            </View>
            <View style={styles.statLine}>
              <Text style={styles.statLabel}>Current</Text>
              <Text style={[styles.statValue, dashboard.batteryCurrent >= 0 ? styles.valuePositive : styles.valueWarm]}>
                {dashboard.batteryCurrent >= 0 ? '+' : ''}{formatFixed(dashboard.batteryCurrent, 1)} A
              </Text>
            </View>
            <View style={[styles.statLine, styles.statLineLast]}>
              <Text style={styles.statLabel}>Battery power</Text>
              <Text style={styles.statValue}>{batteryPowerText.value} {batteryPowerText.unit}</Text>
            </View>
          </View>
        </View>
      </View>

      {!isBatteryProduct && (
        <View style={styles.controlRow}>
          <ToggleControl icon="power" label="Inverter" value={inverterOn} onValueChange={() => canGatewayService.toggleFunc(0)} />
          <ToggleControl icon="electrical-services" label="DC output" value={dcOutOn} onValueChange={() => canGatewayService.toggleFunc(1)} />
        </View>
      )}

      <View style={styles.systemsPanel}>
        <View style={styles.panelHeaderRow}>
          <View>
            <Text style={styles.sectionTitle}>Systems</Text>
            <Text style={styles.sectionMeta}>{isBatteryProduct ? 'Battery unit' : 'LPS functions'}</Text>
          </View>
          <MaterialIcons name={isBatteryProduct ? 'battery-full' : 'memory'} size={18} color={colors.textMuted} />
        </View>
        <View style={styles.systemsGrid}>
          {systemCards.map((item) => (
            <SystemMetricCard
              key={item.key}
              icon={item.icon}
              label={item.label}
              value={item.value}
              unit={item.unit}
              detail={item.detail}
              active={item.active}
              fail={item.fail}
              stateText={item.stateText}
              accentColor={item.accentColor}
              wide={item.wide}
            />
          ))}
        </View>
      </View>

      <View style={{ height: spacing.lg }} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  scroll: { flex: 1, backgroundColor: colors.bg },
  content: { paddingHorizontal: spacing.md, paddingTop: spacing.lg, paddingBottom: spacing.md },
  center: { flex: 1, backgroundColor: colors.bg, alignItems: 'center', justifyContent: 'center', padding: spacing.xl },
  centerTitle: { color: colors.text, fontSize: fontSize.xl, fontWeight: '800', marginTop: spacing.md },
  centerHint: { color: colors.textMuted, fontSize: fontSize.sm, marginTop: 6 },

  heroPanel: {
    backgroundColor: colors.bgElevated,
    borderRadius: 8,
    borderWidth: 1,
    padding: spacing.md,
    marginBottom: spacing.md,
  },
  heroPanelOk: { borderColor: colors.greenBorder },
  heroPanelAlert: { borderColor: colors.redBg },
  heroTopRow: { flexDirection: 'row', alignItems: 'flex-start', justifyContent: 'space-between', gap: spacing.sm },
  overline: { color: colors.textMuted, fontSize: fontSize.xs, fontWeight: '700' },
  heroTitle: { color: colors.text, fontSize: 28, fontWeight: '800', lineHeight: 34, marginTop: 2 },
  heroSubtitle: { color: colors.textMuted, fontSize: fontSize.sm, marginTop: 2 },
  healthPill: { flexDirection: 'row', alignItems: 'center', gap: 6, paddingHorizontal: 10, paddingVertical: 6, borderRadius: 8, borderWidth: 1 },
  healthPillOk: { backgroundColor: colors.greenDeep, borderColor: colors.greenBorder },
  healthPillAlert: { backgroundColor: colors.redDeep, borderColor: colors.redBg },
  healthText: { fontSize: fontSize.sm, fontWeight: '700' },
  healthTextOk: { color: colors.green },
  healthTextAlert: { color: colors.red },
  heroBody: { flexDirection: 'row', alignItems: 'center', gap: spacing.md, marginTop: spacing.md },
  heroStats: { flex: 1 },
  statLine: { paddingVertical: 8, borderBottomWidth: 1, borderBottomColor: colors.borderSubtle },
  statLineLast: { borderBottomWidth: 0 },
  statLabel: { color: colors.textMuted, fontSize: fontSize.xs, fontWeight: '700' },
  statValue: { color: colors.text, fontSize: fontSize.md, fontWeight: '800', marginTop: 2 },
  valuePositive: { color: colors.green },
  valueWarm: { color: colors.orange },

  controlRow: { flexDirection: 'row', justifyContent: 'space-between', gap: spacing.sm, marginBottom: spacing.md },
  controlCard: {
    width: '48.5%',
    backgroundColor: colors.bgElevated,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: colors.border,
    padding: spacing.sm,
    minHeight: 74,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    gap: spacing.sm,
  },
  controlCardActive: { borderColor: colors.greenBorder, backgroundColor: colors.greenDeep },
  controlTextRow: { flexDirection: 'row', alignItems: 'center', gap: spacing.sm, flex: 1 },
  controlIcon: { width: 32, height: 32, borderRadius: 8, alignItems: 'center', justifyContent: 'center', backgroundColor: colors.bgCard, borderWidth: 1, borderColor: colors.border },
  controlIconActive: { borderColor: colors.greenBorder, backgroundColor: colors.greenBg },
  controlCopy: { flex: 1 },
  controlLabel: { color: colors.text, fontSize: fontSize.sm, fontWeight: '800' },
  controlState: { color: colors.textMuted, fontSize: fontSize.xs, marginTop: 2, fontWeight: '700' },
  controlStateActive: { color: colors.green },

  sectionTitle: { color: colors.text, fontSize: fontSize.md, fontWeight: '800' },
  sectionMeta: { color: colors.textMuted, fontSize: fontSize.xs, fontWeight: '700' },

  systemsPanel: {
    backgroundColor: colors.bgElevated,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: colors.border,
    padding: spacing.md,
    marginBottom: spacing.sm,
  },
  panelHeaderRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', marginBottom: spacing.sm },
  systemsGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  systemMetricCard: {
    width: '48.5%',
    minHeight: 122,
    backgroundColor: colors.bgCard,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: colors.borderSubtle,
    padding: spacing.sm,
    marginBottom: spacing.sm,
  },
  systemMetricCardWide: { width: '100%' },
  systemMetricTopRow: { flexDirection: 'row', alignItems: 'center', gap: spacing.sm, marginBottom: spacing.sm },
  systemMetricIcon: { width: 32, height: 32, borderRadius: 8, borderWidth: 1, alignItems: 'center', justifyContent: 'center', backgroundColor: colors.bgElevated },
  systemMetricHeaderCopy: { flex: 1, minWidth: 0 },
  systemMetricLabel: { color: colors.text, fontSize: fontSize.sm, fontWeight: '800' },
  systemMetricStateRow: { flexDirection: 'row', alignItems: 'center', gap: 6, marginTop: 3 },
  systemDot: { width: 8, height: 8, borderRadius: 4 },
  systemMetricState: { fontSize: fontSize.xs, fontWeight: '800' },
  systemMetricValue: { color: colors.text, fontSize: 24, fontWeight: '800', lineHeight: 30 },
  systemMetricUnit: { fontSize: 13, color: colors.textMuted, fontWeight: '700' },
  systemMetricDetail: { color: colors.textFaint, fontSize: fontSize.xs, marginTop: 4, fontWeight: '700' },
});
