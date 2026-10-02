import { bleService } from '../ble/BLEService';
import { getDevices } from '../storage/DeviceStorage';
import { ClaytonPowerDevice } from '../models/ClaytonPowerDevice';
import i18n from '../i18n/config';
import { Logger } from '../components/Logger';
import AsyncStorage from '@react-native-async-storage/async-storage';

// Import Notifee - it's a standard React Native module that should work
import notifee, { AndroidImportance } from '@notifee/react-native';

// Lazy imports for Expo modules to avoid crashing if not available
let BackgroundFetch: any = null;
let TaskManager: any = null;

// Try to import Expo native modules, but don't crash if they're not available
try {
  BackgroundFetch = require('expo-background-fetch');
  TaskManager = require('expo-task-manager');
} catch (error) {
  // Silently fail - background tasks just won't be available
}

const LOW_BATTERY_TASK_NAME = 'low-battery-scan';
const LOW_BATTERY_THRESHOLD = 20; // 20% threshold
const SCAN_DURATION = 15000; // 15 seconds scan duration
const NOTIFICATION_COOLDOWN = 24 * 60 * 60 * 1000; // 24 hours cooldown between notifications for same device
const NOTIFICATION_CACHE_KEY = '@lowBattery/notificationCache';

// Track last notification time per device (24 hour cache)
// Persisted to AsyncStorage to survive app restarts
const lastNotificationTime = new Map<string, number>();
let cacheLoaded = false;

/**
 * Load notification cache from AsyncStorage
 */
async function loadNotificationCache(): Promise<void> {
  if (cacheLoaded) return;
  
  try {
    const cached = await AsyncStorage.getItem(NOTIFICATION_CACHE_KEY);
    if (cached) {
      const parsed = JSON.parse(cached) as Record<string, number>;
      const now = Date.now();
      
      // Only load entries that are still within the cooldown period
      Object.entries(parsed).forEach(([deviceId, timestamp]) => {
        if (now - timestamp < NOTIFICATION_COOLDOWN) {
          lastNotificationTime.set(deviceId, timestamp);
        }
      });
      
      Logger.info(`[LowBattery] Loaded ${lastNotificationTime.size} cached notification entries`);
    }
    cacheLoaded = true;
  } catch (error) {
    Logger.error('[LowBattery] Failed to load notification cache:', error);
    cacheLoaded = true; // Mark as loaded to prevent repeated attempts
  }
}

/**
 * Save notification cache to AsyncStorage
 */
async function saveNotificationCache(): Promise<void> {
  try {
    const cacheObject = Object.fromEntries(lastNotificationTime);
    await AsyncStorage.setItem(NOTIFICATION_CACHE_KEY, JSON.stringify(cacheObject));
  } catch (error) {
    Logger.error('[LowBattery] Failed to save notification cache:', error);
  }
}

/**
 * Parse battery percentage from stateOfCharge string
 */
function parseBatteryPercentage(stateOfCharge: string): number | null {
  if (!stateOfCharge || stateOfCharge === '' || stateOfCharge === '-') {
    return null;
  }
  
  const percentage = parseFloat(stateOfCharge);
  if (isNaN(percentage)) {
    return null;
  }
  
  return Math.max(0, Math.min(100, percentage));
}

/**
 * Check if we should send a notification for this device
 * (respects 24 hour cooldown period)
 */
function shouldSendNotification(deviceId: string): boolean {
  const lastNotified = lastNotificationTime.get(deviceId);
  if (!lastNotified) {
    return true;
  }
  
  const timeSinceLastNotification = Date.now() - lastNotified;
  return timeSinceLastNotification >= NOTIFICATION_COOLDOWN;
}

/**
 * Clear the notification cache for a device (when battery is good)
 */
async function clearNotificationCache(deviceId: string): Promise<void> {
  if (lastNotificationTime.has(deviceId)) {
    lastNotificationTime.delete(deviceId);
    await saveNotificationCache();
    Logger.info(`[LowBattery] Cleared cache for ${deviceId} (battery is good)`);
  }
}

/**
 * Update notification cache for a device
 */
