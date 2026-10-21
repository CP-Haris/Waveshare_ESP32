import bleLink from '../../ble/bleLink';
import { DEVICE_KIND, formatLps2Serial } from '../../ble/gattProfiles';
import { FAULT, emptyDashboard, functionStatus } from '../dashboardModel';
import { DEV_LPS } from '../../utils/units';
import { FrameParser, base64FromBytes, bytesFromBase64, encodeFrame } from './lps2Frame';

// Path B driver: an LPS2 through its built-in BLE module. The module pipes
// frames to the LPS2's display MCU, which answers or forwards them to the
// control board. Exactly one unit, no CAN, no bootloader.
// Protocol: Docs/LPS2-BLE-Protocol.md.

const CMD = {
  VALUE: 0x20, //        read: [20 bb ii]  · reply: [20 bb ii v0 v1 v2 v3]
  SET: 0x50, //          write: [50 bb ii v0 v1 v2 v3]
  MIN_MAX: 0x22, //      read: [22 bb ii]  · reply: [22 bb ii min32 max32]
  ERRORS: 0x41, //       read: [41]        · reply: [41 e1..e8]
  CLEAR_ERRORS: 0x60,
  FUNCTION: 0x80, //     [80 00 id …] toggles an output; the display picks on/off
};

const Q16 = 65536;

// Block 0 value IDs (LPS2 Display Firmware/firmware/src/Values.c)
const V = {
  SOC: 119,
  REMAINING_HOURS: 120,
  BATTERY_V: 100,
  BATTERY_A: 101,
  BATTERY_STATUS: 210,
  SERIAL: 220,
};

// Per function: live power, operating state, failure level (block 0).
const FUNCTIONS = {
  acIn: { power: 104, op: 200, fail: 204, input: true },
  dcIn: { power: 110, op: 202, fail: 206, input: true },
  solar: { power: 79, op: 208, fail: 209, input: true },
  acOut: { power: 107, op: 201, fail: 205, input: false },
  dcOut: { power: 113, op: 203, fail: 207, input: false },
};

// Output toggle IDs for CMD.FUNCTION (UART_App.c: 0xC9 = 230 V, 0xCB = 12 V).
const OUTPUT_ID = { ac: 0xc9, dc: 0xcb };

const POLLED_IDS = [
  ...Object.values(V),
  ...Object.values(FUNCTIONS).flatMap((f) => [f.power, f.op, f.fail]),
];

// Operating state, failure level and battery status are Q16 enums.
const OS_STARTING = 3; // starting, stopping and on all count as running
const FL = { WARNING: 1, SIMPLE_FAILURE: 2, EMPTY: 3 };
const BS = { DISCHARGE: 2, CHARGE: 3, FULL: 4, BALANCING: 5 };

const AUTH_ANSWER_TIMEOUT_MS = 2500;
const ERROR_POLL_EVERY = 3; // dashboard polls

const enumOf = (raw) => Math.round((raw | 0) / Q16);

// Development builds log every frame except the routine block 0 polling, so
// settings, min/max, errors and toggles can be followed in the Metro log.
function trace(direction, payload) {
  if (!__DEV__) return;
  const routinePoll = payload[0] === CMD.VALUE && payload[1] === 0;
  if (routinePoll) return;
  const hex = payload.map((b) => b.toString(16).toUpperCase().padStart(2, '0')).join(' ');
  console.log(`[LPS2] ${direction} ${hex}`);
}

function int32Le(bytes, offset) {
  return bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24);
}

function int32Bytes(value) {
  const v = value | 0;
  return [v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff, (v >> 24) & 0xff];
}

function lps2Fault(failLevel, isInput) {
  // Inputs keep charging at FL_EMPTY; everything above warning blocks the rest.
  if (failLevel >= FL.SIMPLE_FAILURE && !(isInput && failLevel === FL.EMPTY)) return FAULT.BLOCKED;
  if (failLevel === FL.WARNING) return FAULT.WARNING;
  return FAULT.NONE;
}

class Lps2Device {
  kind = DEVICE_KIND.LPS2;

  capabilities = { multiUnit: false, settings: true, firmwareUpdate: false };

  constructor() {
    this.listeners = new Set();
    this.reset();
  }

  reset() {
    this.parser = new FrameParser();
    this.values = new Map(); // `${block}:${id}` -> raw int32
    this.errorCodes = [];
    this.serial = '';
    this.polling = false;
    this.pollCount = 0;
  }

  onNotification(listener) {
    this.listeners.add(listener);
    return () => this.listeners.delete(listener);
  }

  _emit(type, data) {
    this.listeners.forEach((listener) => listener({ type, data }));
  }

  // --- Link lifecycle (called by deviceSession) ---

  /**
   * The link is usable once the LPS2 answers a request. Subscribing to the
   * encrypted notify characteristic is what makes Android ask for the PIN.
   */
  authenticate(device, gatt) {
    return new Promise((resolve, reject) => {
      const parser = new FrameParser();
      let done = false;
      let subscription = null;
      const finish = (error) => {
        if (done) return;
        done = true;
        clearTimeout(timer);
        subscription?.remove();
        if (error) reject(error);
        else resolve();
      };
      const timer = setTimeout(() => finish(new Error('LPS2 did not answer')), AUTH_ANSWER_TIMEOUT_MS);

      subscription = device.monitorCharacteristicForService(gatt.service, gatt.notify, (error, characteristic) => {
        if (error) {
          finish(error);
          return;
        }
        if (characteristic?.value && parser.push(bytesFromBase64(characteristic.value)).length > 0) finish();
      });

      const probe = base64FromBytes(encodeFrame([CMD.VALUE, 0, V.SOC]));
      device.writeCharacteristicWithoutResponseForService(gatt.service, gatt.write, probe).catch(finish);
    });
  }

