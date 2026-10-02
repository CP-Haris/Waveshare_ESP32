import AsyncStorage from '@react-native-async-storage/async-storage';
import { ClaytonPowerDevice } from '../models/ClaytonPowerDevice';

const DEVICES_KEY = 'claytonpower_devices';

// Performance optimization: Cache for pending SoC updates
const pendingSoCUpdates = new Map<string, { stateOfCharge: string; timestamp: number }>();
let socBatchTimeout: NodeJS.Timeout | null = null;

// Performance optimization: In-memory cache of last saved SoC values
const lastSavedSoCCache = new Map<string, string>();

export const getDevices = async (): Promise<ClaytonPowerDevice[]> => {
  try {
    const jsonValue = await AsyncStorage.getItem(DEVICES_KEY);
    const devices = jsonValue ? JSON.parse(jsonValue) : [];
    
    // Filter out devices with invalid serial numbers
    const validDevices = devices.filter((device: any) => 
      device.serialNumber && 
      device.serialNumber !== 'Unknown' && 
      device.serialNumber !== '' && 
      device.serialNumber !== '-'
    );
    
    // Update the SoC cache when loading devices
    validDevices.forEach((device: any) => {
      if (device.deviceId && device.stateOfCharge) {
        lastSavedSoCCache.set(device.deviceId, device.stateOfCharge);
      }
    });
    
    // Preserve stored SoC data
    return validDevices.map((device: any) => ({
      ...device,
      stateOfCharge: device.stateOfCharge || "" // Keep existing SoC or default to empty string
    }));
  } catch (error) {
    console.error('Error reading devices:', error);
    return [];
  }
};

export const saveDevices = async (devices: ClaytonPowerDevice[]) => {
  try {
    await AsyncStorage.setItem(DEVICES_KEY, JSON.stringify(devices));
  } catch (error) {
    console.error('Error saving devices:', error);
  }
};

export const addDevice = async (device: ClaytonPowerDevice) => {
  // Filter out devices with invalid serial numbers
  if (!device.serialNumber || 
      device.serialNumber === 'Unknown' || 
      device.serialNumber === '' || 
      device.serialNumber === '-') {
    console.log(`[Storage] Skipping device with invalid serial number: ${device.serialNumber}`);
    return;
  }

  const devices = await getDevices();
  const existingIndex = devices.findIndex((d) => d.deviceId === device.deviceId);
  
  // Create a device object including stateOfCharge for storage
  const deviceToStore = {
    deviceId: device.deviceId,
    serialNumber: device.serialNumber,
    name: device.name,
    stateOfCharge: device.stateOfCharge || "", // Include SoC data
    isVisible: device.isVisible,
    isConnected: device.isConnected,
    lastSeenTimestamp: device.lastSeenTimestamp
  };
  
  if (existingIndex === -1) {
    devices.push(deviceToStore as ClaytonPowerDevice);
  } else {
    // Overwrite if new device has a better serial, etc.
    const existing = devices[existingIndex];
    if (
      (!existing.serialNumber || existing.serialNumber === "" || existing.serialNumber === "-") &&
      device.serialNumber &&
      device.serialNumber !== "-" &&
      device.serialNumber.length > 0
    ) {
      existing.serialNumber = device.serialNumber;
    }
    existing.name = deviceToStore.name;
    existing.stateOfCharge = deviceToStore.stateOfCharge;
    existing.isVisible = deviceToStore.isVisible;
    existing.isConnected = deviceToStore.isConnected;
    existing.lastSeenTimestamp = deviceToStore.lastSeenTimestamp;
  }
  await saveDevices(devices);
};

export const removeDeviceFromStorage = async (deviceId: string): Promise<void> => {
  try {
    const devices = await getDevices();
    const updatedDevices = devices.filter((d) => d.deviceId !== deviceId);
    await saveDevices(updatedDevices);
  } catch (error) {
    console.error("Error removing device from storage:", error);
  }
};

export const clearDevices = async () => {
  try {
    await AsyncStorage.removeItem(DEVICES_KEY);
  } catch (error) {
    console.error('Error clearing devices:', error);
  }
};

export const updateDeviceSoC = async (deviceId: string, stateOfCharge: string): Promise<void> => {
  try {
    // Check if value actually changed
    const lastSavedValue = lastSavedSoCCache.get(deviceId);
    if (lastSavedValue === stateOfCharge) {
      // Value hasn't changed, skip storage write
      return;
    }
    
    // Update the cache immediately
    lastSavedSoCCache.set(deviceId, stateOfCharge);
    
    // Add to pending updates for batching
    pendingSoCUpdates.set(deviceId, {
      stateOfCharge,
      timestamp: Date.now()
    });
    
    // Schedule batched write
    if (socBatchTimeout) {
      clearTimeout(socBatchTimeout);
    }
    
    socBatchTimeout = setTimeout(flushPendingSoCUpdates, 1000); // Batch writes for 1 second
    
  } catch (error) {
    console.error(`Error updating SoC for device ${deviceId}:`, error);
  }
};

// Performance optimization: Batch multiple SoC updates into a single storage write
const flushPendingSoCUpdates = async () => {
  if (pendingSoCUpdates.size === 0) return;
  
  try {
    console.log(`[Storage] Flushing ${pendingSoCUpdates.size} SoC updates to storage`);
    
    const jsonValue = await AsyncStorage.getItem(DEVICES_KEY);
    const devices = jsonValue ? JSON.parse(jsonValue) : [];
    
    let hasChanges = false;
    
    // Apply all pending updates
    pendingSoCUpdates.forEach((update, deviceId) => {
      const deviceIndex = devices.findIndex((d: any) => d.deviceId === deviceId);
      if (deviceIndex !== -1) {
        devices[deviceIndex].stateOfCharge = update.stateOfCharge;
        devices[deviceIndex].lastSoCUpdate = update.timestamp;
        hasChanges = true;
      }
    });
    
    // Clear pending updates
    pendingSoCUpdates.clear();
    socBatchTimeout = null;
    
    // Only write if there were actual changes
    if (hasChanges) {
      await saveDevices(devices);
      console.log(`[Storage] Successfully batched SoC updates to storage`);
    }
    
  } catch (error) {
    console.error('Error flushing SoC updates:', error);
    // Clear the pending updates even on error to prevent memory buildup
    pendingSoCUpdates.clear();
    socBatchTimeout = null;
  }
};

// Performance optimization: Immediate SoC update for critical scenarios (e.g., app closing)
export const flushPendingSoCUpdatesSync = async (): Promise<void> => {
  if (socBatchTimeout) {
    clearTimeout(socBatchTimeout);
    await flushPendingSoCUpdates();
  }
};

// Performance optimization: Check if SoC has changed without triggering storage write
export const hasSoCChanged = (deviceId: string, newSoC: string): boolean => {
  const lastSavedValue = lastSavedSoCCache.get(deviceId);
  return lastSavedValue !== newSoC;
};
