import React, { useEffect, useState } from 'react';
import { Pressable, ScrollView, StyleSheet, Text, View } from 'react-native';
import CarbonIcon from './CarbonIcon';
import { IconButton, Sheet } from './Carbon';
import { colors, font, spacing, type } from '../utils/theme';
import { unitFamily } from '../utils/units';
import deviceSession from '../devices/deviceSession';

function unitTag(unit) {
  const family = unitFamily(unit || {});
  if (family === 'bms') return 'BMS';
  if (family === 'lps') return 'LPS';
  return 'UNIT';
}

function serialSuffix(unit) {
  return String(unit?.serial || '').replace(/\D/g, '').slice(-4);
}

/** Unit chip + picker sheet (spec §9). */
export default function UnitSwitcher() {
  const [connected, setConnected] = useState(deviceSession.isConnected);
  const [units, setUnits] = useState(() => deviceSession.getUnits());
  const [activeUnit, setActiveUnit] = useState(() => deviceSession.getActiveUnitInfo());
  const [modalVisible, setModalVisible] = useState(false);
  const [locked, setLocked] = useState(deviceSession.isLocked);

  const refreshFromService = () => {
    setUnits(deviceSession.getUnits());
    setActiveUnit(deviceSession.getActiveUnitInfo());
  };

  useEffect(() => {
    refreshFromService();

    const unsubConn = deviceSession.onConnectionChange((nextConnected) => {
      setConnected(nextConnected);
      if (!nextConnected) {
        setModalVisible(false);
        setUnits([]);
        setActiveUnit(null);
        return;
      }
      refreshFromService();
      deviceSession.requestUnits();
    });

    const unsubGateway = deviceSession.onNotification((message) => {
      if (message.type === 'unitInfo' || message.type === 'dashboard' || message.type === 'errors') {
        refreshFromService();
      }
    });

    const lockTimer = setInterval(() => {
      const nextLocked = deviceSession.isLocked;
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

  const selectUnit = (unit) => {
    if (locked) return;
    deviceSession.selectUnit(unit.index);
    setModalVisible(false);
  };

  const suffix = serialSuffix(activeUnit);
  // Only a ClaytonDisplay sees several units; an LPS2 link is always one.
  const pickable = deviceSession.capabilities.multiUnit;

  return (
    <>
      <Pressable
        style={({ pressed }) => [styles.chip, pressed && styles.chipPressed, locked && styles.locked]}
        onPress={() => setModalVisible(true)}
        disabled={locked || !pickable}
        hitSlop={4}
      >
        <Text style={styles.chipTag}>{activeUnit ? unitTag(activeUnit) : 'UNIT'}</Text>
        {!!suffix && <Text style={styles.chipText}>{suffix}</Text>}
        {locked && <CarbonIcon name="lock" size={14} color={colors.dim} strokeWidth={2} />}
        {!locked && pickable && <CarbonIcon name="chevDown" size={14} color={colors.dim} strokeWidth={2} />}
      </Pressable>

      <Sheet
        visible={modalVisible}
        onClose={() => setModalVisible(false)}
        title="SELECT UNIT"
        right={(
          <View style={styles.headActions}>
            <IconButton icon="refresh" color={colors.blue} onPress={() => deviceSession.requestUnits()} />
            <IconButton icon="close" onPress={() => setModalVisible(false)} />
          </View>
        )}
      >
        <ScrollView style={styles.list}>
          {units.map((unit, i) => {
            const isActive = unit.index === activeUnit?.index;
            return (
              <Pressable
                key={unit.index}
                style={({ pressed }) => [styles.row, isActive && styles.rowActive, pressed && styles.chipPressed, i === units.length - 1 && styles.rowLast]}
                onPress={() => selectUnit(unit)}
              >
                <View style={[styles.radio, isActive && styles.radioOn]}>
                  {isActive && <View style={styles.radioDot} />}
                </View>
                <Text style={styles.rowTag}>{unitTag(unit)}</Text>
                <View style={styles.rowCopy}>
                  <Text style={type.label} numberOfLines={1}>{unit.partNumber || '—'}</Text>
                  <Text style={type.small} numberOfLines={1}>{unit.serial || '—'}</Text>
                </View>
                {unit.errorCount > 0 && (
                  <View style={styles.err}>
                    <CarbonIcon name="warn" size={16} color={colors.red} />
                    <Text style={styles.errText}>{unit.errorCount}</Text>
                  </View>
                )}
              </Pressable>
            );
          })}
        </ScrollView>
      </Sheet>
    </>
  );
}

const styles = StyleSheet.create({
  chip: {
    height: 36,
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    paddingHorizontal: 12,
    backgroundColor: colors.panel,
    borderWidth: 1,
    borderColor: colors.edge,
  },
  chipPressed: { backgroundColor: colors.panelPressed },
  locked: { opacity: 0.5 },
  chipTag: { fontFamily: font.bold, fontSize: 15, letterSpacing: 1.2, color: colors.blue },
  chipText: { fontFamily: font.bold, fontSize: 15, letterSpacing: 1.2, color: colors.ink, fontVariant: ['tabular-nums'] },

  headActions: { flexDirection: 'row', gap: spacing.sm },
  list: { maxHeight: 440, marginTop: spacing.sm },
  row: {
    minHeight: 68,
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.md,
    paddingHorizontal: spacing.sm,
    borderBottomWidth: 1,
    borderBottomColor: colors.rowLine,
  },
  rowActive: { backgroundColor: colors.panel },
  rowLast: { borderBottomWidth: 0 },
  radio: { width: 22, height: 22, borderRadius: 11, borderWidth: 2, borderColor: colors.dim, alignItems: 'center', justifyContent: 'center' },
  radioOn: { borderColor: colors.blue },
  radioDot: { width: 10, height: 10, borderRadius: 5, backgroundColor: colors.blue },
  rowTag: {
    fontFamily: font.bold,
    fontSize: 11,
    letterSpacing: 1.2,
    color: colors.blue,
    borderWidth: 1,
    borderColor: colors.edge,
    paddingHorizontal: 5,
    paddingVertical: 1,
  },
  rowCopy: { flex: 1, minWidth: 0 },
  err: { flexDirection: 'row', alignItems: 'center', gap: 4 },
  errText: { fontFamily: font.bold, fontSize: 15, color: colors.red },
});
