import {
  BleManager,
  Device,
  State as BluetoothState,
  Subscription,
  Characteristic,
  BleError,
  ScanMode,
  ScanCallbackType,
} from "react-native-ble-plx";
import { Linking, PermissionsAndroid, Platform, AppState, AppStateStatus } from "react-native";
import { Logger } from "../components/Logger";
import { Buffer } from "buffer";
// Adjust if needed
import { CP_SERVICE } from "./BLEConstants";
import { NativeModules } from 'react-native';
const { BleBondModule } = NativeModules;


class BLEService {
  private static instance: BLEService;
  private manager: BleManager;
  private devices: Map<string, Device>;
  private otaDeviceName = "TouchGridOTA";
  private stateChangeSubscription: Subscription | null = null;
  private appStateSubscription: any = null;
  private seenDevices: Set<string> = new Set();
  private scanListeners: Array<(device: Device) => void> = [];
  private scanRequestCount: number = 0; // Track how many components want scanning active

  private isScanning: boolean = false;
  private scanRetryCount: number = 0;
  private maxScanRetries: number = 3;
  private scanRetryDelay: number = 1000;
  private shouldBeScanning: boolean = false; // Track if we should be scanning
  private scanRestartTimer: NodeJS.Timeout | null = null;

  private lastUpdateTime: Map<string, number> = new Map();
  private readonly MIN_UPDATE_INTERVAL = 1000; // Minimum 1 second between updates for the same device

  /**
   * Private constructor to implement Singleton.
   */
  private constructor() {
    this.manager = new BleManager();
    this.devices = new Map();
    this.monitorBluetoothState();
    this.monitorAppState();
  }

  /**
   * Singleton accessor.
   */
  public static getInstance(): BLEService {
    if (!BLEService.instance) {
      BLEService.instance = new BLEService();
    }
    return BLEService.instance;
  }

  public getManager(): BleManager {
    return this.manager;
  }

  /**
   * Watch for Bluetooth state changes, but do NOT auto-scan
   * when PoweredOn (to avoid duplicate scans).
   */
  private monitorBluetoothState() {
    this.stateChangeSubscription = this.manager.onStateChange((state) => {
      console.log("Bluetooth state changed:", state);
      this.handleBluetoothStateChange(state);
    }, true);
  }

  private handleBluetoothStateChange(state: BluetoothState) {
    switch (state) {
      case BluetoothState.PoweredOn:
        console.log("Bluetooth is powered on");
        // Restart scanning if we should be scanning
        if (this.shouldBeScanning && !this.isScanning) {
          console.log('Restarting scan after Bluetooth powered on');
          this.restartScanIfNeeded();
        }
        break;

      case BluetoothState.PoweredOff:
        console.log("Bluetooth is powered off");
        this.devices.clear();
        // Stop any ongoing scan when Bluetooth is turned off
        this.stopDeviceScan();
        break;

      case BluetoothState.Unauthorized:
        console.error("Bluetooth is unauthorized");
        break;

      case BluetoothState.Unsupported:
        console.error("Bluetooth is unsupported");
        break;

      case BluetoothState.Resetting:
        console.log("Bluetooth is resetting");
        break;

      case BluetoothState.Unknown:
      default:
        break;
    }
  }

  /**
   * Restart scanning if conditions are met
   */
  private async restartScanIfNeeded() {
    if (!this.shouldBeScanning || this.isScanning) {
      return;
    }

    try {
      const currentState = await this.manager.state();
      if (currentState === BluetoothState.PoweredOn) {
        console.log('Conditions met, restarting scan...');
        this.scanRetryCount = 0; // Reset retry count
        this.performScan({ allowDuplicates: true });
      } else {
        console.log('Bluetooth not ready for scanning, state:', currentState);
      }
    } catch (error) {
      console.error('Error checking Bluetooth state for restart:', error);
    }
  }

  /**
   * Subscribe/unsubscribe to device discoveries during scanning.
   */
  public subscribeToDeviceDiscoveries(listener: (device: Device) => void) {
    if (!this.scanListeners.includes(listener)) {
      this.scanListeners.push(listener);
    }
  }

  public unsubscribeFromDeviceDiscoveries(listener: (device: Device) => void) {
    this.scanListeners = this.scanListeners.filter((l) => l !== listener);
  }

