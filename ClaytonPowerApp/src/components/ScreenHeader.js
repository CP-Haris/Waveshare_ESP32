import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { colors, spacing } from '../utils/theme';

export default function ScreenHeader() {
  return (
    <>
      <View style={styles.row}>
        <Text style={styles.title}>Clayton Power</Text>
      </View>
      <View style={styles.divider} />
    </>
  );
}

const styles = StyleSheet.create({
  row: {
    alignItems: 'center',
    justifyContent: 'center',
    paddingBottom: spacing.sm,
  },
  title: {
    color: colors.text,
    fontWeight: '800',
    fontSize: 18,
    letterSpacing: 0.2,
  },
  divider: {
    height: 1,
    backgroundColor: colors.borderSubtle,
    marginBottom: spacing.md,
  },
});
