import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { colors } from '../utils/theme';

const SEGMENT_COUNT = 28;

export default function SocRing({ pct, size = 150 }) {
  const safePct = Math.max(0, Math.min(100, Number.isFinite(pct) ? pct : 0));
  const ringColor = safePct > 50 ? colors.green : safePct > 20 ? colors.solarWarm : colors.red;
  const activeSegments = Math.round((safePct / 100) * SEGMENT_COUNT);
  const innerSize = size - 42;

  return (
    <View style={[styles.outer, { width: size, height: size, borderRadius: size / 2 }]}>
      {Array.from({ length: SEGMENT_COUNT }).map((_, segmentIndex) => {
        const isActive = segmentIndex < activeSegments;
        return (
          <View
            key={segmentIndex}
            style={[
              styles.segment,
              {
                height: Math.max(8, size * 0.08),
                backgroundColor: isActive ? ringColor : colors.borderStrong,
                opacity: isActive ? 1 : 0.32,
                transform: [
                  { rotate: `${segmentIndex * (360 / SEGMENT_COUNT)}deg` },
                  { translateY: -(size / 2 - 8) },
                ],
              },
            ]}
          />
        );
      })}
      <View style={[styles.inner, { width: innerSize, height: innerSize, borderRadius: innerSize / 2 }]}>
        <Text style={styles.pct}>{safePct.toFixed(0)}%</Text>
        <Text style={styles.label}>SOC</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  outer: {
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: colors.bgCard,
    borderWidth: 1,
    borderColor: colors.border,
  },
  segment: {
    position: 'absolute',
    top: '50%',
    left: '50%',
    width: 4,
    marginLeft: -2,
    marginTop: -5,
    borderRadius: 2,
  },
  inner: {
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: colors.bg,
    borderWidth: 1,
    borderColor: colors.borderSubtle,
  },
  pct: { color: colors.text, fontSize: 42, fontWeight: '800', lineHeight: 44 },
  label: { color: colors.textMuted, fontSize: 12, fontWeight: '700' },
});
