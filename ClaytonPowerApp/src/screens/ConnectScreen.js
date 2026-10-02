import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  Platform,
  PermissionsAndroid,
  Linking,
} from 'react-native';
import StatusBar from '../components/StatusBar';
import CarbonIcon from '../components/CarbonIcon';
import { Button, Notice, ScreenTitle, Section } from '../components/Carbon';
import { colors, font, spacing, type } from '../utils/theme';
import bleService from '../services/bleService';

async function requestPermissions() {
  if (Platform.OS === 'android') {
    const granted = await PermissionsAndroid.requestMultiple([
      PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
      PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
    ]);
    return Object.values(granted).every(
      (v) => v === PermissionsAndroid.RESULTS.GRANTED
    );
  }
  return true;
}

// RSSI as four ascending bars (spec §10): lit bars in ink, the rest track.
function SignalBars({ rssi }) {
  const lit = rssi > -60 ? 4 : rssi > -70 ? 3 : rssi > -80 ? 2 : 1;
  return (
    <View style={styles.bars}>
      {[0, 1, 2, 3].map((i) => (
        <View
          key={i}
          style={[styles.bar, { height: 5 + i * 3, backgroundColor: i < lit ? colors.ink : colors.track }]}
        />
      ))}
    </View>
  );
}

export default function ConnectScreen() {
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState([]);
  const [connecting, setConnecting] = useState(null);
  const [connectError, setConnectError] = useState(null);
  const [connected, setConnected] = useState(bleService.isConnected);
  const [connectedDeviceName, setConnectedDeviceName] = useState(null);

  useEffect(() => {
    return bleService.onConnectionChange((c) => {
      setConnected(c);
      if (c) setConnectedDeviceName(bleService.device?.name || 'LPS BLE');
    });
  }, []);

  const startScan = async () => {
    const ok = await requestPermissions();
    if (!ok) return;
    setConnectError(null);
    setScanning(true);
    setDevices([]);
    const found = await bleService.scan(5000);
    setDevices(found);
    setScanning(false);
  };

  const connectDevice = async (deviceId) => {
    setConnecting(deviceId);
    setConnectError(null);
    const ok = await bleService.connect(deviceId);
    if (!ok) {
      setConnectError(
        'Could not connect. If the PIN was entered correctly, the display may have forgotten this '
        + 'phone: remove "Clayton Power" in Bluetooth settings and connect again.'
      );
    }
    setConnecting(null);
  };

  const isDeviceConnected = (id) => connected && bleService.device?.id === id;

  const renderDevice = ({ item, index }) => {
    const isConn = isDeviceConnected(item.id);
    return (
      <View style={[styles.deviceRow, index === devices.length - 1 && styles.deviceRowLast]}>
        <CarbonIcon name="bt" size={24} color={isConn ? colors.blue : colors.dim} />
        <View style={styles.deviceInfo}>
          <Text style={type.label} numberOfLines={1}>{item.name || 'Unknown device'}</Text>
          <View style={styles.signalRow}>
            <SignalBars rssi={item.rssi} />
            <Text style={type.small}>{item.rssi} dBm</Text>
          </View>
        </View>
        <Button
          compact
          label={isConn ? 'DISCONNECT' : 'CONNECT'}
          variant={isConn ? 'outline' : 'primary'}
          loading={connecting === item.id}
          disabled={connecting !== null && !isConn}
          onPress={() => (isConn ? bleService.disconnect() : connectDevice(item.id))}
        />
      </View>
    );
  };

  return (
    <View style={styles.screen}>
      <StatusBar />
      <ScreenTitle title="CONNECT" />

      <View style={styles.state}>
        <View style={[styles.stateRing, connected && styles.stateRingOn]}>
          <CarbonIcon name="bt" size={40} color={connected ? colors.blue : colors.dim} />
        </View>
        <View style={styles.stateCopy}>
          <Text style={[type.value, !connected && styles.dim]} numberOfLines={1}>
            {connected ? connectedDeviceName || 'LPS BLE' : 'No device'}
          </Text>
          <Text style={[styles.stateTag, connected && styles.stateTagOn]}>
            {connected ? 'CONNECTED' : 'NOT CONNECTED'}
          </Text>
        </View>
      </View>

      <Section
        title="AVAILABLE DEVICES"
        right={(
          <Button
            compact
            label={scanning ? 'SCANNING' : 'SCAN'}
            icon={scanning ? undefined : 'scan'}
            loading={scanning}
            onPress={startScan}
          />
        )}
        style={styles.flex}
      >
        {connecting !== null && (
          <Notice color={colors.blue} text="If Android asks for a PIN, enter the 6-digit code shown on the display." />
        )}
        {connectError && (
          <>
            <Notice text={connectError} />
            {Platform.OS === 'android' && (
              <Button
                compact
                label="BLUETOOTH SETTINGS"
                icon="bt"
                onPress={() => Linking.sendIntent('android.settings.BLUETOOTH_SETTINGS').catch(() => Linking.openSettings())}
                style={styles.settingsBtn}
              />
            )}
          </>
        )}

        {devices.length > 0 ? (
          <FlatList
            data={devices}
            keyExtractor={(item) => item.id}
            renderItem={renderDevice}
            style={styles.flex}
            contentContainerStyle={{ paddingBottom: spacing.md }}
          />
        ) : (
          <View style={styles.empty}>
            <CarbonIcon name="scan" size={40} color={colors.faint} />
            <Text style={[type.small, styles.emptyText]}>
              {scanning ? 'Searching for Clayton Power units…' : 'Tap SCAN to search for nearby units'}
            </Text>
          </View>
        )}
      </Section>
    </View>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: colors.bg },
  flex: { flex: 1 },
  dim: { color: colors.dim },

  state: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.md,
    paddingHorizontal: spacing.md,
    paddingVertical: spacing.lg,
    borderBottomWidth: 1,
    borderBottomColor: colors.line,
  },
  stateRing: {
    width: 76,
    height: 76,
    borderRadius: 38,
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: colors.panel,
    borderWidth: 1,
    borderColor: colors.edge,
  },
  stateRingOn: { borderColor: colors.blue },
  stateCopy: { flex: 1, minWidth: 0, gap: 4 },
  stateTag: { fontFamily: font.bold, fontSize: 13, letterSpacing: 2, color: colors.dim },
  stateTagOn: { color: colors.blue },

  deviceRow: {
    minHeight: 72,
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.md,
    borderBottomWidth: 1,
    borderBottomColor: colors.rowLine,
  },
  deviceRowLast: { borderBottomWidth: 0 },
  deviceInfo: { flex: 1, minWidth: 0 },
  signalRow: { flexDirection: 'row', alignItems: 'center', gap: 8, marginTop: 4 },
  bars: { flexDirection: 'row', alignItems: 'flex-end', gap: 2, height: 14 },
  bar: { width: 3 },

  settingsBtn: { alignSelf: 'flex-start', marginBottom: spacing.sm },
  empty: { alignItems: 'center', gap: spacing.md, paddingVertical: spacing.xl * 2 },
  emptyText: { textAlign: 'center' },
});
