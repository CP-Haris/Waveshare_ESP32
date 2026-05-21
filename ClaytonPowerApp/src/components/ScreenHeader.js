import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { colors, spacing } from '../utils/theme';
import UnitSwitcher from './UnitSwitcher';

export default function ScreenHeader() {
  return (
    <>
      <View style={styles.row}>
        <Text style={styles.title}>Clayton Power</Text>
        <UnitSwitcher />
      </View>
      <View style={styles.divider} />
    </>
  );
}

const styles = StyleSheet.create({
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    gap: spacing.sm,
    paddingBottom: spacing.sm,
  },
  title: {
    color: colors.text,
    fontWeight: '800',
    fontSize: 18,
    letterSpacing: 0.2,
    flexShrink: 1,
  },
  divider: {
    height: 1,
    backgroundColor: colors.borderSubtle,
    marginBottom: spacing.md,
  },
});
