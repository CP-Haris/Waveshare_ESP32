import { Platform } from 'react-native';
import { requireOptionalNativeModule } from 'expo';

// Android foreground service that keeps the BLE link alive in the background
// (android/…/BleForegroundService.kt). On iOS, or in a dev build made before
// this module existed, every call is a no-op.
const native = Platform.OS === 'android' ? requireOptionalNativeModule('BleForeground') : null;

export const isAvailable = !!native;

if (__DEV__) console.log(`[BleForeground] native module ${isAvailable ? 'available' : 'NOT available'}`);

/**
 * Show the ongoing notification: starts the service, or just changes its text
 * when it already runs. Android 12+ only allows the start while the app is in
 * the foreground; a refused start is logged, not thrown.
 */
export function startForegroundService(title, text) {
  if (!native) return;
  try {
    if (native.isRunning()) native.update(title, text);
    else native.start(title, text);
  } catch (error) {
    console.warn('[BleForeground] start refused:', error.message);
  }
}

export function stopForegroundService() {
  if (!native) return;
  try {
    native.stop();
  } catch (error) {
    console.warn('[BleForeground] stop failed:', error.message);
  }
}
