import React, { useEffect, useState } from 'react';
import { Modal, ScrollView, StyleSheet, Text, TouchableOpacity, View } from 'react-native';
import { MaterialIcons } from '@expo/vector-icons';
import { colors, fontSize, spacing } from '../utils/theme';
import bleService from '../services/bleService';
import canGatewayService from '../services/canGatewayService';

const DEV_LPS = 1;
const DEV_BMS = 2;

function unitKind(unit) {
  const partNumber = String(unit?.partNumber || '').toUpperCase();
  if (partNumber.startsWith('CB')) return 'Battery';
  if (partNumber.startsWith('CL')) return 'LPS';
  if (unit?.type === DEV_BMS) return 'Battery';
  if (unit?.type === DEV_LPS) return 'LPS';
  return 'Unit';
}

function unitLabel(unit) {
  if (!unit) return 'Select unit';
  const primary = String(unit.partNumber || '').trim() || unitKind(unit);
  const serialSuffix = String(unit.serial || '').replace(/\D/g, '').slice(-4);
  return serialSuffix ? `${primary} - ${serialSuffix}` : primary;
}

export default function UnitSwitcher() {
  const [connected, setConnected] = useState(bleService.isConnected);
  const [units, setUnits] = useState(() => canGatewayService.getUnits());
  const [activeUnit, setActiveUnit] = useState(() => canGatewayService.getActiveUnitInfo());
  const [modalVisible, setModalVisible] = useState(false);
  const [locked, setLocked] = useState(bleService.commandLockOwner === 'firmware-update');

  const refreshFromService = () => {
    setUnits(canGatewayService.getUnits());
    setActiveUnit(canGatewayService.getActiveUnitInfo());
  };

  useEffect(() => {
    refreshFromService();

    const unsubConn = bleService.onConnectionChange((nextConnected) => {
      setConnected(nextConnected);
      if (!nextConnected) {
        setModalVisible(false);
        setUnits([]);
        setActiveUnit(null);
        return;
      }
      refreshFromService();
      canGatewayService.requestUnits();
    });

    const unsubGateway = canGatewayService.onNotification((message) => {
      if (message.type === 'unitInfo' || message.type === 'dashboard' || message.type === 'errors') {
        refreshFromService();
      }
    });

    const lockTimer = setInterval(() => {
      const nextLocked = bleService.commandLockOwner === 'firmware-update';
      setLocked(nextLocked);
      if (nextLocked) setModalVisible(false);
    }, 600);

    return () => {
      unsubConn();
      unsubGateway();
      clearInterval(lockTimer);
    };
  }, []);

  if (!connected || units.length === 0) return null;

  const disabled = locked;

  const selectUnit = (unit) => {
    if (disabled) return;
    canGatewayService.selectUnit(unit.index);
    setModalVisible(false);
  };

  return (
    <>
      <TouchableOpacity
        style={[styles.trigger, disabled && styles.triggerDisabled]}
        onPress={() => setModalVisible(true)}
        disabled={disabled}
        activeOpacity={0.85}
      >
        <MaterialIcons name={disabled ? 'lock' : 'swap-vert'} size={16} color={disabled ? colors.textGhost : colors.accent} />
        <Text style={[styles.triggerText, disabled && styles.triggerTextDisabled]} numberOfLines={1}>
          {unitLabel(activeUnit)}
        </Text>
        <MaterialIcons name="expand-more" size={16} color={colors.textMuted} />
      </TouchableOpacity>

      <Modal
        visible={modalVisible}
        transparent
        animationType="fade"
        onRequestClose={() => setModalVisible(false)}
      >
        <View style={styles.modalBackdrop}>
          <TouchableOpacity style={styles.backdropTapArea} onPress={() => setModalVisible(false)} activeOpacity={1} />
          <View style={styles.sheet}>
            <View style={styles.sheetHeader}>
              <View>
                <Text style={styles.sheetTitle}>Select unit</Text>
                <Text style={styles.sheetMeta}>{units.length} discovered</Text>
              </View>
              <View style={styles.headerActions}>
                <TouchableOpacity style={styles.iconButton} onPress={() => canGatewayService.requestUnits()} activeOpacity={0.85}>
                  <MaterialIcons name="refresh" size={20} color={colors.accent} />
                </TouchableOpacity>
                <TouchableOpacity style={styles.iconButton} onPress={() => setModalVisible(false)} activeOpacity={0.85}>
                  <MaterialIcons name="close" size={20} color={colors.text} />
                </TouchableOpacity>
              </View>
            </View>

            <ScrollView style={styles.unitList} contentContainerStyle={styles.unitListContent}>
              {units.map((unit) => {
                const isActive = unit.index === activeUnit?.index;
                const hasErrors = unit.errorCount > 0;
                return (
                  <TouchableOpacity
                    key={unit.index}
                    style={[styles.unitRow, isActive && styles.unitRowActive]}
                    onPress={() => selectUnit(unit)}
                    activeOpacity={0.85}
                  >
                    <View style={styles.unitIconWrap}>
                      <MaterialIcons name={unitKind(unit) === 'Battery' ? 'battery-full' : 'memory'} size={19} color={isActive ? colors.accent : colors.textMuted} />
                    </View>
                    <View style={styles.unitCopy}>
                      <Text style={styles.unitTitle}>{unitKind(unit)}</Text>
                      <Text style={styles.unitSub} numberOfLines={1}>{unit.partNumber || '-'}</Text>
                      <Text style={styles.unitSerial} numberOfLines={1}>{unit.serial || '-'}</Text>
                    </View>
                    {hasErrors && (
                      <View style={styles.errorBadge}>
                        <Text style={styles.errorBadgeText}>{unit.errorCount}</Text>
                      </View>
                    )}
                    <MaterialIcons
                      name={isActive ? 'check-circle' : 'radio-button-unchecked'}
                      size={21}
                      color={isActive ? colors.green : colors.textGhost}
                    />
                  </TouchableOpacity>
                );
              })}
            </ScrollView>
          </View>
        </View>
      </Modal>
    </>
  );
}