  /** advert: { serial, soc } from the scan, shown until live values arrive. */
  attach(advert = {}) {
    this.reset();
    this.serial = advert.serial || '';
    if (Number.isFinite(advert.soc)) this.values.set(`0:${V.SOC}`, Math.round((advert.soc / 100) * Q16));
    this.requestUnits();
    this.requestDashboard();
    this.requestErrors();
  }

  detach() {
    this.reset();
  }

  handleData(base64) {
    this.parser.push(bytesFromBase64(base64)).forEach((payload) => this._handlePayload(payload));
  }

  // --- Requests (same interface as the display driver) ---

  _send(payload) {
    trace('→', payload);
    return bleLink.writeCommand(base64FromBytes(encodeFrame(payload)));
  }

  getUnits() {
    return [this._unitInfo()];
  }

  getActiveUnitInfo() {
    return this._unitInfo();
  }

  selectUnit() {
    return true; // only ever one unit
  }

  async requestUnits() {
    this._emit('unitInfo', this._unitInfo());
    return this._send([CMD.VALUE, 0, V.SERIAL]);
  }

  /**
   * Emit what we have, then ask for every dashboard value. The LPS2 only
   * forwards values its own screen happens to poll, so the app must poll.
   */
  async requestDashboard() {
    this._emit('dashboard', this._snapshot());
    if (this.polling) return true;
    this.polling = true;
    try {
      for (const id of POLLED_IDS) {
        if (!(await this._send([CMD.VALUE, 0, id]))) return false;
      }
      this.pollCount += 1;
      if (this.pollCount % ERROR_POLL_EVERY === 0) await this._send([CMD.ERRORS]);
      return true;
    } finally {
      this.polling = false;
    }
  }

  requestErrors() {
    this._emit('errors', [...this.errorCodes]);
    this._send([CMD.ERRORS]);
  }

  // Two bytes, as the previous Clayton Power GO app sent them (proven in the field).
  clearErrors() {
    return this._send([CMD.CLEAR_ERRORS, CMD.CLEAR_ERRORS]);
  }

  getSetting(block, id) {
    return this._send([CMD.VALUE, block, id]);
  }

  // SET_VAL, the same command the LPS2 display uses for its own menu. Needs
  // the display firmware fix in Docs/LPS2-Display-Fix-BLE-Settings.md; older
  // displays keep the value in their own RAM only.
  setSetting(block, id, value) {
    return this._send([CMD.SET, block, id, ...int32Bytes(value)]);
  }

  getRange(block, id) {
    return this._send([CMD.MIN_MAX, block, id]);
  }

  /** output: 'ac' | 'dc'. Always a toggle; the LPS2 decides on or off. */
  toggleOutput(output) {
    const id = OUTPUT_ID[output];
    if (id == null) return Promise.resolve(false);
    return this._send([CMD.FUNCTION, 0, id, 0, 0, 0, 0]);
  }

  // --- Incoming ---

  _handlePayload(payload) {
    trace('←', payload);
    const cmd = payload[0];

    if (cmd === CMD.VALUE && payload.length === 7) {
      const [, block, id] = payload;
      const value = int32Le(payload, 3);
      this.values.set(`${block}:${id}`, value);
      if (block === 0 && id === V.SERIAL) {
        this.serial = formatLps2Serial(value >>> 0) || this.serial;
        this._emit('unitInfo', this._unitInfo());
      }
      if (block !== 0) this._emit('settingValue', { block, id, value });
      return;
    }

    if (cmd === CMD.MIN_MAX && payload.length === 11) {
      const [, block, id] = payload;
      this._emit('settingRange', { block, id, min: int32Le(payload, 3), max: int32Le(payload, 7) });
      return;
    }

    if (cmd === CMD.ERRORS && payload.length === 9) {
      this.errorCodes = payload.slice(1).filter((code) => code !== 0);
      this._emit('errors', [...this.errorCodes]);
      this._emit('dashboard', this._snapshot());
    }
  }

  _raw(id) {
    return this.values.get(`0:${id}`) ?? 0;
  }

  _snapshot() {
    const status = enumOf(this._raw(V.BATTERY_STATUS));
    const direction = status === BS.DISCHARGE ? -1
      : status === BS.CHARGE || status === BS.FULL || status === BS.BALANCING ? 1
        : 0;

    const functions = {};
    Object.entries(FUNCTIONS).forEach(([name, f]) => {
      functions[name] = functionStatus(
        enumOf(this._raw(f.op)) >= OS_STARTING,
        lps2Fault(enumOf(this._raw(f.fail)), f.input),
        Math.abs(this._raw(f.power) / Q16),
      );
    });

    return {
      ...emptyDashboard(),
      soc: (this._raw(V.SOC) / Q16) * 100,
      batteryVoltage: this._raw(V.BATTERY_V) / Q16,
      // Direction comes from the battery status, so the sign of the raw
      // current never matters.
      batteryCurrent: direction * Math.abs(this._raw(V.BATTERY_A) / Q16),
      socTimeMin: Math.round(Math.abs(this._raw(V.REMAINING_HOURS) / Q16) * 60),
      functions,
      errorCodes: [...this.errorCodes],
      unitType: DEV_LPS,
      serial: this.serial,
    };
  }

  _unitInfo() {
    return {
      index: 0,
      type: DEV_LPS,
      addr: null,
      partNumber: '',
      serial: this.serial,
      connected: true,
      errorCount: this.errorCodes.length,
    };
  }
}

export default new Lps2Device();