  /**
   * Request Bluetooth permission on Android; on iOS, we just do
   * a quick check if it's "Unauthorized." If so, return false.
   * Otherwise assume permission is granted or will be prompted
   * automatically on usage.
   */
  public async requestBluetoothPermission(): Promise<boolean> {
    if (Platform.OS === "android") {
      const permissions = [
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      ];
      // For Android < 12, also need fine location
      if (parseInt(Platform.Version.toString(), 10) < 31) {
        permissions.push(PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION);
      }

      const result = await PermissionsAndroid.requestMultiple(permissions);
      return Object.values(result).every(
        (status) => status === PermissionsAndroid.RESULTS.GRANTED
      );
    } else if (Platform.OS === "ios") {
      // iOS: let's see if it's unauthorized
      try {
        const currentState = await this.manager.state();
        console.log("Bluetooth status (iOS check):", currentState);

        if (currentState === BluetoothState.Unauthorized) {
          return false;
        }
        return true;
      } catch (err) {
        console.error("Error checking iOS Bluetooth state:", err);
        return false;
      }
    }
    // For other platforms (web?), assume true
    return true;
  }

  /**
   * Attach a callback for when device is disconnected.
   */
  public onDeviceDisconnected(
    deviceId: string,
    callback: (error: BleError | null, device: Device | null) => void
  ): Subscription | undefined {
    const device = this.getDevice(deviceId);
    if (!device) {
      console.error(`Device ${deviceId} not found`);
      return undefined;
    }

    return device.onDisconnected((error, disconnectedDevice) => {
      Logger.info(`Device ${deviceId} disconnected`);
      callback(error ?? null, disconnectedDevice ?? null);
    });
  }

  /**
   * Initialize BLE: ensures permissions are granted or throws "NO_PERMISSIONS".
   * We do NOT wait here for "PoweredOn" - your scanning code
   * can handle "PoweredOff" case with a user alert.
   */
  public async initializeBLE(): Promise<void> {
    const hasPermissions = await this.requestBluetoothPermission();
    if (!hasPermissions) {
      throw new Error("NO_PERMISSIONS");
    }
    // no further checks here; just return
    return Promise.resolve();
  }

  /**
   * Open OS settings (Android or iOS).
   */
  public openBluetoothSettings(): void {
    Linking.openSettings();
  }

  /**
   * Log discovered service UUIDs.
   */
  private logAdvertisedServices(device: Device) {
    const services = device.serviceUUIDs;
    if (services) {
      console.log(
        "Advertised services for device",
        device.localName || device.id,
        ":",
        services
      );
    }
  }

  /**
   * Log manufacturer data in hex for debugging.
   */
  private logManufacturerData(device: Device) {
    const { manufacturerData } = device;
    if (manufacturerData) {
      const rawBuffer = Buffer.from(manufacturerData, "base64");
      const hexString = rawBuffer.toString("hex").toUpperCase();
      console.log("Manufacturer data:", hexString);
    }
  }

  /**
   * Parse the manufacturerData to extract a serial number and state of charge.
   * If either is known to be a 'dummy' value, set it to `"-"`.
   * Otherwise, parse normally. If no valid serial, return empty string to skip device.
   */
  private parseDeviceInfo(device: Device): {
    serialNumber: string;
    stateOfCharge: string;
  } {
    const { manufacturerData } = device;
    if (!manufacturerData) {
      return { serialNumber: "", stateOfCharge: "" };
    }

    const raw = Buffer.from(manufacturerData, "base64");
    // Must have at least 10 bytes: 2 to skip + 4 for serial + 4 for SoC
    if (raw.length < 10) {
      return { serialNumber: "", stateOfCharge: "" };
    }

    // -- parse serial number (4 bytes after first 2)
    const serialBytes = raw.slice(2, 6);
    const serialReversed = Buffer.from(serialBytes).reverse();
    const serialReversedHex = serialReversed.toString("hex").toLowerCase();

    // Check for dummy variants "ddccbbaa" or "aabbccdd"
    if (serialReversedHex === "ddccbbaa" || serialReversedHex === "aabbccdd") {
      return { serialNumber: "-", stateOfCharge: "-" };
    }

    const serialDecimal = parseInt(serialReversedHex, 16);
    if (serialDecimal === 0) {
      return { serialNumber: "", stateOfCharge: "" };
    }

    const serialStr = serialDecimal.toString();
    if (serialStr.length < 5) {
      // Not enough digits to form a "XXXX-XXXX" pattern
      return { serialNumber: "", stateOfCharge: "" };
    }

    // "XXXX-XXXX" style
    const splitIndex = serialStr.length - 4;
    let serialNumber = serialStr.slice(0, splitIndex) + "-" + serialStr.slice(splitIndex);

    // -- parse state of charge (next 4 bytes)
    const socBytes = raw.slice(6, 10);
    const socReversed = Buffer.from(socBytes).reverse();
    const socHex = socReversed.toString("hex").toLowerCase();

    // Check for dummy SoC "40302010" or reversed "10203040"
    if (socHex === "40302010" || socHex === "10203040") {
      return { serialNumber, stateOfCharge: "-" };
    }

    const socIntVal = parseInt(socHex, 16);
    // Convert to % by dividing by 65536 then multiply by 100
    const percentage = Math.floor((socIntVal / 65536.0) * 100);

    return {
      serialNumber,
      stateOfCharge: percentage.toString(),
    };
  }

