import bleLink from '../ble/bleLink';
import { DEVICE_KIND } from '../ble/gattProfiles';
import displayDevice from './display/displayDevice';
import lps2Device from './lps2/lps2Device';

// The one object screens talk to. It owns the connection and forwards every
// call to the driver for the chip that is connected, so no screen ever
// branches on the transport. Differences are expressed as capabilities:
//
//   multiUnit      — several units can be selected (ClaytonDisplay on CAN)
//   settings       — unit settings can be read and written
//   firmwareUpdate — CAN bootloader available (ClaytonDisplay only)
//
// Driver interface (displayDevice.js, lps2Device.js):
//   kind, capabilities, authenticate(device, gatt), attach(advert), detach(),
//   handleData(base64), onNotification(fn), getUnits(), getActiveUnitInfo(),
//   selectUnit(index), requestUnits(), requestDashboard(), requestErrors(),
//   clearErrors(), getSetting(b, id), setSetting(b, id, v), getRange(b, id),
//   toggleOutput('ac' | 'dc')
// Messages: 'dashboard' (devices/dashboardModel.js), 'errors', 'unitInfo',
// 'settingValue', 'settingRange'.

const DRIVERS = {
  [DEVICE_KIND.DISPLAY]: displayDevice,
  [DEVICE_KIND.LPS2]: lps2Device,
};

const NO_CAPABILITIES = { multiUnit: false, settings: false, firmwareUpdate: false };

class DeviceSession {
  constructor() {
    this.driver = null;
    this.connectedName = null;
    this.advert = {};
    this._attempt = 0;
    this.listeners = new Set();
    this.connectionListeners = new Set();

    // Forward messages from whichever driver is active.
    Object.values(DRIVERS).forEach((driver) => {
      driver.onNotification((message) => {
        if (driver === this.driver) this.listeners.forEach((fn) => fn(message));
      });
    });

    // A link that already exists (development reload of this module) gets
    // its driver back instead of streaming data nobody handles.
    if (bleLink.isConnected) {
      this.driver = DRIVERS[bleLink.kind] || displayDevice;
      this.driver.attach({});
    }

    bleLink.onData((base64) => {
      if (__DEV__ && !this._sawData) {
        this._sawData = true;
        console.log(`[Session] first data from the unit (driver=${this.driver?.kind ?? 'none'})`);
      }
      this.driver?.handleData(base64);
    });

    bleLink.onConnectionChange((connected) => {
      // Attach the driver before screens hear about the connection, so their
      // first requests already reach a ready driver.
      if (__DEV__) console.log(`[Session] ${connected ? 'connected' : 'disconnected'} driver=${this.driver?.kind ?? 'none'}`);
      if (connected) this.driver?.attach(this.advert);
      else {
        this._sawData = false;
        this.driver?.detach();
        this.driver = null;
        this.connectedName = null;
      }
      this.connectionListeners.forEach((fn) => fn(connected));
    });
  }

  // --- Connection ---

  scan(timeoutMs) {
    return bleLink.scan(timeoutMs);
  }

  /** device: an entry from scan() — { id, name, kind, serial?, soc? }. */
  async connect(device) {
    await bleLink.cancelPending(); // a manual choice wins over a waiting reconnect
    return this._connectWith(device, (driver, auth) => bleLink.connect(device.id, driver.kind, auth));
  }

  /**
   * Reconnect to a remembered device ({ id, name, kind }) as soon as it is in
   * range. Used by services/backgroundService.js; cancel with cancelReconnect().
   */
  reconnect(device) {
    return this._connectWith(device, (driver, auth) => bleLink.connectWhenInRange(device.id, driver.kind, auth));
  }

  cancelReconnect() {
    return bleLink.cancelPending();
  }

  /** A connect or reconnect is in progress (incl. waiting for the unit to come in range). */
  get isConnecting() {
    return bleLink.isConnecting;
  }

  async _connectWith(device, open) {
    const attempt = ++this._attempt;
    const driver = DRIVERS[device.kind] || displayDevice;
    // Set before connecting so the first notifications reach the driver.
    this.driver = driver;
    this.connectedName = device.name;
    this.advert = { serial: device.serial, soc: device.soc };
    const ok = await open(driver, (dev, gatt) => driver.authenticate(dev, gatt));
    // Only the latest attempt may clear the driver; an older, cancelled one
    // must not undo a connection that replaced it.
    if (!ok && attempt === this._attempt && !bleLink.isConnected) this.driver = null;
    return ok;
  }

  disconnect() {
    return bleLink.disconnect();
  }

  get isConnected() {
    return bleLink.isConnected && this.driver !== null;
  }

  get connectedDeviceId() {
    return bleLink.device?.id ?? null;
  }

  /** { id, name, kind } of the connected unit — what reconnect() needs later. */
  get connectedDevice() {
    if (!this.isConnected) return null;
    return { id: this.connectedDeviceId, name: this.connectedName, kind: this.kind };
  }

  get kind() {
    return this.driver?.kind ?? null;
  }

  get capabilities() {
    return this.driver?.capabilities ?? NO_CAPABILITIES;
  }

  onConnectionChange(fn) {
    this.connectionListeners.add(fn);
    return () => this.connectionListeners.delete(fn);
  }

  onNotification(fn) {
    this.listeners.add(fn);
    return () => this.listeners.delete(fn);
  }

  /** True while the firmware update holds the link exclusively. */
  get isLocked() {
    return bleLink.commandLockOwner === 'firmware-update';
  }

  // --- Device calls (no-ops while disconnected) ---

  getUnits() { return this.driver?.getUnits() ?? []; }
  getActiveUnitInfo() { return this.driver?.getActiveUnitInfo() ?? null; }
  selectUnit(index) { return this.driver?.selectUnit(index) ?? false; }
  requestUnits() { return this.driver?.requestUnits(); }
  requestDashboard() { return this.driver?.requestDashboard(); }
  requestErrors() { return this.driver?.requestErrors(); }
  clearErrors() { return this.driver?.clearErrors() ?? Promise.resolve(false); }
  getSetting(block, id) { return this.driver?.getSetting(block, id); }
  setSetting(block, id, value) { return this.driver?.setSetting(block, id, value) ?? Promise.resolve(false); }
  getRange(block, id) { return this.driver?.getRange(block, id); }
  toggleOutput(output) { return this.driver?.toggleOutput(output) ?? Promise.resolve(false); }
}

export default new DeviceSession();
