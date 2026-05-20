import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { colors } from '../utils/theme';

export default function StatusRow({ label, value, ok }) {
  const bg = ok ? colors.greenBg : value === 'IDLE' ? colors.bgElevated : colors.redDeep;
  const textColor = ok ? colors.green : value === 'IDLE' ? colors.textMuted : colors.red;
  return (
    <View style={styles.row}>
      <Text style={styles.label}>{label}</Text>
      <View style={[styles.badge, { backgroundColor: bg }]}>
        <Text style={[styles.value, { color: textColor }]}>{value}</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 10,
    borderBottomWidth: 1,
    borderBottomColor: colors.borderSubtle,
  },
  label: { color: colors.textLight, fontSize: 14 },
  badge: { paddingHorizontal: 10, paddingVertical: 4, borderRadius: 4 },
  value: { fontWeight: '700', fontSize: 12, letterSpacing: 0.5 },
});