  /**
   * Determine if we should add a device by checking service UUIDs or localName for "Clayton".
   */
  private shouldAddDevice(device: Device): boolean {
    const services = device.serviceUUIDs;
    if (
      (services && services.includes(CP_SERVICE.CLAYTON)) ||
      device.localName?.includes("Clayton")
    ) {
      return true;
    }
    return false;
  }

  /**
   * Request scanning to be active. Uses reference counting - scanning stays active
   * as long as at least one component has requested it.
   * Returns true if scanning was started, false if already active.
   */
  public requestScanning(
    maxRetries: number = 5, // Increased from 3 for more persistence
    retryDelay: number = 500, // Reduced from 2000ms for faster retries
    options: { 
      allowDuplicates?: boolean;
      scanMode?: ScanMode;
      callbackType?: ScanCallbackType;
    } = { 
      allowDuplicates: true, // Changed from false - more aggressive
      scanMode: ScanMode.LowLatency, // Most aggressive scan mode (Android)
      callbackType: ScanCallbackType.AllMatches, // Report all matches (Android)
    }
  ): boolean {
    this.scanRequestCount++;
    
    if (this.isScanning) {
      Logger.debug(`Scanning already active (${this.scanRequestCount} requests).`);
      return false;
    }
    
    this.shouldBeScanning = true;
    this.scanRetryCount = 0;
    this.maxScanRetries = maxRetries;
    this.scanRetryDelay = retryDelay;
    
    // Clear any existing restart timer
    if (this.scanRestartTimer) {
      clearTimeout(this.scanRestartTimer);
      this.scanRestartTimer = null;
    }
    
    this.performScan(options);
    return true;
  }

  /**
   * @deprecated Use requestScanning() instead. This method is kept for backward compatibility.
   */
  public startScanning(
    maxRetries: number = 3,
    retryDelay: number = 2000,
    options: { allowDuplicates?: boolean } = { allowDuplicates: true }
  ) {
    this.requestScanning(maxRetries, retryDelay, options);
  }

  private performScan(options: { 
    allowDuplicates?: boolean;
    scanMode?: ScanMode;
    callbackType?: ScanCallbackType;
  }) {
    if (this.isScanning) {
      Logger.debug("Already scanning, ignoring performScan call.");
      return;
    }
    
    if (!this.shouldBeScanning) {
      Logger.debug("Should not be scanning, aborting performScan.");
      return;
    }
    
    this.isScanning = true;
    Logger.debug("Starting BLE device scan with Clayton service UUID...");

    // Use service UUID for more reliable scanning, especially on iOS
    // This also improves battery efficiency by only scanning for relevant devices
    this.manager.startDeviceScan(
      [CP_SERVICE.CLAYTON],
      options,
      async (error, device) => {
        if (error) {
          Logger.error("Error during device scan:", error);
          this.isScanning = false;

          if (!this.shouldBeScanning) {
            Logger.debug("Should not be scanning, not retrying.");
            return;
          }

          if (this.scanRetryCount < this.maxScanRetries) {
            this.scanRetryCount++;
            Logger.debug(
              `Scan error. Retrying scan (${this.scanRetryCount}/${this.maxScanRetries}) in ${this.scanRetryDelay}ms...`
            );
            setTimeout(() => {
              if (this.shouldBeScanning) {
                this.performScan(options);
              }
            }, this.scanRetryDelay);
          } else {
            Logger.error("Maximum scan retries reached. Setting up restart timer...");
            // Set up a longer restart timer instead of giving up completely
            this.scanRestartTimer = setTimeout(() => {
              if (this.shouldBeScanning && !this.isScanning) {
                Logger.debug("Attempting to restart scan after extended delay...");
                this.scanRetryCount = 0; // Reset retry count
                this.performScan(options);
              }
            }, 10000); // Try again after 10 seconds (reduced from 30 for faster recovery)
          }
          return;
        }

        if (device && this.shouldAddDevice(device)) {
          const parsedInfo = this.parseDeviceInfo(device);
          const enrichedDevice = {
            deviceId: device.id,
            name: device.name || device.localName || "",
            serialNumber: parsedInfo.serialNumber,
            stateOfCharge: parsedInfo.stateOfCharge,
          };

          // Notify all listeners with the enriched device info
          this.scanListeners.forEach((listener) => listener(enrichedDevice as any));
        }
      }
    );
  }