const styles = StyleSheet.create({
  trigger: {
    maxWidth: 178,
    minHeight: 34,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: colors.border,
    backgroundColor: colors.bgElevated,
    paddingHorizontal: spacing.sm,
    flexDirection: 'row',
    alignItems: 'center',
    gap: 5,
  },
  triggerDisabled: { opacity: 0.55 },
  triggerText: { color: colors.text, fontSize: fontSize.sm, fontWeight: '800', flexShrink: 1 },
  triggerTextDisabled: { color: colors.textMuted },
  modalBackdrop: { flex: 1, backgroundColor: 'rgba(0,0,0,0.68)', justifyContent: 'flex-end' },
  backdropTapArea: { ...StyleSheet.absoluteFillObject },
  sheet: {
    maxHeight: '72%',
    backgroundColor: colors.bgElevated,
    borderTopLeftRadius: 8,
    borderTopRightRadius: 8,
    borderWidth: 1,
    borderColor: colors.border,
    padding: spacing.md,
  },
  sheetHeader: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', gap: spacing.md, marginBottom: spacing.md },
  sheetTitle: { color: colors.text, fontSize: fontSize.lg, fontWeight: '800' },
  sheetMeta: { color: colors.textMuted, fontSize: fontSize.xs, fontWeight: '700', marginTop: 2 },
  headerActions: { flexDirection: 'row', alignItems: 'center', gap: spacing.sm },
  iconButton: { width: 38, height: 38, borderRadius: 8, alignItems: 'center', justifyContent: 'center', backgroundColor: colors.bgCard, borderWidth: 1, borderColor: colors.borderSubtle },
  unitList: { maxHeight: 430 },
  unitListContent: { paddingBottom: spacing.sm },
  unitRow: {
    minHeight: 76,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: colors.borderSubtle,
    backgroundColor: colors.bgCard,
    padding: spacing.sm,
    marginBottom: spacing.sm,
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.sm,
  },
  unitRowActive: { borderColor: colors.accent, backgroundColor: colors.bgInset },
  unitIconWrap: { width: 34, height: 34, borderRadius: 8, alignItems: 'center', justifyContent: 'center', backgroundColor: colors.bgElevated, borderWidth: 1, borderColor: colors.borderSubtle },
  unitCopy: { flex: 1, minWidth: 0 },
  unitTitle: { color: colors.text, fontSize: fontSize.md, fontWeight: '800' },
  unitSub: { color: colors.textMuted, fontSize: fontSize.sm, marginTop: 2 },
  unitSerial: { color: colors.textFaint, fontSize: fontSize.xs, marginTop: 2, fontWeight: '700' },
  errorBadge: { minWidth: 24, height: 24, borderRadius: 12, backgroundColor: colors.redBg, alignItems: 'center', justifyContent: 'center', paddingHorizontal: 7, borderWidth: 1, borderColor: colors.red },
  errorBadgeText: { color: colors.redSoft, fontSize: fontSize.xs, fontWeight: '800' },
});