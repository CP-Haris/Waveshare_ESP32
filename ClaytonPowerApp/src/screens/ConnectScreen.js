import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  FlatList,
  ActivityIndicator,
  Platform,
  PermissionsAndroid,
} from 'react-native';
import { MaterialIcons } from '@expo/vector-icons';
import { colors, spacing, fontSize, radius } from '../utils/theme';
import bleService from '../services/bleService';
import ScreenHeader from '../components/ScreenHeader';

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
      setConnectError('Pairing failed. Enter the BLE PIN shown on the display when Android asks for it.');
    }
    setConnecting(null);
  };

  const isDeviceConnected = (id) => connected && bleService.device?.id === id;

  const renderDevice = ({ item }) => {
    const isConn = isDeviceConnected(item.id);
    const signalStrong = item.rssi > -65;
    const signalMed = item.rssi > -78;
    const signalLabel = signalStrong ? 'Strong' : signalMed ? 'Medium' : 'Weak';
    const signalColor = signalStrong ? colors.green : signalMed ? colors.solar : colors.red;

    return (
      <View style={styles.deviceCard}>
        <View style={styles.deviceRow}>
          <View style={styles.deviceIconCircle}>
            <MaterialIcons name="bluetooth" size={22} color={isConn ? colors.accent : colors.textMuted} />
          </View>
          <View style={styles.deviceInfo}>
            <Text style={styles.deviceName}>{item.name || 'Unknown Device'}</Text>
            <View style={styles.signalRow}>
              <MaterialIcons
                name={signalStrong ? 'signal-cellular-alt' : signalMed ? 'signal-cellular-connected-no-internet-4-bar' : 'signal-cellular-0-bar'}
                size={14}
                color={signalColor}
              />
              <Text style={styles.signalText}>Signal: {signalLabel}, RSSI {item.rssi}dBm</Text>
            </View>
          </View>
        </View>
        <TouchableOpacity
          style={[styles.actionBtn, isConn ? styles.disconnectBtnStyle : styles.connectBtnStyle]}
          onPress={() => isConn ? bleService.disconnect() : connectDevice(item.id)}
          disabled={connecting !== null && !isConn}
        >
          {connecting === item.id ? (
            <ActivityIndicator color={colors.text} size="small" />
          ) : (
            <Text style={[styles.actionBtnText, !isConn && styles.connectText]}>
              {isConn ? 'DISCONNECT' : 'CONNECT'}
            </Text>
          )}
        </TouchableOpacity>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <ScreenHeader />

      {connected && (
        <View style={styles.systemReadyCard}>
          <View style={styles.btGreenRing}>
            <MaterialIcons name="bluetooth-connected" size={28} color={colors.green} />
          </View>
          <View style={styles.systemReadyInfo}>
            <Text style={styles.systemReadyTitle}>System Ready</Text>
            <View style={styles.connectedRow}>
              <View style={styles.greenDot} />
              <Text style={styles.connectedLabel}>
                Connected to {connectedDeviceName || 'LPS BLE'}
              </Text>
            </View>
          </View>
        </View>
      )}

      <View style={styles.sectionRow}>
        <Text style={styles.sectionLabel}>AVAILABLE DEVICES</Text>
        <TouchableOpacity onPress={startScan} disabled={scanning} hitSlop={{ top: 8, bottom: 8, left: 8, right: 8 }}>
          {scanning
            ? <ActivityIndicator size="small" color={colors.accent} />
            : <MaterialIcons name="radar" size={22} color={colors.textMuted} />
          }
        </TouchableOpacity>
      </View>
      <View style={styles.sectionDivider} />

      {connectError && (
        <View style={styles.errorCard}>
          <MaterialIcons name="lock" size={18} color={colors.red} />
          <Text style={styles.errorText}>{connectError}</Text>
        </View>
      )}

      {devices.length > 0 ? (
        <FlatList
          data={devices}
          keyExtractor={(item) => item.id}
          renderItem={renderDevice}
          style={styles.list}
          contentContainerStyle={{ paddingBottom: spacing.md }}
        />
      ) : (
        <View style={styles.emptyState}>
          <Text style={styles.emptyText}>
            {scanning ? 'Scanning for devices...' : 'Tap the scan icon above to search for devices'}
          </Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: colors.bg,
    paddingHorizontal: spacing.md,
    paddingTop: spacing.lg,
  },
  systemReadyCard: {
    backgroundColor: colors.greenBg,
    borderRadius: radius.lg,
    borderWidth: 1,
    borderColor: colors.greenBorder,
    padding: spacing.md,
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: spacing.lg,
  },
  btGreenRing: {
    width: 68,
    height: 68,
    borderRadius: 34,
    borderWidth: 2,
    borderColor: colors.green,
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: colors.greenDeep,
    marginRight: spacing.md,
  },
  systemReadyInfo: { flex: 1 },
  systemReadyTitle: {
    color: colors.text,
    fontSize: 20,
    fontWeight: '800',
  },
  connectedRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginTop: 4,
    gap: 6,
  },
  greenDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    backgroundColor: colors.green,
  },
  connectedLabel: {
    color: colors.textDim,
    fontSize: fontSize.sm,
  },
  sectionRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginBottom: 8,
  },
  sectionLabel: {
    color: colors.textMuted,
    fontSize: fontSize.xs,
    fontWeight: '700',
    letterSpacing: 1.2,
    textTransform: 'uppercase',
  },
  sectionDivider: {
    height: 1,
    backgroundColor: colors.borderSubtle,
    marginBottom: spacing.sm,
  },
  errorCard: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.sm,
    backgroundColor: colors.redBg,
    borderWidth: 1,
    borderColor: colors.red,
    borderRadius: radius.md,
    padding: spacing.md,
    marginBottom: spacing.md,
  },
  errorText: {
    flex: 1,
    color: colors.text,
    fontSize: fontSize.sm,
  },
  list: { flex: 1 },
  deviceCard: {
    backgroundColor: colors.bgElevated,
    borderRadius: radius.md,
    borderWidth: 1,
    borderColor: colors.border,
    marginBottom: spacing.sm,
    overflow: 'hidden',
  },
  deviceRow: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: spacing.md,
    gap: spacing.sm,
  },
  deviceIconCircle: {
    width: 44,
    height: 44,
    borderRadius: 22,
    backgroundColor: colors.bgInset,
    alignItems: 'center',
    justifyContent: 'center',
  },
  deviceInfo: { flex: 1 },
  deviceName: {
    color: colors.text,
    fontSize: fontSize.lg,
    fontWeight: '700',
  },
  signalRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 4,
    marginTop: 3,
  },
  signalText: {
    color: colors.textMuted,
    fontSize: fontSize.sm,
  },
  actionBtn: {
    marginHorizontal: spacing.md,
    marginBottom: spacing.md,
    borderRadius: radius.sm,
    paddingVertical: 14,
    alignItems: 'center',
    justifyContent: 'center',
  },
  connectBtnStyle: { backgroundColor: colors.accent },
  disconnectBtnStyle: {
    backgroundColor: 'transparent',
    borderWidth: 1,
    borderColor: colors.borderStrong,
  },
  actionBtnText: {
    color: colors.textDim,
    fontWeight: '800',
    fontSize: fontSize.sm,
    letterSpacing: 1,
  },
  connectText: { color: colors.text },
  emptyState: {
    alignItems: 'center',
    paddingVertical: spacing.xl,
  },
  emptyText: {
    color: colors.textFaint,
    fontSize: fontSize.sm,
    textAlign: 'center',
  },
});
