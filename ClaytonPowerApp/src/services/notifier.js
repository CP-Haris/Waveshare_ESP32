import { Platform } from 'react-native';
import * as Notifications from 'expo-notifications';

// Local notifications about the connected unit (errors, battery). The ongoing
// "connected in the background" notification is owned by the native
// foreground service, not by this module.

const CHANNEL_ID = 'unit-alerts';

let ready = false;

export async function setupNotifications() {
  if (ready) return;
  ready = true;
  Notifications.setNotificationHandler({
    handleNotification: async () => ({
      shouldShowBanner: true,
      shouldShowList: true,
      shouldPlaySound: true,
      shouldSetBadge: false,
    }),
  });
  if (Platform.OS === 'android') {
    await Notifications.setNotificationChannelAsync(CHANNEL_ID, {
      name: 'Unit alerts',
      description: 'Errors and battery events from the connected unit',
      importance: Notifications.AndroidImportance.HIGH,
      lightColor: '#4E9EEB',
    });
  }
}

/** Ask once (Android 13+ / iOS). Returns true when notifications may be shown. */
export async function requestNotificationPermission() {
  const current = await Notifications.getPermissionsAsync();
  if (current.granted) return true;
  const result = await Notifications.requestPermissionsAsync();
  return !!result.granted;
}

export function postNotification(title, body) {
  return Notifications.scheduleNotificationAsync({
    content: { title, body },
    trigger: Platform.OS === 'android' ? { channelId: CHANNEL_ID } : null,
  }).catch((error) => console.warn('[Notify] failed:', error.message));
}
