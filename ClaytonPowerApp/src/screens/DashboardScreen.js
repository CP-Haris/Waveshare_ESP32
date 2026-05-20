import React, { useState, useEffect, useRef } from 'react';
import {
  View,
  Text,
  ScrollView,
  StyleSheet,
  Switch,
} from 'react-native';
import { MaterialIcons } from '@expo/vector-icons';
import { colors, spacing, fontSize, radius } from '../utils/theme';
import bleService from '../services/bleService';
import canGatewayService from '../services/canGatewayService';
import ScreenHeader from '../components/ScreenHeader';
import SocRing from '../components/SocRing';

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

  const d = data;
  const pct = Math.max(0, Math.min(100, d.soc));
  const timeStr = d.socTimeMin > 0
    ? `${Math.floor(d.socTimeMin / 60)}h ${String(d.socTimeMin % 60).padStart(2, '0')}m`
    : '--h --m';
  const inverterOn = d.inverterState >= 1;
  const dcOutOn = d.dcOutState >= 1;
  const statusOk = d.errorCount === 0;
  const dcOutPower = Math.round(d.dcOutVoltage * d.dcOutCurrent);

  const systemIndicators = [
    { key: 'inverter', label: 'Inverter', icon: 'flash-on', active: d.inverterState >= 1 && d.inverterFail === 0 },
    { key: 'charger', label: 'Charger', icon: 'ev-station', active: d.chargerState >= 1 && d.chargerFail === 0 },
    { key: 'solar', label: 'Solar', icon: 'wb-sunny', active: d.solarState >= 1 && d.solarFail === 0 },
    { key: 'dcin', label: 'DC IN', icon: 'input', active: d.dcInState >= 1 && d.dcInFail === 0 },
    { key: 'dcout', label: 'DC OUT', icon: 'output', active: d.dcOutState >= 1 && d.dcOutFail === 0 },
  ];

  return (
    <ScrollView style={styles.scroll} contentContainerStyle={styles.content}>
      <ScreenHeader />

      <View style={styles.socCard}>
        <SocRing pct={pct} />
        <View style={styles.socBottom}>
          <Text style={styles.timeLabel}>TIME LEFT</Text>
          <Text style={styles.timeValue}>{timeStr}</Text>
        </View>
      </View>

      <View style={styles.toggleRow}>
        <View style={styles.toggleCard}>
          <Text style={styles.toggleLabel}>INVERTER</Text>
          <Switch
            value={inverterOn}
            onValueChange={() => canGatewayService.toggleFunc(0)}
            trackColor={{ false: colors.borderInput, true: colors.accentDim }}
            thumbColor={inverterOn ? colors.accent : colors.textMuted}
          />
        </View>
        <View style={styles.toggleCard}>
          <Text style={styles.toggleLabel}>DC OUTPUT</Text>
          <Switch
            value={dcOutOn}
            onValueChange={() => canGatewayService.toggleFunc(1)}
            trackColor={{ false: colors.borderInput, true: colors.accentDim }}
            thumbColor={dcOutOn ? colors.accent : colors.textMuted}
          />
        </View>
      </View>

      <View style={styles.card}>
        <View style={styles.cardTitleRow}>
          <View style={styles.cardTitleLeft}>
            <MaterialIcons name="battery-full" size={18} color={colors.accent} />
            <Text style={styles.cardTitle}>Battery Status</Text>
          </View>
          <View style={[styles.badge, statusOk ? styles.badgeGreen : styles.badgeRed]}>
            <Text style={[styles.badgeText, statusOk ? styles.badgeGreenText : styles.badgeRedText]}>
              {statusOk ? 'HEALTHY' : 'ERROR'}
            </Text>
          </View>
        </View>
        <View style={styles.metricRow3}>
          <View style={styles.metric3}>
            <Text style={styles.metricLabel}>VOLTAGE</Text>
            <Text style={styles.metricValue}>{d.batteryVoltage.toFixed(1)} <Text style={styles.metricUnit}>V</Text></Text>
          </View>
          <View style={styles.metric3}>
            <Text style={styles.metricLabel}>CURRENT</Text>
            <Text style={[styles.metricValue, { color: d.batteryCurrent >= 0 ? colors.green : colors.orange }]}>
              {d.batteryCurrent >= 0 ? '+' : ''}{d.batteryCurrent.toFixed(0)} <Text style={styles.metricUnit}>A</Text>
            </Text>
          </View>
        </View>
      </View>

      <View style={styles.card}>
        <View style={styles.cardTitleRow}>
          <Text style={styles.sectionTag}>DC OUTPUT</Text>
          <MaterialIcons name="output" size={18} color={colors.accent} />
        </View>
        <Text style={styles.bigNum}>{dcOutPower} <Text style={styles.bigUnit}>W</Text></Text>
      </View>

      <View style={[styles.card, styles.cardGreenBorder]}>
        <View style={styles.cardTitleRow}>
          <Text style={styles.sectionTag}>AC INPUT</Text>
          <MaterialIcons name="input" size={18} color={colors.green} />
        </View>
        <Text style={[styles.bigNum, { color: colors.green }]}>{d.acInPower} <Text style={styles.bigUnit}>W</Text></Text>
      </View>

      <View style={styles.card}>
        <Text style={styles.sectionTag}>ACTIVE SYSTEMS</Text>
        <View style={styles.systemIconGrid}>
          {systemIndicators.map((item) => (
            <View key={item.key} style={[styles.systemIconItem, item.active ? styles.systemIconItemActive : styles.systemIconItemInactive]}>
              <MaterialIcons
                name={item.icon}
                size={20}
                color={item.active ? colors.green : colors.textMuted}
              />
              <Text style={[styles.systemIconLabel, item.active && styles.systemIconLabelActive]}>{item.label}</Text>
            </View>
          ))}
        </View>
      </View>

      <View style={[styles.card, d.errorCount > 0 ? styles.cardError : styles.cardOk]}>
        <View style={styles.errorRow}>
          <MaterialIcons
            name={d.errorCount > 0 ? 'error-outline' : 'check-circle'}
            size={22}
            color={d.errorCount > 0 ? colors.red : colors.green}
          />
          <View style={styles.errorInfo}>
            <Text style={styles.errorTitle}>{d.errorCount} Active Errors</Text>
            <Text style={styles.errorSub}>
              {d.errorCount > 0 ? d.errorCodes.map(c => `#${c}`).join(', ') : 'System operating normally'}
            </Text>
          </View>
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

  socCard: {
    backgroundColor: '#141a14',
    borderRadius: radius.lg,
    borderWidth: 1,
    borderColor: colors.greenBorder,
    padding: spacing.md,
    alignItems: 'center',
    marginBottom: spacing.sm,
  },
  socBottom: { alignItems: 'center', marginTop: spacing.sm },
  timeLabel: { color: colors.textMuted, fontSize: 10, letterSpacing: 1.5, fontWeight: '700' },
  timeValue: { color: colors.text, fontSize: 24, fontWeight: '800', marginTop: 2 },

  toggleRow: { flexDirection: 'row', gap: spacing.sm, marginBottom: spacing.sm },
  toggleCard: {
    flex: 1,
    backgroundColor: colors.bgElevated,
    borderRadius: radius.md,
    borderWidth: 1,
    borderColor: colors.border,
    padding: spacing.md,
    alignItems: 'center',
    gap: 8,
  },
  toggleLabel: { color: colors.textMuted, fontSize: 11, fontWeight: '700', letterSpacing: 1 },

  card: {
    backgroundColor: colors.bgElevated,
    borderRadius: radius.md,
    borderWidth: 1,
    borderColor: colors.border,
    padding: spacing.md,
    marginBottom: spacing.sm,
  },
  cardGreenBorder: { borderLeftWidth: 3, borderLeftColor: colors.green },
  cardError: { borderColor: colors.redBg },
  cardOk: { borderColor: colors.greenBg },

  cardTitleRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  cardTitleLeft: { flexDirection: 'row', alignItems: 'center', gap: 6 },
  cardTitle: { color: colors.text, fontSize: fontSize.md, fontWeight: '700' },
  sectionTag: { color: colors.textMuted, fontSize: 11, fontWeight: '700', letterSpacing: 1 },

  badge: { paddingHorizontal: 8, paddingVertical: 3, borderRadius: 4 },
  badgeGreen: { backgroundColor: colors.greenBg },
  badgeRed: { backgroundColor: colors.redBg },
  badgeText: { fontSize: 11, fontWeight: '800', letterSpacing: 0.5 },
  badgeGreenText: { color: colors.green },
  badgeRedText: { color: colors.red },

  metricRow3: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 6 },
  metric3: { flex: 1 },
  metricLabel: { color: colors.textFaint, fontSize: 10, fontWeight: '700', letterSpacing: 0.5, marginBottom: 2 },
  metricValue: { color: colors.text, fontSize: 18, fontWeight: '800' },
  metricUnit: { fontSize: 12, color: colors.textMuted },

  bigNum: { color: colors.text, fontSize: 22, fontWeight: '800', lineHeight: 30 },
  bigUnit: { fontSize: 13, color: colors.textMuted },

  systemIconGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: spacing.sm,
    marginTop: spacing.sm,
  },
  systemIconItem: {
    width: '31%',
    borderRadius: radius.md,
    borderWidth: 1,
    paddingVertical: spacing.sm,
    alignItems: 'center',
    justifyContent: 'center',
    gap: 4,
  },
  systemIconItemActive: {
    borderColor: colors.greenBorder,
    backgroundColor: colors.greenDeep,
  },
  systemIconItemInactive: {
    borderColor: colors.border,
    backgroundColor: colors.bgCard,
  },
  systemIconLabel: {
    color: colors.textMuted,
    fontSize: 11,
    fontWeight: '700',
    letterSpacing: 0.2,
  },
  systemIconLabelActive: {
    color: colors.green,
  },

  errorRow: { flexDirection: 'row', alignItems: 'center' },
  errorInfo: { marginLeft: spacing.sm },
  errorTitle: { color: colors.text, fontSize: 14, fontWeight: '700' },
  errorSub: { color: colors.textMuted, fontSize: 12, marginTop: 2 },
});
