import { useState, useEffect, useCallback, useRef } from 'react';
import { Alert, AppState, AppStateStatus } from 'react-native';
import { BleError, Device, Subscription } from 'react-native-ble-plx';
import { bleService } from '../ble/BLEService';
import { getDevices, addDevice, removeDeviceFromStorage, hasSoCChanged, updateDeviceSoC as saveDeviceSoCToStorage, flushPendingSoCUpdatesSync } from '../storage/DeviceStorage';
import { ClaytonPowerDevice } from '../models/ClaytonPowerDevice';

// Performance optimization: Debounce utility
const debounce = <T extends (...args: any[]) => void>(func: T, delay: number): T => {
  let timeoutId: NodeJS.Timeout;
  return ((...args: any[]) => {
    clearTimeout(timeoutId);
    timeoutId = setTimeout(() => func(...args), delay);
  }) as T;
};

/**
 * useDeviceList hook with:
 * - Efficient device visibility tracking
 * - Separate state for reachable devices
 * - Optimized updates with debouncing
 * - Better performance for large device lists
 */
export const useDeviceList = () => {
  const [devices, setDevices] = useState<ClaytonPowerDevice[]>([]);
  const [isScanning, setIsScanning] = useState(false);
  const [isConnecting, setIsConnecting] = useState(false);

  // Keep track of reachable devices separately
  const reachableDevicesRef = useRef<Set<string>>(new Set());
  const lastSeenRef = useRef<Map<string, number>>(new Map());
  const updateTimeoutRef = useRef<NodeJS.Timeout | null>(null);
  
  // Performance optimization: Batch pending device updates
  const pendingUpdatesRef = useRef<Map<string, Partial<ClaytonPowerDevice>>>(new Map());

  const hasShownOffAlertRef = useRef(false);
  const bondingAlertShownRef = useRef(false);
  const bondingDeclinedRef = useRef(false);
  const bluetoothStateSubscriptionRef = useRef<Subscription | null>(null);
  const appStateRef = useRef(AppState.currentState);
  const shouldRestartScanRef = useRef(false);

  // Keep a reference to devices for background polling
  const devicesRef = useRef(devices);
  useEffect(() => {
    devicesRef.current = devices;
  }, [devices]);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      if (updateTimeoutRef.current) {
        clearTimeout(updateTimeoutRef.current);
      }
      reachableDevicesRef.current.clear();
      lastSeenRef.current.clear();
      pendingUpdatesRef.current.clear();
    };
  }, []);

  /**
   * Update device visibility based on last seen time
   * Performance optimization: Increased interval from 1000ms to 3000ms
   */
  const updateDeviceVisibility = useCallback(() => {
    const now = Date.now();
    const VISIBILITY_TIMEOUT = 5000; // 5 seconds - slightly increased for better stability

    // Check which devices should be marked as unreachable
    const unreachableDevices = new Set<string>();
    lastSeenRef.current.forEach((timestamp, deviceId) => {
      if (now - timestamp > VISIBILITY_TIMEOUT) {
        unreachableDevices.add(deviceId);
      }
    });

    // If no changes, skip the update
    if (unreachableDevices.size === 0) return;

    // Update the devices state
    setDevices(prev => {
      const hasChanges = prev.some(device => 
        unreachableDevices.has(device.deviceId) && device.isVisible
      );

      if (!hasChanges) return prev;

      return prev.map(device => {
        if (unreachableDevices.has(device.deviceId)) {
          return { ...device, isVisible: false };
        }
        return device;
      });
    });

    // Clean up unreachable devices from our tracking
    unreachableDevices.forEach(deviceId => {
      lastSeenRef.current.delete(deviceId);
      reachableDevicesRef.current.delete(deviceId);
    });
  }, []);

  /**
   * Performance optimization: Debounced device updates
   */
  const applyPendingUpdates = useCallback(() => {
    if (pendingUpdatesRef.current.size === 0) return;

    const updates = new Map(pendingUpdatesRef.current);
    pendingUpdatesRef.current.clear();

    setDevices(prev => {
      let hasChanges = false;
      const updated = prev.map(device => {
        const update = updates.get(device.deviceId);
        if (update) {
          hasChanges = true;
          return { ...device, ...update };
        }
        return device;
      });

      // Add new devices
      updates.forEach((update, deviceId) => {
        if (!prev.find(d => d.deviceId === deviceId)) {
          // Filter out devices with invalid serial numbers
          if (!update.serialNumber || 
              update.serialNumber === 'Unknown' || 
              update.serialNumber === '' || 
              update.serialNumber === '-') {
            // Don't add devices without valid serial numbers
            return;
          }
          
          hasChanges = true;
          updated.push({
            deviceId,
            serialNumber: update.serialNumber,
            name: update.name || 'Unnamed Device',
            stateOfCharge: update.stateOfCharge,
            isVisible: update.isVisible ?? true,
            lastSeenTimestamp: update.lastSeenTimestamp ?? Date.now(),
            isConnected: update.isConnected ?? false
          } as ClaytonPowerDevice);
        }
      });

      return hasChanges ? updated : prev;
    });
  }, []);

  // Debounced update function
  const debouncedApplyUpdates = useCallback(
    debounce(applyPendingUpdates, 300), // 300ms debounce
    [applyPendingUpdates]
  );

  const onDeviceFound = useCallback((device: any) => {
    const deviceId = device.deviceId;
    const now = Date.now();

    if (!device.serialNumber || 
        device.serialNumber === 'Unknown' || 
        device.serialNumber === '' || 
        device.serialNumber === '-') {
      return;
    }

    lastSeenRef.current.set(deviceId, now);
    reachableDevicesRef.current.add(deviceId);

    const existingDevice = devicesRef.current.find(d => d.deviceId === deviceId);
    const currentUpdate = pendingUpdatesRef.current.get(deviceId) || {};
    
    const hasSerialChange = !existingDevice?.serialNumber && device.serialNumber;
    const hasVisibilityChange = !existingDevice || !existingDevice.isVisible;
    const deviceHasSoC = device.stateOfCharge && device.stateOfCharge !== "" && device.stateOfCharge !== "-";
    const shouldUpdate = !existingDevice || hasVisibilityChange || hasSerialChange || deviceHasSoC;

    if (shouldUpdate) {
      let bestSoC = "";
      if (deviceHasSoC) {
        bestSoC = device.stateOfCharge;
        saveDeviceSoCToStorage(deviceId, device.stateOfCharge);
      } else {
        const lastSeen = lastSeenRef.current.get(deviceId);
        const hasRecentLiveData = lastSeen && (now - lastSeen < 5000);
        bestSoC = currentUpdate.stateOfCharge || existingDevice?.stateOfCharge || "";
      }
      
      pendingUpdatesRef.current.set(deviceId, {
        ...currentUpdate,
        isVisible: true,
        lastSeenTimestamp: now,
        stateOfCharge: bestSoC,
        serialNumber: device.serialNumber,
        name: currentUpdate.name || existingDevice?.name || device.name || 'Unnamed Device'
      });

      debouncedApplyUpdates();
    }
  }, [debouncedApplyUpdates]);

  // Performance optimization: Increased interval from 1000ms to 3000ms
  useEffect(() => {
    const intervalId = setInterval(updateDeviceVisibility, 3000);
    return () => clearInterval(intervalId);
  }, [updateDeviceVisibility]);

  const updateDeviceSoC = useCallback((deviceId: string, newSoC: string) => {
    const now = Date.now();
    lastSeenRef.current.set(deviceId, now);
    reachableDevicesRef.current.add(deviceId);
    
    const update = pendingUpdatesRef.current.get(deviceId) || {};
    pendingUpdatesRef.current.set(deviceId, {
      ...update,
      stateOfCharge: newSoC,
      lastSeenTimestamp: now
    });
    
    saveDeviceSoCToStorage(deviceId, newSoC);
    debouncedApplyUpdates();
  }, [debouncedApplyUpdates]);

  const loadCachedDevices = useCallback(async () => {
    const cached = await getDevices();
    const now = Date.now();
    const LIVE_DATA_THRESHOLD = 5000;
    
    const updated = await Promise.all(
      cached.map(async (d) => {
        const isConnected = await bleService.isDeviceConnected(d.deviceId);
        const lastSeen = lastSeenRef.current.get(d.deviceId);
        const hasLiveData = lastSeen && (now - lastSeen < LIVE_DATA_THRESHOLD);
        const pendingUpdate = pendingUpdatesRef.current.get(d.deviceId);
        const hasPendingLiveSoC = pendingUpdate?.stateOfCharge && 
                                   pendingUpdate?.stateOfCharge !== "" &&
                                   pendingUpdate?.stateOfCharge !== "-";
        const existingDevice = devicesRef.current.find(dev => dev.deviceId === d.deviceId);
        const hasExistingLiveSoC = existingDevice?.stateOfCharge && 
                                   existingDevice.stateOfCharge !== "" &&
                                   existingDevice.stateOfCharge !== "-" &&
                                   (existingDevice.isVisible || existingDevice.isConnected);
        
        let socToUse = "";
        if (hasPendingLiveSoC && pendingUpdate?.stateOfCharge) {
          socToUse = pendingUpdate.stateOfCharge;
        } else if (hasExistingLiveSoC && existingDevice?.stateOfCharge) {
          socToUse = existingDevice.stateOfCharge;
        } else if (hasLiveData && existingDevice?.stateOfCharge) {
          socToUse = existingDevice.stateOfCharge;
        } else {
          socToUse = d.stateOfCharge || "";
        }
        
        return {
          ...d,
          isVisible: Boolean(isConnected || hasLiveData),
          isConnected: Boolean(isConnected),
          stateOfCharge: socToUse
        };
      })
    );
    
    setDevices(prev => {
      const merged = new Map<string, ClaytonPowerDevice>();
      
      prev.forEach(device => {
        const lastSeen = lastSeenRef.current.get(device.deviceId);
        const hasLiveData = lastSeen && (now - lastSeen < LIVE_DATA_THRESHOLD);
        const pendingUpdate = pendingUpdatesRef.current.get(device.deviceId);
        
        if (hasLiveData || device.isConnected || device.isVisible) {
          const liveSoC = pendingUpdate?.stateOfCharge || device.stateOfCharge;
          if (liveSoC && liveSoC !== "" && liveSoC !== "-") {
            merged.set(device.deviceId, {
              ...device,
              stateOfCharge: liveSoC
            });
          } else {
            merged.set(device.deviceId, device);
          }
        } else {
          merged.set(device.deviceId, device);
        }
      });
      
      updated.forEach(cachedDevice => {
        const existing = merged.get(cachedDevice.deviceId);
        if (!existing) {
          merged.set(cachedDevice.deviceId, {
            ...cachedDevice,
            isVisible: Boolean(cachedDevice.isVisible),
            isConnected: Boolean(cachedDevice.isConnected)
          });
        } else {
          const lastSeen = lastSeenRef.current.get(cachedDevice.deviceId);
          const hasLiveData = lastSeen && (now - lastSeen < LIVE_DATA_THRESHOLD);
          const pendingUpdate = pendingUpdatesRef.current.get(cachedDevice.deviceId);
          
          if (!hasLiveData && !existing.isConnected && !existing.isVisible) {
            merged.set(cachedDevice.deviceId, {
              ...existing,
              ...cachedDevice,
              isVisible: Boolean(cachedDevice.isVisible ?? existing.isVisible),
              isConnected: Boolean(cachedDevice.isConnected ?? existing.isConnected),
              stateOfCharge: cachedDevice.stateOfCharge || existing.stateOfCharge
            });
          } else {
            merged.set(cachedDevice.deviceId, {
              ...existing,
              ...cachedDevice,
              isVisible: Boolean(cachedDevice.isVisible ?? existing.isVisible),
              isConnected: Boolean(cachedDevice.isConnected ?? existing.isConnected),
              stateOfCharge: pendingUpdate?.stateOfCharge || existing.stateOfCharge
            });
          }
        }
      });
      
      return Array.from(merged.values());
    });
  }, []);

  // Load cached devices on mount (deferred to avoid blocking startup)
  useEffect(() => {
    // Defer loading to allow UI to render first
    const loadTimer = setTimeout(() => {
      loadCachedDevices();
    }, 50);
    
    return () => clearTimeout(loadTimer);
  }, [loadCachedDevices]);

  /**
   * Handle device disconnection immediately without debouncing
   */
  const handleDeviceDisconnected = useCallback((deviceId: string) => {
    // Remove from reachable devices
    reachableDevicesRef.current.delete(deviceId);
    lastSeenRef.current.delete(deviceId);

    // Update device state
    setDevices(prev => 
      prev.map(device => 
        device.deviceId === deviceId 
          ? { ...device, isConnected: false, isVisible: false }
          : device
      )
    );
  }, []);

  /**
   * Connect to a device with retries. After connecting, discover services/characteristics.
   */
  const connectToDevice = useCallback(async (device: ClaytonPowerDevice) => {
    setIsConnecting(true);
    try {
      // Check if we're already connected to this device
      const isConnected = await bleService.isDeviceConnected(device.deviceId);
      if (isConnected) {
        // Update the device's connection state and return
        setDevices((prev) =>
          prev.map((d) =>
            d.deviceId === device.deviceId ? { ...d, isConnected: true } : d
          )
        );
        return;
      }

      // If not connected, proceed with connection
      await connectWithRetry(device.deviceId, 3, 1000);
      device.isConnected = true;
      if (device.serialNumber && device.serialNumber !== "-" && device.serialNumber.length > 0) {
        await addDevice(device);
      }
      await loadCachedDevices();
      
      // Set up disconnection listener
      const disconnectSub = bleService.onDeviceDisconnected(device.deviceId, (error, disconnectedDevice) => {
        if (disconnectedDevice) {
          handleDeviceDisconnected(device.deviceId);
          disconnectSub?.remove();
        }
      });
    } catch (err: any) {
      const bleError = err as BleError;
      if (bleService.isBondingIssue(bleError)) {
        if (!bondingAlertShownRef.current) {
          bondingAlertShownRef.current = true;
          Alert.alert(
            "Bonding Issue",
            "Pairing information missing for your Clayton Power device. Please remove the device in Bluetooth settings and try again."
          );
        }
      } else if (
        err?.name === "BondingDeclined" ||
        err?.code === "BONDING_DECLINED"
      ) {
        Alert.alert(
          "Pairing Cancelled",
          "You cancelled the pairing request. Please try connecting again and accept the pairing prompt."
        );
      } else {
        Alert.alert("Connection Failed", "Could not connect after multiple attempts.");
      }
    } finally {
      setIsConnecting(false);
      bondingAlertShownRef.current = false;
    }
  }, [loadCachedDevices, handleDeviceDisconnected]);

  // Replace your connectWithRetry function with this version
  async function connectWithRetry(deviceId: string, attempts: number, delayMs: number) {
    bondingDeclinedRef.current = false;
    let lastError: any;

    for (let i = 0; i < attempts; i++) {
      try {
        if (bondingDeclinedRef.current) {
          throw lastError;
        }

        await bleService.connectToDeviceWithTimeout(deviceId, 5000);
        return;
      } catch (err: any) {
        if (
          err?.name === "BondingDeclined" ||
          err?.code === "BONDING_DECLINED"
        ) {
          bondingDeclinedRef.current = true;
          throw err; // Exit loop early, don't retry
        }

        if (bleService.isBondingIssue(err)) {
          throw err; // Exit loop
        }

        lastError = err;

        if (i < attempts - 1 && !bondingDeclinedRef.current) {
          await new Promise((res) => setTimeout(res, delayMs));
        }
      }
    }

    throw lastError;
  }

  /**
   * Remove device from local state + storage, also disconnect from BLE.
   */
  const removeDevice = useCallback(async (deviceId: string) => {
    setDevices((prev) => prev.filter((d) => d.deviceId !== deviceId));
    await bleService.disconnectFromDevice(deviceId);
    await removeDeviceFromStorage(deviceId);
  }, []);

  /**
   * Disconnect from device (but keep it in the list).
   */
  const disconnectDevice = useCallback(async (deviceId: string) => {
    try {
      await bleService.disconnectFromDevice(deviceId);
      setDevices((prev) =>
        prev.map((d) =>
          d.deviceId === deviceId ? { ...d, isConnected: false } : d
        )
      );
    } catch (error) {
      console.error('Error disconnecting device:', error);
    }
  }, []);

  /**
   * Start scanning without an auto-stop, waiting for focus behavior to handle stop.
   */
  const startScan = useCallback(async () => {
    console.log('startScan: Starting scan process...');
    try {
      console.log('startScan: Initializing BLE...');
      await bleService.initializeBLE();
      console.log('startScan: BLE initialized successfully');

      const currentState = await bleService.getManager().state();
      console.log('startScan: Current Bluetooth state:', currentState);
      
      if (currentState === 'PoweredOn') {
        console.log('startScan: Bluetooth is powered on, starting scan...');
        hasShownOffAlertRef.current = false;
        setDevices((prev) => prev.map((d) => ({ ...d, isVisible: false })));
        setIsScanning(true);
        bleService.subscribeToDeviceDiscoveries(onDeviceFound);
        // Request scanning - uses reference counting so multiple components can share the scan
        bleService.requestScanning();
      } else if (currentState === 'PoweredOff') {
        console.log('startScan: Bluetooth is powered off');
        if (!hasShownOffAlertRef.current) {
          hasShownOffAlertRef.current = true;
          Alert.alert(
            'Bluetooth is Off',
            'Clayton Power GO requires Bluetooth for the full experience. Please turn on Bluetooth.'
          );
        }
      } else if (currentState === 'Unauthorized') {
        console.log('startScan: Bluetooth is unauthorized');
        Alert.alert(
          'Bluetooth Unauthorized',
          'Permissions for Bluetooth are denied. Please enable them in Settings.',
          [
            {
              text: 'Open Settings',
              onPress: () => bleService.openBluetoothSettings(),
            },
            { text: 'Cancel', style: 'cancel' },
          ]
        );
      }
    } catch (error) {
      console.log('startScan: Error during initialization:', error);
      if (error instanceof Error && error.message === 'NO_PERMISSIONS') {
        console.log('startScan: No permissions, checking if they were declined...');
        const hasPermissions = await bleService.requestBluetoothPermission();
        console.log('startScan: Permission request result:', hasPermissions);
        
        if (!hasPermissions) {
          console.log('startScan: Permissions were declined, showing alert...');
          Alert.alert(
            'Permissions Required',
            'Clayton Power GO requires Bluetooth permissions. Please enable them in Settings.',
            [
              {
                text: 'Open Settings',
                onPress: () => bleService.openBluetoothSettings(),
              },
              { text: 'Cancel', style: 'cancel' },
            ]
          );
        }
        return;
      } else {
        const bleError = error as BleError;
        console.error('Error initializing BLE:', bleError.errorCode, bleError.message);
        return;
      }
    }
  }, [onDeviceFound]);

  /**
   * Stop scanning manually (when screen unfocuses, etc.).
   */
  const stopScan = useCallback(() => {
    setIsScanning(false);
    bleService.unsubscribeFromDeviceDiscoveries(onDeviceFound);
    // Release scanning request - scanning will only stop if no other components need it
    bleService.releaseScanning();
  }, [onDeviceFound]);

  // Handle app state changes
  const handleAppStateChange = useCallback(async (nextAppState: AppStateStatus) => {
    console.log('App state changed from', appStateRef.current, 'to', nextAppState);
    
    if (appStateRef.current.match(/inactive|background/) && nextAppState === 'active') {
      console.log('App came to foreground, checking if scan restart is needed');
      
      // If we were scanning before going to background, restart scanning
      if (shouldRestartScanRef.current && isScanning) {
        console.log('Restarting scan after foreground transition');
        setTimeout(() => {
          if (bleService) {
            // Request scanning - will reuse existing scan if already active
            bleService.requestScanning();
          }
        }, 1000); // Small delay to ensure app is fully active
      }
    } else if (nextAppState.match(/inactive|background/)) {
      console.log('App going to background, marking scan for restart if needed');
      shouldRestartScanRef.current = isScanning;
      
      // Performance optimization: Flush pending SoC updates before going to background
      try {
        await flushPendingSoCUpdatesSync();
        console.log('Successfully flushed pending SoC updates before background');
      } catch (error) {
        console.error('Error flushing SoC updates before background:', error);
      }
    }
    
    appStateRef.current = nextAppState;
  }, [isScanning, bleService]);

  useEffect(() => {
    const subscription = AppState.addEventListener('change', handleAppStateChange);
    
    return () => {
      subscription?.remove();
    };
  }, [handleAppStateChange]);

  return {
    devices,
    isScanning,
    isConnecting,
    startScan,
    stopScan,
    connectToDevice,
    removeDevice,
    disconnectDevice,
    setDevices,
    updateDeviceSoC,
    loadCachedDevices,
  };
};