  /**
   * Release a scanning request. Scanning will only stop when all requests are released.
   * Returns true if scanning was stopped, false if other components still want it active.
   */
  public releaseScanning(): boolean {
    if (this.scanRequestCount > 0) {
      this.scanRequestCount--;
    }
    
    if (this.scanRequestCount > 0) {
      Logger.debug(`Scanning still requested by ${this.scanRequestCount} component(s).`);
      return false;
    }
    
    // No more requests, stop scanning
    this.manager.stopDeviceScan();
    this.isScanning = false;
    this.shouldBeScanning = false;
    this.seenDevices.clear();
    this.lastUpdateTime.clear();
    
    // Clear any pending restart timer
    if (this.scanRestartTimer) {
      clearTimeout(this.scanRestartTimer);
      this.scanRestartTimer = null;
    }
    
    Logger.debug("Scanning stopped - no more requests.");
    return true;
  }

  /**
   * @deprecated Use releaseScanning() instead. This method is kept for backward compatibility.
   */
  public stopDeviceScan(): void {
    this.releaseScanning();
  }

  /**
   * Check if scanning is currently active.
   */
  public isScanningActive(): boolean {
    return this.isScanning;
  }

  /**
   * Destroy BLE manager and clean up.
   */
  public destroy(): void {
    this.manager.destroy();
    this.devices.clear();
    this.shouldBeScanning = false;
    
    if (this.stateChangeSubscription) {
      this.stateChangeSubscription.remove();
      this.stateChangeSubscription = null;
    }
    
    if (this.appStateSubscription) {
      this.appStateSubscription.remove();
      this.appStateSubscription = null;
    }
    
    if (this.scanRestartTimer) {
      clearTimeout(this.scanRestartTimer);
      this.scanRestartTimer = null;
    }
  }

  /**
   * Connect to a device with retries. After connecting, discover services/characteristics.
   */
  public async connectToDevice(
    deviceId: string,
    attempts = 3,
    delay = 1000
  ): Promise<void> {
    return new Promise<void>(async (resolve, reject) => {
      for (let i = 0; i < attempts; i++) {
        try {
          await this.connectToDeviceWithTimeout(deviceId, 10000);
          resolve(); // success
          return;
        } catch (error) {
          console.error(`Connection attempt ${i + 1} failed:`, error);

          // 🛑 Stop retrying if bonding was declined
          if (
            (error instanceof Error && error.name === "BondingDeclined") ||
            (typeof error === 'object' && error !== null && (error as any).code === 'BONDING_DECLINED')
          ) {
            Logger.warn("User declined bonding — aborting retries.");
            reject(error);
            return;
          }

          if (i < attempts - 1) {
            await new Promise((res) => setTimeout(res, delay));
          } else {
            reject(error);
          }
        }
      }
    });
  }


  /**
   * Disconnect from a device if connected.
   */
  public async disconnectFromDevice(deviceId: string): Promise<void> {
    try {
      const isDeviceConnected = await this.manager.isDeviceConnected(deviceId);
      if (!isDeviceConnected) {
        console.log("Device is already disconnected");
        return;
      }
      await this.manager.cancelDeviceConnection(deviceId);
      this.devices.delete(deviceId);
    } catch (error) {
      console.error("Disconnection error:", error);
      throw error;
    }
  }

  /**
   * Get a currently stored device from the Map.
   */
  public getDevice(deviceId: string): Device | undefined {
    return this.devices.get(deviceId);
  }

  /**
   * Request a specific MTU size (Android only).
   */
  public requestMTU(deviceId: string, mtu: number): Promise<number> {
    return new Promise<number>(async (resolve, reject) => {
      try {
        const device = this.getDevice(deviceId);
        if (!device) {
          throw new Error("Device not found");
        }
        await device.requestMTU(mtu);
        resolve(mtu);
      } catch (error) {
        console.error("Error requesting MTU:", error);
        reject(error);
      }
    });
  }

