import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  Pressable,
  ActivityIndicator,
  Platform,
  PermissionsAndroid,
  Linking,
} from 'react-native';
import StatusBar from '../components/StatusBar';
import CarbonIcon from '../components/CarbonIcon';
import { Button, Notice, ScreenTitle, Section } from '../components/Carbon';
import deviceSession from '../devices/deviceSession';
import { backgroundService } from '../services/backgroundService';
import { DEVICE_KIND } from '../ble/gattProfiles';
import { colors, font, spacing, type } from '../utils/theme';

// What the user is connecting to: a ClaytonDisplay (CAN gateway) or an LPS2
// through its own built-in Bluetooth.
const KIND_TAG = {
  [DEVICE_KIND.DISPLAY]: 'DISPLAY',
  [DEVICE_KIND.LPS2]: 'LPS 2',
};

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
  const [connected, setConnected] = useState(deviceSession.isConnected);
  const [connectedDeviceName, setConnectedDeviceName] = useState(null);

  useEffect(() => {
    return deviceSession.onConnectionChange((c) => {
      setConnected(c);
      if (c) setConnectedDeviceName(deviceSession.connectedName || 'Clayton Power');
    });
  }, []);

  const startScan = async () => {
    const ok = await requestPermissions();
    if (!ok) return;
    setConnectError(null);
    setScanning(true);
    setDevices([]);
    const found = await deviceSession.scan(5000);
    setDevices(found);
    setScanning(false);
  };

  const connectDevice = async (device) => {
    setConnecting(device.id);
    setConnectError(null);
    const ok = await deviceSession.connect(device);
    if (!ok) {
      setConnectError(
        'Could not connect. If the PIN was entered correctly, the unit may have forgotten this '
        + 'phone: remove "Clayton Power" in Bluetooth settings and connect again.'
      );
    }
    setConnecting(null);
  };

  const isDeviceConnected = (id) => connected && deviceSession.connectedDeviceId === id;

  // The whole row is the button: tap to connect (chevron + press highlight).
  // The connected one is marked with a blue edge, icon and check, and is
  // disconnected from the state panel above, never by a tap in the list.
  const renderDevice = ({ item, index }) => {
    const isConn = isDeviceConnected(item.id);
    const isConnecting = connecting === item.id;
    return (
      <Pressable
        onPress={() => connectDevice(item)}
        disabled={isConn || connecting !== null}
        accessibilityRole="button"
        accessibilityLabel={`Connect to ${item.name || 'device'}`}
        style={({ pressed }) => [
          styles.deviceRow,
          index === devices.length - 1 && styles.deviceRowLast,
          isConn && styles.deviceRowOn,
          pressed && styles.deviceRowPressed,
          connecting !== null && !isConnecting && styles.deviceRowIdle,
        ]}
      >
        <CarbonIcon name="bt" size={24} color={isConn ? colors.blue : colors.dim} />
        <View style={styles.deviceInfo}>
          <View style={styles.nameRow}>
            <Text style={styles.kindTag}>{KIND_TAG[item.kind]}</Text>
            <Text style={[type.label, styles.flex]} numberOfLines={1}>{item.name || 'Unknown device'}</Text>
          </View>
          <View style={styles.signalRow}>
            <SignalBars rssi={item.rssi} />
            <Text style={type.small}>{item.rssi} dBm</Text>
            {/* An LPS2 advertises its SoC and serial before we connect. */}
            {Number.isFinite(item.soc) && <Text style={type.small}>· {item.soc} %</Text>}
            {!!item.serial && <Text style={type.small}>· {item.serial}</Text>}
          </View>
        </View>
        {isConnecting && <ActivityIndicator size="small" color={colors.blue} />}
        {isConn && <CarbonIcon name="check" size={22} color={colors.blue} />}
        {!isConnecting && !isConn && <CarbonIcon name="chev" size={20} color={colors.dim} />}
      </Pressable>
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
        {connected && <Button compact label="DISCONNECT" onPress={() => backgroundService.disconnectByUser()} />}
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
          <Notice color={colors.blue} text="If Android asks for a PIN, enter the 6-digit code shown on the unit's screen." />
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
    paddingHorizontal: spacing.sm,
    borderBottomWidth: 1,
    borderBottomColor: colors.rowLine,
  },
  deviceRowLast: { borderBottomWidth: 0 },
  deviceRowOn: { borderLeftWidth: 3, borderLeftColor: colors.blue, paddingLeft: spacing.sm + 2 },
  deviceRowPressed: { backgroundColor: colors.sheet },
  deviceRowIdle: { opacity: 0.4 },
  deviceInfo: { flex: 1, minWidth: 0 },
  nameRow: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  kindTag: {
    fontFamily: font.bold,
    fontSize: 11,
    letterSpacing: 1.2,
    color: colors.blue,
    borderWidth: 1,
    borderColor: colors.edge,
    paddingHorizontal: 5,
    paddingVertical: 1,
  },
  signalRow: { flexDirection: 'row', alignItems: 'center', gap: 8, marginTop: 4 },
  bars: { flexDirection: 'row', alignItems: 'flex-end', gap: 2, height: 14 },
  bar: { width: 3 },

  settingsBtn: { alignSelf: 'flex-start', marginBottom: spacing.sm },
  empty: { alignItems: 'center', gap: spacing.md, paddingVertical: spacing.xl * 2 },
  emptyText: { textAlign: 'center' },
});
