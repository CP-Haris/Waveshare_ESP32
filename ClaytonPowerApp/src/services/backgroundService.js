import { AppState } from 'react-native';
import deviceSession from '../devices/deviceSession';
import { startForegroundService, stopForegroundService } from '../../modules/ble-foreground';
import { prefs } from './prefs';
import { requestNotificationPermission, setupNotifications } from './notifier';
import { setAlertsEnabled, startAlertMonitor } from './alertMonitor';

// Remembers the last connected unit and reconnects to it when it comes in
// range. With "Background notifications" on (Settings), an Android foreground
// service keeps that link alive while the app is closed, and alertMonitor
// turns what the unit reports into notifications.
//
// Reconnecting runs while the app is open, or always when background is on.
// Disconnecting by hand stops it until the user connects again.

const RETRY_MS = 10000;

let enabled = false;
let lastDevice = null; // { id, name, kind }
let userDisconnected = false;
let retryTimer = null;
const listeners = new Set();

const emit = () => listeners.forEach((fn) => fn({ enabled, lastDevice }));

function shouldReconnect() {
  return !!lastDevice && !userDisconnected && !deviceSession.isConnected && !deviceSession.isConnecting
    && (enabled || AppState.currentState === 'active');
}

async function reconnectIfNeeded() {
  clearTimeout(retryTimer);
  retryTimer = null;
  if (!shouldReconnect()) return;
  const ok = await deviceSession.reconnect(lastDevice);
  if (!ok && shouldReconnect()) retryTimer = setTimeout(reconnectIfNeeded, RETRY_MS);
}

function updateServiceNotification() {
  if (!enabled || !lastDevice || userDisconnected) {
    stopForegroundService();
    return;
  }
  const name = deviceSession.connectedName || lastDevice.name || 'Clayton Power';
  startForegroundService(
    name,
    deviceSession.isConnected ? 'Connected · monitoring in the background' : 'Waiting for the unit to come in range',
  );
}

function onConnectionChange(connected) {
  if (connected) {
    userDisconnected = false;
    const device = deviceSession.connectedDevice;
    if (device) {
      lastDevice = device;
      prefs.setLastDevice(device);
      emit();
    }
  } else {
    reconnectIfNeeded();
  }
  updateServiceNotification();
}

export const backgroundService = {
  /** Call once at app start (while the app is in the foreground). */
  async start() {
    await setupNotifications();
    startAlertMonitor();
    [enabled, lastDevice] = await Promise.all([prefs.getBackgroundEnabled(), prefs.getLastDevice()]);
    setAlertsEnabled(enabled);
    emit();

    deviceSession.onConnectionChange(onConnectionChange);
    AppState.addEventListener('change', (appState) => {
      if (appState === 'active') reconnectIfNeeded();
    });

    updateServiceNotification();
    reconnectIfNeeded();
  },

  /** Settings toggle. Returns false when notification permission is refused. */
  async setEnabled(value) {
    if (value && !(await requestNotificationPermission())) return false;
    enabled = !!value;
    setAlertsEnabled(enabled);
    await prefs.setBackgroundEnabled(enabled);
    emit();
    updateServiceNotification();
    reconnectIfNeeded();
    return true;
  },

  /** DISCONNECT in the app: stay disconnected until the user connects again. */
  async disconnectByUser() {
    userDisconnected = true;
    clearTimeout(retryTimer);
    await deviceSession.cancelReconnect();
    await deviceSession.disconnect();
    updateServiceNotification();
  },

  get state() {
    return { enabled, lastDevice };
  },

  subscribe(fn) {
    listeners.add(fn);
    return () => listeners.delete(fn);
  },
};