  /**
   * Log discovered services and their characteristics (for debugging).
   */
  private async logServices(device: Device) {
    const services = await device.services();
    for (const service of services) {
      console.log("Service:", service.uuid);
      const characteristics = await device.characteristicsForService(service.uuid);
      for (const characteristic of characteristics) {
        console.log(
          "  Characteristic:",
          characteristic.uuid,
          characteristic.isReadable ? "Readable" : "",
          characteristic.isWritableWithResponse ? "WritableWithResponse" : "",
          characteristic.isWritableWithoutResponse ? "WritableWithoutResponse" : "",
          characteristic.isNotifiable ? "Notifiable" : ""
        );
      }
    }
  }

  /**
   * Read from a characteristic and return the base64 value.
   */
  public async readFromDevice(
    deviceId: string,
    serviceUUID: string,
    characteristicUUID: string
  ): Promise<string> {
    try {
      const dev = this.getDevice(deviceId);
      if (!dev) {
        throw new Error("Device not found");
      }
      const characteristic = await dev.readCharacteristicForService(serviceUUID, characteristicUUID);
      if (characteristic.value === null) {
        throw new Error("Characteristic value is null");
      }
      return characteristic.value;
    } catch (error) {
      throw error;
    }
  }

  /**
   * Write a base64-encoded string to a characteristic with response.
   */
  public async writeToDevice(
    deviceId: string,
    serviceUUID: string,
    characteristicUUID: string,
    data: string
  ): Promise<void> {
    try {
      const dev = this.getDevice(deviceId);
      if (!dev) {
        console.error("Device not found");
        return;
      }
      const services = await dev.services();
      const service = services.find((s) => s.uuid === serviceUUID);
      if (!service) {
        console.error("Service not found " + serviceUUID);
        return;
      }
      console.log("Writing data to device:", data);
      await dev.writeCharacteristicWithResponseForService(
        serviceUUID,
        characteristicUUID,
        data
      );
    } catch (error) {
      console.error("Error during write operation:", error);
    }
  }

  /**
   * Write a base64-encoded string to a characteristic without response.
   */
  public async writeToDeviceWithoutResponse(
    deviceId: string,
    serviceUUID: string,
    characteristicUUID: string,
    data: string
  ): Promise<void> {
    try {
      const dev = this.getDevice(deviceId);
      if (!dev) {
        console.error("Device not found");
        return;
      }
      const services = await dev.services();
      const service = services.find((s) => s.uuid === serviceUUID);
      if (!service) {
        console.error("Service not found " + serviceUUID);
        return;
      }
      console.log("Writing data to device without response:", data);
      await dev.writeCharacteristicWithoutResponseForService(
        serviceUUID,
        characteristicUUID,
        data
      );
    } catch (error) {
      console.error("Error during write operation:", error);
    }
  }

  public async connectToDeviceWithTimeout(
    deviceId: string,
    timeoutMs: number
  ): Promise<void> {
    return new Promise<void>((resolve, reject) => {
      let timeoutHandle: NodeJS.Timeout | null = null;

      const timeoutPromise = new Promise<void>((_, timeoutReject) => {
        timeoutHandle = setTimeout(() => {
          timeoutReject(new Error("Connection timeout"));
          this.manager.cancelDeviceConnection(deviceId).catch(() => { });
        }, timeoutMs);
      });

      const connectPromise = this.manager
        .connectToDevice(deviceId)
        .then(async (device) => {
          await device.discoverAllServicesAndCharacteristics();
          this.devices.set(deviceId, device);

          if (Platform.OS === "android") {
            let isBonded = await this.isDeviceBonded(deviceId);

            if (!isBonded) {
              await this.requestBondAndroid(deviceId);

              const maxWait = 15000;
              const interval = 1000;
              let waited = 0;

              while (!isBonded && waited < maxWait) {
                await new Promise((res) => setTimeout(res, interval));
                isBonded = await this.isDeviceBonded(deviceId);
                waited += interval;
              }

              if (!isBonded) {
                // 🛑 bonding never happened — treat as "declined"
                const err = new Error("User declined bonding or bonding timed out");
                err.name = "BondingDeclined";
                throw err;
              }

              Logger.debug(`Bonding completed for device ${deviceId}`);
            }
          }
        });

      Promise.race([connectPromise, timeoutPromise])
        .then(() => {
          if (timeoutHandle) clearTimeout(timeoutHandle);
          resolve();
        })
        .catch((err) => {
          const bleError = err as BleError;

          Logger.error({
            errorCode: bleError?.errorCode,
            message: bleError?.message,
            attErrorCode: bleError?.attErrorCode,
            iosErrorCode: bleError?.iosErrorCode,
            androidErrorCode: bleError?.androidErrorCode,
            reason: bleError?.reason,
          });

          if (timeoutHandle) clearTimeout(timeoutHandle);
          reject(err);
        });
    });
  }

