import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { colors, radius, spacing } from '../utils/theme';

const MIN_V = 2.8;
const MAX_V = 3.65;

export default function CellBars({ cells }) {
  if (!cells || cells.length === 0) return null;
  return (
    <View style={styles.wrap}>
      <Text style={styles.title}>CELL VOLTAGES</Text>
      <View style={styles.row}>
        {cells.map((v, i) => {
          const pct = Math.max(0, Math.min(1, (v - MIN_V) / (MAX_V - MIN_V)));
          return (
            <View key={i} style={styles.col}>
              <Text style={styles.val}>{v.toFixed(2)}V</Text>
              <View style={styles.barBg}>
                <View style={[styles.barFill, { height: `${Math.round(pct * 100)}%` }]} />
              </View>
              <Text style={styles.cellLabel}>C{i + 1}</Text>
            </View>
          );
        })}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { marginTop: spacing.sm, backgroundColor: colors.bgCard, borderRadius: radius.sm, padding: spacing.sm },
  title: { color: colors.textMuted, fontSize: 10, fontWeight: '700', letterSpacing: 1, marginBottom: 10 },
  row: { flexDirection: 'row', justifyContent: 'space-around', alignItems: 'flex-end', height: 80 },
  col: { alignItems: 'center', flex: 1 },
  val: { color: colors.textLight, fontSize: 10, marginBottom: 4 },
  barBg: { width: 36, height: 60, backgroundColor: '#2a2a3a', borderRadius: 4, overflow: 'hidden', justifyContent: 'flex-end' },
  barFill: { backgroundColor: colors.accent, borderRadius: 4, width: '100%' },
  cellLabel: { color: colors.textMuted, fontSize: 10, marginTop: 4 },
});