async function updateNotificationCache(deviceId: string): Promise<void> {
  lastNotificationTime.set(deviceId, Date.now());
  await saveNotificationCache();
}

/**
 * Send low battery notification for a device
 */
async function sendLowBatteryNotification(device: ClaytonPowerDevice, batteryLevel: number) {
  try {
    // Check cooldown
    if (!shouldSendNotification(device.deviceId)) {
      Logger.info(`[LowBattery] Skipping notification for ${device.serialNumber} - still in cooldown`);
      return;
    }

    // Create a channel for Android
    const channelId = await notifee.createChannel({
      id: 'low-battery',
      name: 'Low Battery Warnings',
      importance: AndroidImportance.HIGH,
      sound: 'default',
      vibration: true,
    });

    // Get device name for notification
    const deviceName = device.serialNumber || device.name || 'Device';
    
    // Get translated notification text (with fallback to English)
    const title = i18n.t('notifications.lowBatteryTitle', { defaultValue: 'Low Battery Warning' });
    const body = i18n.t('notifications.lowBatteryBody', {
      deviceName,
      batteryLevel,
      defaultValue: `${deviceName} has ${batteryLevel}% battery remaining`
    });

    // Display a notification
    await notifee.displayNotification({
      title,
      body,
      android: {
        channelId,
        importance: AndroidImportance.HIGH,
        pressAction: {
          id: 'default',
        },
        smallIcon: 'ic_launcher',
      },
      ios: {
        foregroundPresentationOptions: {
          alert: true,
          badge: true,
          sound: true,
        },
      },
      data: {
        deviceId: device.deviceId,
        serialNumber: device.serialNumber,
        batteryLevel: batteryLevel.toString(),
      },
    });

    // Update last notification time (persisted)
    await updateNotificationCache(device.deviceId);
    Logger.info(`[LowBattery] Sent notification for ${device.serialNumber} (${batteryLevel}%)`);
  } catch (error) {
    Logger.error(`[LowBattery] Error sending notification:`, error);
  }
}

/**
 * Background task that scans for devices and checks battery levels
 */