  /**
   * Setup notifications on a characteristic.
   */
  public async setupNotifications(
    deviceId: string,
    serviceUUID: string,
    characteristicUUID: string,
    onNotificationReceived: (
      error: Error | null,
      characteristic: Characteristic | null
    ) => void
  ): Promise<Subscription> {
    Logger.debug("Setting up notifications for device:", deviceId);
    try {
      const dev = this.getDevice(deviceId);
      if (!dev) {
        throw new Error("Device not found");
      }
      return dev.monitorCharacteristicForService(
        serviceUUID,
        characteristicUUID,
        onNotificationReceived
      );
    } catch (error) {
      console.error("Error setting up notifications:", error);
      throw error;
    }
  }

  /**
   * Check if a device is currently connected.
   */
  public async isDeviceConnected(deviceId: string): Promise<boolean> {
    try {
      return await this.manager.isDeviceConnected(deviceId);
    } catch (error) {
      console.log("Error checking device connection:", error);
      return false;
    }
  }

  public isBondingIssue(error: BleError): boolean {
    const peerRemovedPairingCode = 200;
    const peerRemovedPairingIOSCode = 14;
    const phoneRemovedPairingCode = 201;
    const phoneRemovedPairingIOSCode = 7;
    const attErrorCode = 62;

    if (
      error.errorCode === peerRemovedPairingCode &&
      (error.iosErrorCode as number) === peerRemovedPairingIOSCode
    ) {
      return true;
    }

    if (
      error.errorCode === phoneRemovedPairingCode &&
      (error.iosErrorCode as number) === phoneRemovedPairingIOSCode
    ) {
      return true;
    }

    if (
      error.errorCode === peerRemovedPairingCode &&
      (error.attErrorCode as number) === attErrorCode
    ) {
      return true;
    }

    if (
      error.errorCode === phoneRemovedPairingCode &&
      (error.attErrorCode as number) === attErrorCode
    ) {
      return true;
    }

    return false;
  }

  public async isDeviceBonded(deviceId: string): Promise<boolean> {
    if (Platform.OS !== 'android') {
      // iOS handles bonding internally
      return true;
    }

    try {
      const result: boolean = await BleBondModule.isDeviceBonded(deviceId);
      Logger.debug(`Bonded state for ${deviceId}: ${result}`);
      return result;
    } catch (error) {
      Logger.error(`Error checking bond state for ${deviceId}:`, error);
      return false;
    }
  }

  private async requestBondAndroid(deviceId: string): Promise<void> {
    if (Platform.OS !== 'android') return;

    try {
      await BleBondModule.requestBond(deviceId);
      Logger.debug(`Bonding completed for ${deviceId}`);
    } catch (e: any) {
      if (e.code === 'BONDING_DECLINED') {
        const err = new Error("User declined bonding");
        err.name = "BondingDeclined";
        throw err;
      }
      Logger.error("Bonding failed:", e);
      throw e;
    }
  }

  /**
   * Monitor app state changes to handle background/foreground transitions
   */
  private monitorAppState() {
    this.appStateSubscription = AppState.addEventListener('change', this.handleAppStateChange.bind(this));
  }

  /**
   * Handle app state changes
   */
  private handleAppStateChange(nextAppState: AppStateStatus) {
    console.log('App state changed to:', nextAppState);
    
    if (nextAppState === 'active') {
      // App came to foreground
      console.log('App came to foreground');
      if (this.shouldBeScanning && !this.isScanning) {
        console.log('Restarting scan after coming to foreground');
        this.restartScanIfNeeded();
      }
    } else if (nextAppState === 'background') {
      // App went to background
      console.log('App went to background');
      // On iOS, scanning automatically stops in background
      // On Android, we might want to continue scanning but with reduced frequency
      if (Platform.OS === 'ios' && this.isScanning) {
        this.isScanning = false; // iOS stops scanning automatically
      }
    }
  }

}

// Export the singleton instance
export const bleService = BLEService.getInstance();
