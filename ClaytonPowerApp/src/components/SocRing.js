import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { colors } from '../utils/theme';

export default function SocRing({ pct }) {
  const ringColor = pct > 50 ? colors.green : pct > 20 ? colors.solarWarm : colors.red;
  return (
    <View style={styles.outer}>
      <View style={[styles.ring, { borderColor: ringColor }]}>
        <Text style={styles.pct}>{pct.toFixed(0)}%</Text>
        <Text style={styles.label}>SOC</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  outer: { alignItems: 'center', justifyContent: 'center', padding: 8 },
  ring: {
    width: 150,
    height: 150,
    borderRadius: 75,
    borderWidth: 10,
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: colors.greenDeep,
  },
  pct: { color: colors.text, fontSize: 42, fontWeight: '800', lineHeight: 44 },
  label: { color: colors.textMuted, fontSize: 12, letterSpacing: 2, textTransform: 'uppercase' },
});
