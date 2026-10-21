import { BleManager, ConnectionPriority } from 'react-native-ble-plx';
import { GATT, SCAN_SERVICE_UUIDS, kindFromServiceUuids, parseLps2Advert, DEVICE_KIND } from './gattProfiles';

// Time allowed for the user to read the PIN off the display and type it in.
const PAIRING_TIMEOUT_MS = 45000;
// A remembered device is already bonded, so authentication is quick.
const RECONNECT_AUTH_TIMEOUT_MS = 15000;
const PAIRING_RETRY_MS = 1000;

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

/**
 * The BLE link: scanning, connecting (including waiting for PIN pairing),
 * one notify stream in and writes out. It knows the GATT layout of each chip
 * but nothing about what the bytes mean — that is the device drivers' job
 * (src/devices/*).
 */
class BleLink {
  constructor() {
    this.manager = new BleManager();
    this.device = null;
    this.kind = null;
    this.dataListeners = new Set();
    this.connectionListeners = new Set();
    this._subscription = null;
    this._pendingId = null; // waiting for a remembered device to come in range
    this._attempt = 0; //      id of the current connection attempt
    this._connecting = false;
    this._connectingId = null;
    this._disconnectSubscription = null;
    this._commandLockToken = null;
    this._commandLockOwner = null;
  }

  // --- Exclusive command lock (used by the firmware update) ---

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

  get commandLockOwner() {
    return this._commandLockOwner;
  }

  // --- Listeners ---

  /** Raw notifications from the connected chip, as base64 strings. */
  onData(fn) {
    this.dataListeners.add(fn);
    return () => this.dataListeners.delete(fn);
  }

  onConnectionChange(fn) {
    this.connectionListeners.add(fn);
    return () => this.connectionListeners.delete(fn);
  }

  _emitConnection(connected) {
    this.connectionListeners.forEach((fn) => fn(connected));
  }

  // --- Scan ---

  /** Both chip types; LPS2 adverts also carry serial and SoC. */
  async scan(timeoutMs = 5000) {
    const devices = [];
    return new Promise((resolve) => {
      this.manager.startDeviceScan(SCAN_SERVICE_UUIDS, { allowDuplicates: false }, (error, device) => {
        if (error) {
          console.warn('[BLE] Scan error:', error.message);
          return;
        }
        if (!device || devices.find((d) => d.id === device.id)) return;
        const kind = kindFromServiceUuids(device.serviceUUIDs);
        devices.push({
          id: device.id,
          name: device.name || device.localName || 'Unknown',
          rssi: device.rssi,
          kind,
          ...(kind === DEVICE_KIND.LPS2 ? parseLps2Advert(device.manufacturerData) : {}),
        });
      });
      setTimeout(() => {
        this.manager.stopDeviceScan();
        resolve(devices);
      }, timeoutMs);
    });
  }

  // --- Connect ---
  //
  // One connection attempt at a time. Every attempt (connect, reconnect,
  // cancel, disconnect) bumps `_attempt`; an attempt that has been replaced
  // by a newer one never touches the link again — no adopt, no cancel, no
  // teardown — so a cancelled reconnect cannot kill the connection that
  // replaced it.

  _nextAttempt() {
    this._attempt += 1;
    return this._attempt;
  }

  _assertCurrent(attempt) {
    if (attempt !== this._attempt) throw new Error('Superseded by a newer connection attempt');
  }

  /** Stop whatever attempt is running (and its pending OS-level connect). */
  async _abortAttempt() {
    this._nextAttempt();
    const ids = [this._pendingId, this._connectingId];
    this._pendingId = null;
    this._connectingId = null;
    this._connecting = false;
    for (const id of new Set(ids)) {
      if (!id || id === this.device?.id) continue;
      try {
        await this.manager.cancelDeviceConnection(id);
      } catch (error) {
        // nothing in progress
      }
    }
  }

  /**
   * Open a GATT link, reusing one Android still holds for this device (e.g.
   * from an earlier app instance) instead of failing with "already connected".
   */
  async _open(deviceId, options) {
    if (await this.manager.isDeviceConnected(deviceId)) {
      const [device] = await this.manager.devices([deviceId]);
      if (device) return device;
    }
    return this.manager.connectToDevice(deviceId, options);
  }

  /**
   * Connect and wait until the link is usable. `authenticate(device, gatt)`
   * comes from the driver and must resolve once the chip answers over an
   * encrypted link; it throws while Android's PIN dialog is still open, so
   * keep retrying (and reconnecting if the link drops) until the deadline.
   */
  async _connectAndAuthenticate(attempt, deviceId, gatt, authenticate) {
    const deadline = Date.now() + PAIRING_TIMEOUT_MS;
    let lastError = null;

    while (Date.now() < deadline) {
      this._assertCurrent(attempt);
      try {
        const device = await this._open(deviceId, { requestMTU: 256 });
        await device.discoverAllServicesAndCharacteristics();

        while (Date.now() < deadline) {
          this._assertCurrent(attempt);
          try {
            await authenticate(device, gatt);
            return device;
          } catch (authError) {
            lastError = authError;
            if (!(await device.isConnected())) break; // link dropped: reconnect
            await sleep(PAIRING_RETRY_MS);
          }
        }
      } catch (error) {
        lastError = error;
      }

      this._assertCurrent(attempt);
      try {
        await this.manager.cancelDeviceConnection(deviceId);
      } catch (cancelError) {
        // already disconnected
      }
      await sleep(PAIRING_RETRY_MS);
    }

    throw lastError || new Error('Pairing timed out');
  }

