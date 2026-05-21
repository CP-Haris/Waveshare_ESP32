import { BleManager, ConnectionPriority } from 'react-native-ble-plx';
import {
  BLE_SERVICE_UUID,
  BLE_TX_CHAR_UUID,
  BLE_RX_CHAR_UUID,
  decodeNotification,
} from '../utils/protocol';

class BleService {
  constructor() {
    this.manager = new BleManager();
    this.device = null;
    this.listeners = new Set();
    this.connectionListeners = new Set();
    this.scanning = false;
    this._subscription = null;
    this._commandLockToken = null;
    this._commandLockOwner = null;
  }

  acquireCommandLock(owner = 'unknown') {
    if (this._commandLockToken) return null;
    const token = `${owner}:${Date.now()}:${Math.random().toString(16).slice(2)}`;
    this._commandLockToken = token;
    this._commandLockOwner = owner;
    return token;
  }

  releaseCommandLock(token) {
    if (token && this._commandLockToken === token) {
      this._commandLockToken = null;
      this._commandLockOwner = null;
    }
  }

  clearCommandLock() {
    this._commandLockToken = null;
    this._commandLockOwner = null;
  }

  onNotification(fn) {
    this.listeners.add(fn);
    return () => this.listeners.delete(fn);
  }

  onConnectionChange(fn) {
    this.connectionListeners.add(fn);
    return () => this.connectionListeners.delete(fn);
  }

  _emitConnection(connected) {
    this.connectionListeners.forEach((fn) => fn(connected));
  }

  _emitNotification(msg) {
    this.listeners.forEach((fn) => fn(msg));
  }

  async scan(timeoutMs = 5000) {
    const devices = [];
    return new Promise((resolve) => {
      this.scanning = true;
      this.manager.startDeviceScan(
        [BLE_SERVICE_UUID],
        { allowDuplicates: false },
        (error, device) => {
          if (error) {
            console.warn('[BLE] Scan error:', error.message);
            return;
          }
          if (device && !devices.find((d) => d.id === device.id)) {
            devices.push({
              id: device.id,
              name: device.name || device.localName || 'Unknown',
              rssi: device.rssi,
            });
          }
        }
      );
      setTimeout(() => {
        this.manager.stopDeviceScan();
        this.scanning = false;
        resolve(devices);
      }, timeoutMs);
    });
  }

  stopScan() {
    this.manager.stopDeviceScan();
    this.scanning = false;
  }

  async connect(deviceId) {
    try {
      const device = await this.manager.connectToDevice(deviceId, {
        requestMTU: 256,
      });
      this.device = device;

      try {
        await device.requestConnectionPriority(ConnectionPriority.High);
      } catch (error) {
        console.warn('[BLE] Connection priority request failed:', error.message);
      }

      device.onDisconnected((error) => {
        if (error?.message) console.warn('[BLE] Disconnected:', error.message);
        this.device = null;
        this._subscription?.remove();
        this._subscription = null;
        this.clearCommandLock();
        this._emitConnection(false);
      });

      await device.discoverAllServicesAndCharacteristics();

      this._subscription = device.monitorCharacteristicForService(
        BLE_SERVICE_UUID,
        BLE_TX_CHAR_UUID,
        (error, characteristic) => {
          if (error) {
            console.warn('[BLE] Notify error:', error.message);
            return;
          }
          if (characteristic?.value) {
            try {
              const msg = decodeNotification(characteristic.value);
              this._emitNotification(msg);
            } catch (e) {
              console.warn('[BLE] Decode error:', e.message);
            }
          }
        }
      );

      this._emitConnection(true);
      return true;
    } catch (error) {
      console.error('[BLE] Connect error:', error.message);
      this.device = null;
      this._emitConnection(false);
      return false;
    }
  }

  async disconnect() {
    if (this.device) {
      try {
        await this.device.cancelConnection();
      } catch (e) {
        // device may already be disconnected
      }
    }
    this.device = null;
    this._subscription?.remove();
    this._subscription = null;
    this.clearCommandLock();
    this._emitConnection(false);
  }

  async writeCommand(base64Data, commandLockToken = null) {
    if (!this.device) return false;
    if (this._commandLockToken && commandLockToken !== this._commandLockToken) {
      return false;
    }
    try {
      await this.device.writeCharacteristicWithoutResponseForService(
        BLE_SERVICE_UUID,
        BLE_RX_CHAR_UUID,
        base64Data
      );
      return true;
    } catch (error) {
      console.warn('[BLE] Write error:', error.message);
      return false;
    }
  }

  async writeCommandWithResponse(base64Data, commandLockToken = null) {
    if (!this.device) return false;
    if (this._commandLockToken && commandLockToken !== this._commandLockToken) {
      return false;
    }

    try {
      await this.device.writeCharacteristicWithResponseForService(
        BLE_SERVICE_UUID,
        BLE_RX_CHAR_UUID,
        base64Data
      );
      return true;
    } catch (error) {
      // Some firmware revisions only expose write-without-response on RX.
      console.warn('[BLE] Write-with-response error, retrying without response:', error.message);
      try {
        await this.device.writeCharacteristicWithoutResponseForService(
          BLE_SERVICE_UUID,
          BLE_RX_CHAR_UUID,
          base64Data
        );
        return true;
      } catch (fallbackError) {
        console.warn('[BLE] Write fallback error:', fallbackError.message);
        return false;
      }
    }
  }

  get isConnected() {
    return this.device !== null;
  }

  get commandLockOwner() {
    return this._commandLockOwner;
  }

  getConnectedDeviceInfo() {
    if (!this.device) return null;
    return {
      id: this.device.id,
      name: this.device.name || this.device.localName || 'Unknown',
      mtu: this.device.mtu,
    };
  }

  destroy() {
    this.disconnect();
    this.clearCommandLock();
    this.manager.destroy();
  }
}

export default new BleService();
