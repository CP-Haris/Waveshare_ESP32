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

function unitTypeLabel(unitType) {
  if (unitType === 1) return 'LPS';
  if (unitType === 2) return 'BMS';
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

function MetricTile({ icon, label, value, unit, detail, accentColor = colors.accent }) {
  return (
    <View style={styles.metricTile}>
      <View style={styles.metricTopRow}>
        <View style={[styles.metricIcon, { borderColor: accentColor }]}>
          <MaterialIcons name={icon} size={18} color={accentColor} />
        </View>
        <Text style={styles.metricLabel}>{label}</Text>
      </View>
      <Text style={styles.metricValue}>
        {value} <Text style={styles.metricUnit}>{unit}</Text>
      </Text>
      {!!detail && <Text style={styles.metricDetail}>{detail}</Text>}
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

function SystemStatusItem({ item }) {
  return (
    <View style={styles.systemItem}>
      <View style={[styles.systemDot, { backgroundColor: item.active ? item.color : colors.bgInset }]} />
      <MaterialIcons name={item.icon} size={18} color={item.active ? item.color : colors.textMuted} />
      <View style={styles.systemCopy}>
        <Text style={styles.systemLabel}>{item.label}</Text>
        <Text style={[styles.systemState, item.active && { color: item.color }]}>{item.stateLabel}</Text>
      </View>
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

  const systemIndicators = [
    { key: 'inverter', label: 'Inverter', icon: 'flash-on', color: colors.accent, active: dashboard.inverterState >= 1 && dashboard.inverterFail === 0, stateLabel: stateLabel(dashboard.inverterState, dashboard.inverterFail) },
    { key: 'charger', label: 'Charger', icon: 'ev-station', color: colors.green, active: dashboard.chargerState >= 1 && dashboard.chargerFail === 0, stateLabel: stateLabel(dashboard.chargerState, dashboard.chargerFail) },
    { key: 'Solar', label: 'Solar', icon: 'wb-sunny', color: colors.solar, active: dashboard.solarState >= 1 && dashboard.solarFail === 0, stateLabel: stateLabel(dashboard.solarState, dashboard.solarFail) },
    { key: 'dcin', label: 'DC input', icon: 'input', color: colors.accent, active: dashboard.dcInState >= 1 && dashboard.dcInFail === 0, stateLabel: stateLabel(dashboard.dcInState, dashboard.dcInFail) },
    { key: 'dcout', label: 'DC output', icon: 'output', color: colors.green, active: dashboard.dcOutState >= 1 && dashboard.dcOutFail === 0, stateLabel: stateLabel(dashboard.dcOutState, dashboard.dcOutFail) },
  ];

  return (
    <ScrollView style={styles.scroll} contentContainerStyle={styles.content}>
      <ScreenHeader />

      <View style={[styles.heroPanel, statusOk ? styles.heroPanelOk : styles.heroPanelAlert]}>
        <View style={styles.heroTopRow}>
          <View>
            <Text style={styles.overline}>{unitTypeLabel(dashboard.unitType)}</Text>
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

      <View style={styles.controlRow}>
        <ToggleControl icon="power" label="Inverter" value={inverterOn} onValueChange={() => canGatewayService.toggleFunc(0)} />
        <ToggleControl icon="electrical-services" label="DC output" value={dcOutOn} onValueChange={() => canGatewayService.toggleFunc(1)} />
      </View>

      <View style={styles.sectionHeaderRow}>
        <Text style={styles.sectionTitle}>Energy flow</Text>
        <Text style={styles.sectionMeta}>Live</Text>
      </View>

      <View style={styles.metricGrid}>
        <MetricTile icon="battery-full" label="Battery" value={batteryPowerText.value} unit={batteryPowerText.unit} detail={`${formatFixed(dashboard.batteryVoltage, 1)} V | ${formatFixed(dashboard.batteryCurrent, 1)} A`} accentColor={dashboard.batteryCurrent >= 0 ? colors.green : colors.orange} />
        <MetricTile icon="output" label="DC output" value={dcOutPowerText.value} unit={dcOutPowerText.unit} detail={`${formatFixed(dashboard.dcOutVoltage, 2)} V | ${formatFixed(dashboard.dcOutCurrent, 1)} A`} accentColor={colors.accent} />
        <MetricTile icon="input" label="DC input" value={dcInPowerText.value} unit={dcInPowerText.unit} detail={`${formatFixed(dashboard.dcInVoltage, 2)} V | ${formatFixed(dashboard.dcInCurrent, 1)} A`} accentColor={colors.green} />
        <MetricTile icon="bolt" label="AC input" value={acInPowerText.value} unit={acInPowerText.unit} detail={`AC out ${acOutPowerText.value} ${acOutPowerText.unit}`} accentColor={colors.solar} />
      </View>

      <View style={styles.sectionPanel}>
        <View style={styles.panelHeaderRow}>
          <Text style={styles.sectionTitle}>System state</Text>
          <MaterialIcons name="memory" size={18} color={colors.textMuted} />
        </View>
        <View style={styles.systemList}>
          {systemIndicators.map((item) => <SystemStatusItem key={item.key} item={item} />)}
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

  sectionHeaderRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: spacing.sm },
  sectionTitle: { color: colors.text, fontSize: fontSize.md, fontWeight: '800' },
  sectionMeta: { color: colors.textMuted, fontSize: fontSize.xs, fontWeight: '700' },

  metricGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between', marginBottom: spacing.sm },
  metricTile: {
    width: '48.5%',
    minHeight: 116,
    backgroundColor: colors.bgElevated,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: colors.border,
    padding: spacing.sm,
    marginBottom: spacing.sm,
  },
  metricTopRow: { flexDirection: 'row', alignItems: 'center', gap: spacing.sm, marginBottom: spacing.sm },
  metricIcon: { width: 32, height: 32, borderRadius: 8, borderWidth: 1, alignItems: 'center', justifyContent: 'center', backgroundColor: colors.bgCard },
  metricLabel: { color: colors.textMuted, fontSize: fontSize.xs, fontWeight: '800', flex: 1 },
  metricValue: { color: colors.text, fontSize: 24, fontWeight: '800', lineHeight: 30 },
  metricUnit: { fontSize: 13, color: colors.textMuted, fontWeight: '700' },
  metricDetail: { color: colors.textFaint, fontSize: fontSize.xs, marginTop: 4, fontWeight: '700' },

  sectionPanel: {
    backgroundColor: colors.bgElevated,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: colors.border,
    padding: spacing.md,
    marginBottom: spacing.sm,
  },
  panelHeaderRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', marginBottom: spacing.sm },
  systemList: { gap: spacing.sm },
  systemItem: { flexDirection: 'row', alignItems: 'center', minHeight: 42, borderBottomWidth: 1, borderBottomColor: colors.borderSubtle, paddingBottom: spacing.sm, gap: spacing.sm },
  systemDot: { width: 8, height: 8, borderRadius: 4 },
  systemCopy: { flex: 1, flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', gap: spacing.sm },
  systemLabel: { color: colors.text, fontSize: fontSize.sm, fontWeight: '700' },
  systemState: { color: colors.textMuted, fontSize: fontSize.sm, fontWeight: '700' },
});