  async connect(deviceId, kind, authenticate) {
    await this._abortAttempt();
    // Mark the attempt before dropping an old link, so the disconnect event
    // that follows does not start an automatic reconnect in between.
    const attempt = this._nextAttempt();
    this._connecting = true;
    this._connectingId = deviceId;
    const previous = this.device;
    if (previous) {
      this._teardown();
      try {
        await previous.cancelConnection();
      } catch (error) {
        // already gone
      }
    }
    try {
      const device = await this._connectAndAuthenticate(attempt, deviceId, GATT[kind], authenticate);
      this._assertCurrent(attempt);
      await this._adopt(device, kind);
      return true;
    } catch (error) {
      console.warn('[BLE] Connect failed:', error.message);
      return false;
    } finally {
      if (attempt === this._attempt) {
        this._connecting = false;
        this._connectingId = null;
      }
    }
  }

  /**
   * Reconnect to a remembered (already bonded) device. Uses Android's
   * autoConnect: the request waits, at low power, until the device is in
   * range — no scanning needed. Resolves false if cancelled or failed.
   */
  async connectWhenInRange(deviceId, kind, authenticate) {
    await this._abortAttempt();
    if (this.device) return false; // already connected; nothing to wait for
    const attempt = this._nextAttempt();
    const gatt = GATT[kind];
    this._pendingId = deviceId;
    try {
      let device = await this._open(deviceId, { autoConnect: true });
      this._assertCurrent(attempt);
      this._pendingId = null;
      await device.discoverAllServicesAndCharacteristics();
      try {
        device = await device.requestMTU(256);
      } catch (mtuError) {
        // keep the default MTU
      }

      const deadline = Date.now() + RECONNECT_AUTH_TIMEOUT_MS;
      for (;;) {
        this._assertCurrent(attempt);
        try {
          await authenticate(device, gatt);
          break;
        } catch (authError) {
          if (Date.now() > deadline || !(await device.isConnected())) throw authError;
          await sleep(PAIRING_RETRY_MS);
        }
      }

      this._assertCurrent(attempt);
      await this._adopt(device, kind);
      return true;
    } catch (error) {
      if (attempt !== this._attempt) return false; // replaced: leave the link alone
      console.warn('[BLE] Reconnect failed:', error.message);
      this._pendingId = null;
      try {
        await this.manager.cancelDeviceConnection(deviceId);
      } catch (cancelError) {
        // not connected
      }
      return false;
    }
  }

  /** Stop waiting for a remembered device (manual connect or disconnect). */
  cancelPending() {
    return this._abortAttempt();
  }

  /** A connect or reconnect is in progress (including a pending autoConnect). */
  get isConnecting() {
    return this._connecting || this._pendingId !== null;
  }

  /** Take over a connected, authenticated device: notifications + events. */
  async _adopt(device, kind) {
    const gatt = GATT[kind];
    this.device = device;
    this.kind = kind;

    try {
      await device.requestConnectionPriority(ConnectionPriority.High);
    } catch (error) {
      console.warn('[BLE] Connection priority request failed:', error.message);
    }

    this._disconnectSubscription = device.onDisconnected((error) => {
      if (error?.message) console.warn('[BLE] Disconnected:', error.message);
      if (this.device === device) this._teardown();
    });

    this._subscription = device.monitorCharacteristicForService(gatt.service, gatt.notify, (error, characteristic) => {
      if (error) {
        // "cancelled" is our own teardown removing the subscription
        if (!/cancel/i.test(error.message)) console.warn('[BLE] Notify error:', error.message);
        return;
      }
      if (characteristic?.value) this.dataListeners.forEach((fn) => fn(characteristic.value));
    });

    this._emitConnection(true);
  }

  async disconnect() {
    await this._abortAttempt();
    const device = this.device;
    this._teardown();
    if (device) {
      try {
        await device.cancelConnection();
      } catch (e) {
        // device may already be disconnected
      }
    }
  }

  _teardown() {
    const wasConnected = this.device !== null;
    this.device = null;
    this.kind = null;
    this._disconnectSubscription?.remove();
    this._disconnectSubscription = null;
    this._subscription?.remove();
    this._subscription = null;
    this.clearCommandLock();
    if (wasConnected) this._emitConnection(false);
  }

  // --- Write ---

  _lockAllows(commandLockToken) {
    return !this._commandLockToken || commandLockToken === this._commandLockToken;
  }

  async writeCommand(base64Data, commandLockToken = null) {
    if (!this.device || !this._lockAllows(commandLockToken)) return false;
    const gatt = GATT[this.kind];
    try {
      await this.device.writeCharacteristicWithoutResponseForService(gatt.service, gatt.write, base64Data);
      return true;
    } catch (error) {
      console.warn('[BLE] Write error:', error.message);
      return false;
    }
  }

  async writeCommandWithResponse(base64Data, commandLockToken = null) {
    if (!this.device || !this._lockAllows(commandLockToken)) return false;
    const gatt = GATT[this.kind];
    try {
      await this.device.writeCharacteristicWithResponseForService(gatt.service, gatt.write, base64Data);
      return true;
    } catch (error) {
      // Some firmware revisions only expose write-without-response.
      console.warn('[BLE] Write-with-response error, retrying without response:', error.message);
      return this.writeCommand(base64Data, commandLockToken);
    }
  }

  get isConnected() {
    return this.device !== null;
  }

  getConnectedDeviceInfo() {
    if (!this.device) return null;
    return {
      id: this.device.id,
      name: this.device.name || this.device.localName || 'Unknown',
      mtu: this.device.mtu,
      kind: this.kind,
    };
  }
}

export default new BleLink();