async function performLowBatteryScan() {
  const scanStartTime = new Date().toLocaleTimeString();
  Logger.info('[LowBattery] 🔍 Starting battery scan at', scanStartTime);
  Logger.info(`[LowBattery] 📊 Will notify if devices < ${LOW_BATTERY_THRESHOLD}%`);
  
  try {
    // Ensure notification cache is loaded
    await loadNotificationCache();
    
    // Load saved devices from storage
    const savedDevices = await getDevices();
    Logger.info(`[LowBattery] Found ${savedDevices.length} saved devices`);
    
    if (savedDevices.length === 0) {
      Logger.info('[LowBattery] No saved devices to check');
      return BackgroundFetch?.BackgroundFetchResult?.NoData ?? 'NoData';
    }

    // Initialize BLE if needed
    try {
      await bleService.initializeBLE();
    } catch (error) {
      Logger.error('[LowBattery] Failed to initialize BLE:', error);
      return BackgroundFetch?.BackgroundFetchResult?.Failed ?? 'Failed';
    }

    // Create a Set of saved device IDs and serial numbers for quick lookup
    const savedDeviceIds = new Set(savedDevices.map(d => d.deviceId));
    const savedSerialNumbers = new Set(
      savedDevices
        .map(d => d.serialNumber)
        .filter(sn => sn && sn !== '' && sn !== '-' && sn !== 'Unknown')
    );

    // Start scanning for devices
    const foundDevices = new Map<string, { device: ClaytonPowerDevice; batteryLevel: number }>();
    
    // Set up a listener for found devices (only saved devices)
    const deviceFoundListener = (enrichedDevice: any) => {
      const isSavedDevice = savedDeviceIds.has(enrichedDevice.deviceId) || 
                           (enrichedDevice.serialNumber && savedSerialNumbers.has(enrichedDevice.serialNumber));
      
      if (!isSavedDevice) return;

      const batteryLevel = parseBatteryPercentage(enrichedDevice.stateOfCharge);
      
      if (batteryLevel !== null) {
        const savedDevice = savedDevices.find(
          d => d.deviceId === enrichedDevice.deviceId || 
               d.serialNumber === enrichedDevice.serialNumber
        );
        
        if (savedDevice) {
          foundDevices.set(savedDevice.deviceId, {
            device: savedDevice,
            batteryLevel,
          });
        }
      }
    };

    // Register listener to receive scan results
    bleService.subscribeToDeviceDiscoveries(deviceFoundListener);

    // Request scanning with aggressive settings
    const wasAlreadyScanning = !bleService.requestScanning(5, 500);
    
    if (wasAlreadyScanning) {
      Logger.info('[LowBattery] Using existing scan (app in foreground)');
      await new Promise(resolve => setTimeout(resolve, Math.min(SCAN_DURATION, 5000)));
    } else {
      Logger.info('[LowBattery] Started new scan for battery check');
      await new Promise(resolve => setTimeout(resolve, SCAN_DURATION));
    }

    // Release scanning request
    bleService.releaseScanning();
    bleService.unsubscribeFromDeviceDiscoveries(deviceFoundListener);

    Logger.info(`[LowBattery] Scan complete. Found ${foundDevices.size} devices`);

    // Check for low battery devices and send notifications
    let lowBatteryCount = 0;
    for (const [, { device, batteryLevel }] of foundDevices.entries()) {
      if (batteryLevel < LOW_BATTERY_THRESHOLD) {
        if (shouldSendNotification(device.deviceId)) {
          Logger.info(`[LowBattery] ${device.serialNumber}: SoC=${batteryLevel}% < ${LOW_BATTERY_THRESHOLD}% → Sending notification`);
          await sendLowBatteryNotification(device, batteryLevel);
          lowBatteryCount++;
        } else {
          const lastNotified = lastNotificationTime.get(device.deviceId);
          const timeSince = lastNotified ? Date.now() - lastNotified : 0;
          const hoursSince = Math.round(timeSince / (60 * 60 * 1000));
          const minutesSince = Math.round(timeSince / (60 * 1000));
          const displayTime = hoursSince > 0 ? `${hoursSince}h` : `${minutesSince}min`;
          Logger.info(`[LowBattery] ${device.serialNumber}: SoC=${batteryLevel}% < ${LOW_BATTERY_THRESHOLD}% → Skipped (notified ${displayTime} ago)`);
        }
      } else {
        Logger.info(`[LowBattery] ${device.serialNumber}: SoC=${batteryLevel}% ≥ ${LOW_BATTERY_THRESHOLD}% → OK`);
        await clearNotificationCache(device.deviceId);
      }
    }

    // Check saved devices not found in scan (use stored battery data if fresh)
    for (const savedDevice of savedDevices) {
      if (foundDevices.has(savedDevice.deviceId)) continue;

      const batteryLevel = parseBatteryPercentage(savedDevice.stateOfCharge);
      if (batteryLevel === null) continue;
      
      if (batteryLevel < LOW_BATTERY_THRESHOLD) {
        const lastUpdate = savedDevice.lastSoCUpdate || savedDevice.lastSeenTimestamp || 0;
        const dataAge = Date.now() - lastUpdate;
        const MAX_DATA_AGE = 3600000; // 1 hour
        
        if (dataAge < MAX_DATA_AGE && shouldSendNotification(savedDevice.deviceId)) {
          Logger.info(`[LowBattery] ${savedDevice.serialNumber} (stored): SoC=${batteryLevel}% → Sending notification`);
          await sendLowBatteryNotification(savedDevice, batteryLevel);
          lowBatteryCount++;
        } else if (dataAge >= MAX_DATA_AGE) {
          Logger.info(`[LowBattery] ${savedDevice.serialNumber} (stored): Data too old (${Math.round(dataAge / 60000)}min)`);
        }
      } else {
        await clearNotificationCache(savedDevice.deviceId);
      }
    }

    Logger.info(`[LowBattery] 📊 Summary: ${savedDevices.length} saved, ${foundDevices.size} found, ${lowBatteryCount} low battery`);
    
    return lowBatteryCount > 0 
      ? (BackgroundFetch?.BackgroundFetchResult?.NewData ?? 'NewData')
      : (BackgroundFetch?.BackgroundFetchResult?.NoData ?? 'NoData');
      
  } catch (error) {
    Logger.error('[LowBattery] Error during background scan:', error);
    return BackgroundFetch?.BackgroundFetchResult?.Failed ?? 'Failed';
  }
}

