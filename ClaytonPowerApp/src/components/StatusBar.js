import React, { useEffect, useState } from 'react';
import { Pressable, StyleSheet, Text, View } from 'react-native';
import { useSafeAreaInsets } from 'react-native-safe-area-context';
import CarbonIcon from './CarbonIcon';
import UnitSwitcher from './UnitSwitcher';
import { openErrorList, worstColor } from './ErrorCenter';
import { colors, font, spacing, type } from '../utils/theme';
import { activeErrorDefinitions } from '../utils/errorCodes';
import deviceSession from '../devices/deviceSession';

function clockText() {
  const d = new Date();
  return `${String(d.getHours()).padStart(2, '0')}:${String(d.getMinutes()).padStart(2, '0')}`;
}

/** Shared 48 dp status line (spec §5): error badge, BLE, unit chip, clock. */
export default function StatusBar() {
  const insets = useSafeAreaInsets();
  const [connected, setConnected] = useState(deviceSession.isConnected);
  const [defs, setDefs] = useState([]);
  const [clock, setClock] = useState(clockText);

  useEffect(() => {
    const unsubConn = deviceSession.onConnectionChange((c) => {
      setConnected(c);
      if (!c) setDefs([]);
    });
    const unsub = deviceSession.onNotification((msg) => {
      if (msg.type === 'dashboard') setDefs(activeErrorDefinitions(msg.data.errorCodes));
      else if (msg.type === 'errors') setDefs(activeErrorDefinitions(msg.data));
    });
    const timer = setInterval(() => setClock(clockText()), 10000);
    return () => {
      unsubConn();
      unsub();
      clearInterval(timer);
    };
  }, []);

  const errColor = worstColor(defs);

  return (
    <View style={[styles.bar, { paddingTop: insets.top }]}>
      <View style={styles.inner}>
        <Pressable onPress={openErrorList} hitSlop={8} style={styles.badge}>
          <CarbonIcon name="warn" size={20} color={errColor} />
          {defs.length > 0 && <Text style={[styles.count, { color: errColor }]}>{defs.length}</Text>}
        </Pressable>
        <CarbonIcon name="bt" size={20} color={connected ? colors.blue : colors.faint} />
        <View style={styles.spacer} />
        <UnitSwitcher />
        <Text style={[type.value, styles.clock]}>{clock}</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  bar: { backgroundColor: colors.bg, borderBottomWidth: 1, borderBottomColor: colors.line },
  inner: { height: 48, flexDirection: 'row', alignItems: 'center', gap: spacing.md, paddingHorizontal: spacing.md },
  badge: { flexDirection: 'row', alignItems: 'center', gap: 5, minWidth: 44, minHeight: 44 },
  count: { fontFamily: font.bold, fontSize: 16, fontVariant: ['tabular-nums'] },
  spacer: { flex: 1 },
  clock: { fontSize: 17 },
});
