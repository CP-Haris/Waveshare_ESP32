import React from 'react';
import { ActivityIndicator, Modal, Pressable, StyleSheet, Text, View } from 'react-native';
import CarbonIcon from './CarbonIcon';
import { colors, spacing, type } from '../utils/theme';

// Shared Carbon Blue primitives (spec §5, §8): square geometry, hairlines,
// no cards.

export function ScreenTitle({ title, onBack, right }) {
  return (
    <View style={styles.titleBar}>
      {onBack && (
        <Pressable onPress={onBack} style={({ pressed }) => [styles.back, pressed && styles.pressed]} hitSlop={4}>
          <CarbonIcon name="back" size={24} />
        </Pressable>
      )}
      <Text style={[type.title, styles.titleText]} numberOfLines={1}>{title}</Text>
      {right}
    </View>
  );
}

export function Section({ title, right, children, style }) {
  return (
    <View style={[styles.section, style]}>
      {(title || right) && (
        <View style={styles.sectionHead}>
          <Text style={type.section}>{title}</Text>
          {right}
        </View>
      )}
      {children}
    </View>
  );
}

/**
 * A 64 dp row. kind 'status' = read-only (dim label, ink value);
 * kind 'setting' = editable (ink label, blue value, chevron).
 */
export function Row({ label, value, kind = 'status', onPress, icon, iconColor, tag, tagColor, last, right, height = 64 }) {
  const editable = kind === 'setting';
  const content = (
    <>
      {icon && <CarbonIcon name={icon} size={26} color={iconColor || colors.blue} />}
      <View style={styles.rowCopy}>
        <Text style={[type.label, kind === 'status' && styles.dimLabel]} numberOfLines={1}>{label}</Text>
        {!!tag && <Text style={[type.micro, styles.tag, { color: tagColor || colors.dim }]}>{tag}</Text>}
      </View>
      {value != null && (
        <Text style={[type.value, editable && styles.blueValue]} numberOfLines={1}>{value}</Text>
      )}
      {right}
      {(editable || kind === 'nav') && <CarbonIcon name="chev" size={20} color={colors.dim} />}
    </>
  );

  const rowStyle = [styles.row, { minHeight: height }, last && styles.rowLast];
  if (!onPress) return <View style={rowStyle}>{content}</View>;
  return (
    <Pressable onPress={onPress} style={({ pressed }) => [...rowStyle, pressed && styles.rowPressed]}>
      {content}
    </Pressable>
  );
}

export function Button({ label, onPress, variant = 'outline', disabled, icon, loading, style, compact }) {
  const primary = variant === 'primary';
  const textColor = primary ? colors.onBlue : variant === 'danger' ? colors.red : colors.ink;
  return (
    <Pressable
      onPress={onPress}
      disabled={disabled || loading}
      style={({ pressed }) => [
        styles.button,
        compact && styles.buttonCompact,
        primary ? styles.buttonPrimary : styles.buttonOutline,
        pressed && (primary ? styles.buttonPrimaryPressed : styles.pressed),
        (disabled || loading) && styles.disabled,
        style,
      ]}
    >
      {loading
        ? <ActivityIndicator size="small" color={textColor} />
        : icon && <CarbonIcon name={icon} size={18} color={textColor} />}
      <Text style={[type.button, compact && styles.buttonTextCompact, { color: textColor }]}>{label}</Text>
    </Pressable>
  );
}

export function Sheet({ visible, onClose, title, right, children }) {
  return (
    <Modal visible={visible} transparent animationType="fade" onRequestClose={onClose} statusBarTranslucent>
      <View style={styles.scrim}>
        <Pressable style={StyleSheet.absoluteFill} onPress={onClose} />
        <View style={styles.sheet}>
          {(title || right) && (
            <View style={styles.sheetHead}>
              <Text style={[type.section, styles.sheetTitle]} numberOfLines={1}>{title}</Text>
              {right}
            </View>
          )}
          {children}
        </View>
      </View>
    </Modal>
  );
}