// Define the background task (only if TaskManager is available)
if (TaskManager) {
  try {
    TaskManager.defineTask(LOW_BATTERY_TASK_NAME, async () => {
      Logger.info('[LowBattery] ⚡ Background task triggered by system');
      const result = await performLowBatteryScan();
      Logger.info('[LowBattery] ✅ Background task completed:', result);
      return result;
    });
    Logger.info('[LowBattery] Background task defined:', LOW_BATTERY_TASK_NAME);
  } catch (error) {
    Logger.warn('[LowBattery] Failed to define task:', error);
  }
}

/**
 * Register the background fetch task
 */
export async function registerLowBatteryBackgroundTask(): Promise<boolean> {
  if (!BackgroundFetch || !TaskManager) {
    Logger.info('[LowBattery] BackgroundFetch/TaskManager modules not loaded');
    return false;
  }

  try {
    if (typeof BackgroundFetch.getStatusAsync !== 'function' || 
        typeof BackgroundFetch.registerTaskAsync !== 'function') {
      Logger.info('[LowBattery] BackgroundFetch methods not available - native code may need rebuild');
      return false;
    }
    
    // BackgroundFetchStatus: Denied = 1, Restricted = 2, Available = 3
    const status = await BackgroundFetch.getStatusAsync();
    if (status === null || status === 1 || status === 2) {
      Logger.warn('[LowBattery] Background fetch not available (status:', status, ')');
      return false;
    }

    const isRegistered = await TaskManager.isTaskRegisteredAsync(LOW_BATTERY_TASK_NAME);
    if (isRegistered) {
      Logger.info('[LowBattery] ✅ Background task already registered');
      return true;
    }

    await BackgroundFetch.registerTaskAsync(LOW_BATTERY_TASK_NAME, {
      minimumInterval: 60 * 60, // 1 hour
      stopOnTerminate: false,
      startOnBoot: true,
    });

    Logger.info('[LowBattery] ✅ Background task registered (1 hour interval)');
    Logger.warn('[LowBattery] ⚠️ iOS Background Fetch timing is controlled by iOS');
    return true;
  } catch (error) {
    Logger.error('[LowBattery] Failed to register background task:', error);
    return false;
  }
}

/**
 * Unregister the background fetch task
 */
export async function unregisterLowBatteryBackgroundTask(): Promise<void> {
  if (!BackgroundFetch || !TaskManager) return;

  try {
    const isRegistered = await TaskManager.isTaskRegisteredAsync(LOW_BATTERY_TASK_NAME);
    if (isRegistered) {
      await BackgroundFetch.unregisterTaskAsync(LOW_BATTERY_TASK_NAME);
      Logger.info('[LowBattery] Background task unregistered');
    }
  } catch (error) {
    Logger.error('[LowBattery] Failed to unregister background task:', error);
  }
}

/**
 * Initialize Notifee notification channels
 */
export async function initializeNotifee(): Promise<void> {
  try {
    await notifee.requestPermission();

    const channelName = i18n.t('notifications.lowBatteryChannelName', { defaultValue: 'Low Battery Warnings' });

    await notifee.createChannel({
      id: 'low-battery',
      name: channelName,
      importance: AndroidImportance.HIGH,
      sound: 'default',
      vibration: true,
    });

    // Load notification cache on initialization
    await loadNotificationCache();

    Logger.info('[LowBattery] Notifee initialized');
  } catch (error) {
    Logger.error('[LowBattery] Failed to initialize Notifee:', error);
  }
}

/**
 * Manually trigger a battery scan (for testing)
 */
export async function triggerManualScan(): Promise<string> {
  try {
    Logger.info('[LowBattery] 🔍 Manual scan triggered');
    const result = await performLowBatteryScan();
    Logger.info('[LowBattery] ✅ Scan completed:', result);
    return `Scan completed: ${result}`;
  } catch (error) {
    Logger.error('[LowBattery] Manual scan failed:', error);
    return `Scan failed: ${error}`;
  }
}