export function IconButton({ icon, onPress, color = colors.ink, disabled }) {
  return (
    <Pressable
      onPress={onPress}
      disabled={disabled}
      hitSlop={6}
      style={({ pressed }) => [styles.iconButton, pressed && styles.pressed, disabled && styles.disabled]}
    >
      <CarbonIcon name={icon} size={20} color={color} />
    </Pressable>
  );
}

/** On/off switch: square plate, knob slides right and turns the plate blue. */
export function Toggle({ value, onValueChange, disabled }) {
  return (
    <Pressable
      onPress={() => onValueChange(!value)}
      disabled={disabled}
      hitSlop={8}
      accessibilityRole="switch"
      accessibilityState={{ checked: !!value, disabled: !!disabled }}
      style={[styles.toggle, value && styles.toggleOn, disabled && styles.disabled]}
    >
      <View style={[styles.toggleKnob, value && styles.toggleKnobOn]} />
    </Pressable>
  );
}

export function Notice({ text, color = colors.red }) {
  return (
    <View style={[styles.notice, { borderLeftColor: color }]}>
      <Text style={type.body}>{text}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  titleBar: {
    height: 56,
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: spacing.md,
    gap: spacing.md,
    borderBottomWidth: 1,
    borderBottomColor: colors.line,
  },
  back: {
    width: 56,
    height: 56,
    marginLeft: -spacing.md,
    alignItems: 'center',
    justifyContent: 'center',
    borderRightWidth: 1,
    borderRightColor: colors.line,
  },
  titleText: { flex: 1 },

  section: { paddingHorizontal: spacing.md, paddingTop: spacing.md },
  sectionHead: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', minHeight: 32, marginBottom: 2 },

  row: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.md,
    borderBottomWidth: 1,
    borderBottomColor: colors.rowLine,
  },
  rowLast: { borderBottomWidth: 0 },
  rowPressed: { backgroundColor: colors.sheet },
  rowCopy: { flex: 1, minWidth: 0 },
  dimLabel: { color: colors.dim },
  blueValue: { color: colors.blue },
  tag: { marginTop: 2 },
  pressed: { backgroundColor: colors.panelPressed },

  button: {
    minHeight: 52,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: spacing.sm,
    paddingHorizontal: spacing.lg,
    borderWidth: 1,
  },
  buttonCompact: { minHeight: 44, paddingHorizontal: spacing.md },
  buttonTextCompact: { fontSize: 13 },
  buttonPrimary: { backgroundColor: colors.blue, borderColor: colors.blue },
  buttonPrimaryPressed: { opacity: 0.85 },
  buttonOutline: { borderColor: colors.edge, backgroundColor: colors.panel },
  disabled: { opacity: 0.4 },

  scrim: { flex: 1, backgroundColor: colors.scrim, justifyContent: 'flex-end' },
  sheet: {
    maxHeight: '85%',
    backgroundColor: colors.sheet,
    borderTopWidth: 1,
    borderTopColor: colors.edge,
    paddingHorizontal: spacing.lg,
    paddingTop: spacing.md,
    paddingBottom: spacing.lg,
  },
  sheetHead: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', minHeight: 44, gap: spacing.md },
  sheetTitle: { fontSize: 15, flex: 1 },

  iconButton: {
    width: 44,
    height: 44,
    alignItems: 'center',
    justifyContent: 'center',
    borderWidth: 1,
    borderColor: colors.edge,
    backgroundColor: colors.panel,
  },

  toggle: {
    width: 52,
    height: 30,
    padding: 3,
    justifyContent: 'center',
    backgroundColor: colors.panel,
    borderWidth: 1,
    borderColor: colors.edge,
  },
  toggleOn: { backgroundColor: colors.blue, borderColor: colors.blue },
  toggleKnob: { width: 22, height: 22, backgroundColor: colors.dim },
  toggleKnobOn: { alignSelf: 'flex-end', backgroundColor: colors.ink },

  notice: {
    borderLeftWidth: 4,
    backgroundColor: colors.panel,
    paddingVertical: spacing.sm + 4,
    paddingHorizontal: spacing.md,
    marginVertical: spacing.sm,
  },
});
