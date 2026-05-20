import bleService from './bleService';
import { encodeSendCanFrame, encodeSetCanPassthrough } from '../utils/protocol';

const CAN_DISPLAY_ADDR = 0xfe;
const CAN_ID_INFO_BROADCAST = 0x18eafffe;
const BOOTLOADER_ADDR = 0xf0;

const CAN_CMD_GET_VAL = 0x40;
const CAN_CMD_SET_VAL = 0x41;
const CAN_CMD_GET_MIN = 0x43;
const CAN_CMD_GET_MAX = 0x44;

const BOOTLOADER_TOP24 = new Set([0x19ff20, 0x19ff21, 0x19ff22]);

const FUNC_BLOCK = 0;
const FUNC_INVERTER_ID = 158;
const FUNC_DCOUT_ID = 159;

const ERROR_CLEAR_BLOCK = 1;
const ERROR_CLEAR_ID = 252;
const ERROR_CLEAR_VALUE = 1234;

const DEV_UNKNOWN = 0;
const DEV_LPS = 1;
const DEV_BMS = 2;

const Q16_ONE = 65536;

function readUint16(data, offset) {
  return (((data[offset] || 0) << 8) | (data[offset + 1] || 0)) & 0xffff;
}

function readInt16(data, offset) {
  const value = readUint16(data, offset);
  return value & 0x8000 ? value - 0x10000 : value;
}

function readInt32Le(data, offset) {
  return (
    (data[offset] || 0) |
    ((data[offset + 1] || 0) << 8) |
    ((data[offset + 2] || 0) << 16) |
    ((data[offset + 3] || 0) << 24)
  );
}

function int32ToBytes(value) {
  const signed = value | 0;
  return [
    signed & 0xff,
    (signed >> 8) & 0xff,
    (signed >> 16) & 0xff,
    (signed >> 24) & 0xff,
  ];
}

function asciiFromBytes(data, maxLength = data.length) {
  let text = '';
  for (let offset = 0; offset < Math.min(data.length, maxLength); offset++) {
    const value = data[offset] & 0xff;
    if (value === 0 || value === 0xff) break;
    text += value >= 32 && value <= 126 ? String.fromCharCode(value) : ' ';
  }
  return text.replace(/\s+$/g, '');
}

function createDashboardData() {
  return {
    unitType: DEV_UNKNOWN,
    soc: 0,
    batteryVoltage: 0,
    batteryCurrent: 0,
    batteryDodAh: 0,
    socTimeMin: 0,
    dcInVoltage: 0,
    dcInCurrent: 0,
    dcOutVoltage: 0,
    dcOutCurrent: 0,
    acInVoltage: 0,
    acInCurrent: 0,
    acInPower: 0,
    acOutVoltage: 0,
    acOutCurrent: 0,
    acOutPower: 0,
    solarCurrent: 0,
    cellVoltage: [0, 0, 0, 0],
    temps: [0, 0, 0],
    tempCellAvg: 0,
    operatingState: 0,
    failureLevel: 0,
    inverterState: 0,
    inverterFail: 0,
    chargerState: 0,
    chargerFail: 0,
    dcInState: 0,
    dcInFail: 0,
    dcOutState: 0,
    dcOutFail: 0,
    solarState: 0,
    solarFail: 0,
    errorCount: 0,
    errorCodes: [],
    failureCodes: [0, 0, 0, 0, 0, 0, 0, 0],
    connected: false,
    cellVMin: 0,
    cellVMax: 0,
    negTermTemp: 0,
    posTermTemp: 0,
    bmsOutputStatus: 0,
    bmsWakeupFlags: 0,
    bmsBattCount: 0,
  };
}

function snapshotDashboard(unit) {
  const data = unit.data;
  return {
    ...data,
    unitType: unit.type,
    partNumber: unit.partNumber,
    serial: unit.serial,
    cellVoltage: [...data.cellVoltage],
    temps: [...data.temps],
    errorCodes: [...data.errorCodes],
  };
}

class CanGatewayService {
  constructor() {
    this.listeners = new Set();
    this.unitsByAddr = new Map();
    this.activeUnitIndex = null;
    this.nextUnitIndex = 0;
    this.rangePending = new Map();
    this.passthroughEnabled = false;
    this.passthroughEnabledAt = 0;
    this.enablePromise = null;

    bleService.onNotification((message) => {
      if (message?.type === 'canFrame') this._handleCanFrame(message.data);
    });

    bleService.onConnectionChange((connected) => {
      this.reset();
      if (connected) {
        this.ensurePassthrough().then((enabled) => {
          if (enabled) this.requestUnits();
        });
      }
    });
  }

  reset() {
    this.unitsByAddr.clear();
    this.activeUnitIndex = null;
    this.nextUnitIndex = 0;
    this.rangePending.clear();
    this.passthroughEnabled = false;
    this.passthroughEnabledAt = 0;
    this.enablePromise = null;
  }

  onNotification(listener) {
    this.listeners.add(listener);
    return () => this.listeners.delete(listener);
  }

  async ensurePassthrough() {
    if (!bleService.isConnected) return false;

    const now = Date.now();
    if (this.passthroughEnabled && now - this.passthroughEnabledAt < 5000) {
      return true;
    }

    if (this.enablePromise) return this.enablePromise;

    this.enablePromise = bleService.writeCommandWithResponse(encodeSetCanPassthrough(true))
      .then((ok) => {
        this.passthroughEnabled = !!ok;
        this.passthroughEnabledAt = ok ? Date.now() : 0;
        return !!ok;
      })
      .finally(() => {
        this.enablePromise = null;
      });

    return this.enablePromise;
  }

  async sendCan(canId, dataBytes) {
    const enabled = await this.ensurePassthrough();
    if (!enabled) return false;
    return bleService.writeCommand(encodeSendCanFrame(canId >>> 0, dataBytes));
  }

  async requestUnits() {
    const enabled = await this.ensurePassthrough();
    if (!enabled) return false;

    this._emitExistingUnits();
    await this.sendCan(CAN_ID_INFO_BROADCAST, [0x01, 0xff, 0x00, 0, 0, 0, 0, 0]);
    await this.sendCan(CAN_ID_INFO_BROADCAST, [0x00, 0xff, 0x01, 0, 0, 0, 0, 0]);
    return true;
  }

  async requestDashboard() {
    const enabled = await this.ensurePassthrough();
    if (!enabled) return false;
    this._emitActiveDashboard();
    return true;
  }

  requestErrors() {
    const activeUnit = this._getActiveUnit();
    this._emit('errors', activeUnit ? [...activeUnit.data.errorCodes] : []);
  }

  async clearErrors() {
    return this._sendCanExtra(CAN_CMD_SET_VAL, ERROR_CLEAR_BLOCK, ERROR_CLEAR_ID, ERROR_CLEAR_VALUE);
  }

  selectUnit(index) {
    const unit = this._findUnitByIndex(index);
    if (!unit) return false;
    this.activeUnitIndex = unit.index;
    this._emit('unitInfo', this._unitInfo(unit));
    this._emitActiveDashboard();
    this.requestErrors();
    return true;
  }

  async getSetting(block, id) {
    return this._sendCanExtra(CAN_CMD_GET_VAL, block, id, 0);
  }

  async setSetting(block, id, value) {
    return this._sendCanExtra(CAN_CMD_SET_VAL, block, id, value);
  }

  async getRange(block, id) {
    const minOk = await this._sendCanExtra(CAN_CMD_GET_MIN, block, id, 0);
    const maxOk = await this._sendCanExtra(CAN_CMD_GET_MAX, block, id, 0);
    return minOk && maxOk;
  }

  async toggleFunc(funcId) {
    const activeUnit = this._getActiveUnit();
    if (!activeUnit) return false;

    if (funcId === 0) {
      const nextValue = activeUnit.data.inverterState >= 1 ? 0 : Q16_ONE;
      return this.setSetting(FUNC_BLOCK, FUNC_INVERTER_ID, nextValue);
    }

    if (funcId === 1) {
      const nextValue = activeUnit.data.dcOutState >= 1 ? 0 : Q16_ONE;
      return this.setSetting(FUNC_BLOCK, FUNC_DCOUT_ID, nextValue);
    }

    return false;
  }

  _sendCanExtra(cmd, block, id, value) {
    const activeUnit = this._getActiveUnit();
    if (!activeUnit) return Promise.resolve(false);

    const canId = (0x19ef0000 | ((activeUnit.addr & 0xff) << 8) | CAN_DISPLAY_ADDR) >>> 0;
    const dataBytes = [0x00, cmd & 0xff, block & 0xff, id & 0xff, ...int32ToBytes(value)];
    return this.sendCan(canId, dataBytes);
  }

  _handleCanFrame(frame) {
    if (!frame || !Number.isInteger(frame.canId)) return;
    if (bleService.commandLockOwner === 'firmware-update') return;

    const canId = frame.canId >>> 0;
    const data = Array.isArray(frame.data) ? frame.data : [];
    const sourceAddr = canId & 0xff;
    const upper = (canId >>> 16) & 0xffff;
    const pgnByte = (canId >>> 8) & 0xff;
    const top24 = (canId >>> 8) & 0xffffff;

    if (sourceAddr === BOOTLOADER_ADDR && BOOTLOADER_TOP24.has(top24)) return;

    if (upper === 0x19ef || upper === 0x15ef) {
      const unit = this._ensureUnit(sourceAddr);
      unit.data.connected = true;
      if (unit.index === this.activeUnitIndex) this._decodeCanExtra(data);
      return;
    }

    if (upper === 0x19ff && pgnByte <= 0x08) {
      const unit = this._ensureUnit(sourceAddr);
      this._decodeIdentification(unit, pgnByte, data);
      this._emit('unitInfo', this._unitInfo(unit));
      return;
    }

    if (upper === 0x18ff || upper === 0x14ff || upper === 0x19ff) {
      const unit = this._ensureUnit(sourceAddr);
      unit.data.connected = true;
      if (unit.type === DEV_BMS) this._decodeBmsBroadcast(unit.data, pgnByte, data);
      else this._decodeLpsBroadcast(unit.data, pgnByte, data);

      if (unit.index === this.activeUnitIndex) {
        this._emitActiveDashboard();
        if (pgnByte === 0x05) this.requestErrors();
      }
    }
  }

  _decodeCanExtra(data) {
    if (data.length < 8 || data[0] !== 0x00) return;

    const cmd = data[1] & 0xff;
    const block = data[2] & 0xff;
    const id = data[3] & 0xff;
    const value = readInt32Le(data, 4);

    if (cmd === CAN_CMD_GET_VAL || cmd === CAN_CMD_SET_VAL) {
      this._emit('settingValue', { block, id, value });
      return;
    }

    if (cmd !== CAN_CMD_GET_MIN && cmd !== CAN_CMD_GET_MAX) return;

    const key = `${block}-${id}`;
    const pending = this.rangePending.get(key) || { block, id };
    if (cmd === CAN_CMD_GET_MIN) pending.min = value;
    if (cmd === CAN_CMD_GET_MAX) pending.max = value;
    this.rangePending.set(key, pending);

    if (pending.min != null && pending.max != null) {
      this.rangePending.delete(key);
      this._emit('settingRange', { block, id, min: pending.min, max: pending.max });
    }
  }

  _decodeIdentification(unit, pgnByte, data) {
    if (data.length < 8) return;

    if (pgnByte === 0x00) {
      if (data[2] !== 0xff) {
        unit.serial = `${String(data[0]).padStart(2, '0')}.${String(data[1]).padStart(2, '0')}.${String(data[2]).padStart(2, '0')}-${String(data[3]).padStart(2, '0')}${String(data[4]).padStart(2, '0')}${String(data[5]).padStart(2, '0')}-${String(data[6]).padStart(2, '0')}${String(data[7]).padStart(2, '0')}`;
      } else {
        unit.serial = `${String(data[0]).padStart(2, '0')}.${String(data[1]).padStart(2, '0')}-${String(data[3]).padStart(2, '0')}${String(data[4]).padStart(2, '0')}${String(data[5]).padStart(2, '0')}-${String(data[6]).padStart(2, '0')}${String(data[7]).padStart(2, '0')}`;
      }
      return;
    }

    if (pgnByte === 0x03) {
      unit.partHigh = asciiFromBytes(data, 8);
    } else if (pgnByte === 0x04) {
      unit.partLow = asciiFromBytes(data, 7);
    } else {
      return;
    }

    unit.partNumber = `${unit.partHigh || ''}${unit.partLow || ''}`.trim();
    this._classifyUnit(unit);
    this._preferIdentifiedUnit(unit);
  }

  _decodeLpsBroadcast(data, pgnByte, payload) {
    if (payload.length < 8) return;

    switch (pgnByte) {
      case 0x00:
        data.soc = readUint16(payload, 0) / 65535 * 100;
        data.batteryCurrent = readInt16(payload, 2) / 10;
        data.socTimeMin = readInt16(payload, 4);
        data.batteryDodAh = readUint16(payload, 6);
        break;
      case 0x01:
        data.dcInVoltage = readUint16(payload, 0) / 100;
        data.dcInCurrent = readInt16(payload, 2) / 100;
        data.dcOutVoltage = readUint16(payload, 4) / 1000;
        data.dcOutCurrent = readInt16(payload, 6) / 10;
        break;
      case 0x03:
        data.operatingState = payload[0] << 24 >> 24;
        data.failureLevel = payload[1] & 0xff;
        break;
      case 0x04:
        data.inverterState = payload[0] << 24 >> 24;
        data.inverterFail = payload[1] & 0xff;
        data.chargerState = payload[2] << 24 >> 24;
        data.chargerFail = payload[3] & 0xff;
        data.dcInState = payload[4] << 24 >> 24;
        data.dcInFail = payload[5] & 0xff;
        data.dcOutState = payload[6] << 24 >> 24;
        data.dcOutFail = payload[7] & 0xff;
        break;
      case 0x05:
        this._setFailureCodes(data, payload);
        break;
      case 0x06:
        data.temps = [
          readInt16(payload, 0) / 256,
          readInt16(payload, 2) / 256,
          readInt16(payload, 4) / 256,
        ];
        data.tempCellAvg = readInt16(payload, 6) / 256;
        break;
      case 0x09:
        data.acInVoltage = readUint16(payload, 0) / 10;
        data.acInCurrent = readUint16(payload, 2) / 1000;
        data.acOutVoltage = readUint16(payload, 4) / 10;
        data.acOutCurrent = readUint16(payload, 6) / 100;
        break;
      case 0x10:
        data.batteryVoltage = 0;
        data.cellVoltage = [0, 1, 2, 3].map((cellIndex) => {
          const cellVoltage = readUint16(payload, cellIndex * 2) / 8192;
          data.batteryVoltage += cellVoltage;
          return cellVoltage;
        });
        break;
      case 0x20:
        data.solarCurrent = readInt16(payload, 4) / 100;
        data.solarState = payload[6] << 24 >> 24;
        data.solarFail = payload[7] & 0xff;
        break;
      case 0x22:
        data.acInPower = readUint16(payload, 0);
        data.acOutPower = readUint16(payload, 2);
        break;
      default:
        break;
    }
  }

  _decodeBmsBroadcast(data, pgnByte, payload) {
    if (payload.length < 8) return;

    switch (pgnByte) {
      case 0x00:
        data.operatingState = payload[0] << 24 >> 24;
        data.failureLevel = payload[1] & 0xff;
        data.bmsWakeupFlags = readUint16(payload, 4);
        data.bmsOutputStatus = payload[6] & 0xff;
        data.bmsBattCount = payload[7] & 0xff;
        break;
      case 0x01:
        data.soc = readUint16(payload, 0) / 65535 * 100;
        data.socTimeMin = readInt16(payload, 2);
        data.batteryDodAh = readUint16(payload, 4);
        break;
      case 0x02:
        data.batteryVoltage = readUint16(payload, 0) / 1000;
        data.batteryCurrent = readInt16(payload, 2) / 10;
        data.cellVMin = readUint16(payload, 4) / 8192;
        data.cellVMax = readUint16(payload, 6) / 8192;
        break;
      case 0x03:
        data.dcOutVoltage = readUint16(payload, 0) / 1000;
        break;
      case 0x05:
        this._setFailureCodes(data, payload);
        break;
      case 0x06:
        data.temps = [
          readInt16(payload, 0) / 256,
          readInt16(payload, 2) / 256,
          readInt16(payload, 4) / 256,
        ];
        break;
      case 0x08:
        data.negTermTemp = readInt16(payload, 0) / 256;
        data.posTermTemp = readInt16(payload, 2) / 256;
        break;
      case 0x10:
        data.batteryVoltage = 0;
        data.cellVoltage = [0, 1, 2, 3].map((cellIndex) => {
          const cellVoltage = readUint16(payload, cellIndex * 2) / 8192;
          data.batteryVoltage += cellVoltage;
          return cellVoltage;
        });
        break;
      default:
        break;
    }
  }

  _setFailureCodes(data, payload) {
    const failureCodes = payload.slice(0, 8).map((value) => value & 0xff);
    data.failureCodes = failureCodes;
    data.errorCodes = failureCodes.filter((value) => value !== 0);
    data.errorCount = data.errorCodes.length;
  }

  _ensureUnit(addr) {
    const normalizedAddr = addr & 0xff;
    let unit = this.unitsByAddr.get(normalizedAddr);
    if (!unit) {
      unit = {
        index: this.nextUnitIndex++,
        addr: normalizedAddr,
        type: DEV_UNKNOWN,
        partNumber: '',
        partHigh: '',
        partLow: '',
        serial: '',
        data: createDashboardData(),
      };
      this.unitsByAddr.set(normalizedAddr, unit);
      if (this.activeUnitIndex == null) this.activeUnitIndex = unit.index;
      this._emit('unitInfo', this._unitInfo(unit));
    }
    return unit;
  }

  _classifyUnit(unit) {
    const partNumber = String(unit.partNumber || '').toUpperCase();
    if (partNumber.startsWith('CL')) unit.type = DEV_LPS;
    else if (partNumber.startsWith('CB')) unit.type = DEV_BMS;
    else unit.type = DEV_UNKNOWN;
    unit.data.unitType = unit.type;
  }

  _preferIdentifiedUnit(unit) {
    if (unit.type === DEV_UNKNOWN) return;

    const activeUnit = this._getActiveUnit();
    if (!activeUnit || activeUnit.type === DEV_UNKNOWN || activeUnit.addr === BOOTLOADER_ADDR) {
      this.activeUnitIndex = unit.index;
      this._emitActiveDashboard();
      this.requestErrors();
    }
  }

  _findUnitByIndex(index) {
    return Array.from(this.unitsByAddr.values()).find((unit) => unit.index === index) || null;
  }

  _getActiveUnit() {
    if (this.activeUnitIndex == null) return null;
    return this._findUnitByIndex(this.activeUnitIndex);
  }

  _emitExistingUnits() {
    Array.from(this.unitsByAddr.values())
      .sort((left, right) => left.index - right.index)
      .forEach((unit) => this._emit('unitInfo', this._unitInfo(unit)));
  }

  _emitActiveDashboard() {
    const activeUnit = this._getActiveUnit();
    if (activeUnit) this._emit('dashboard', snapshotDashboard(activeUnit));
  }

  _unitInfo(unit) {
    return {
      index: unit.index,
      type: unit.type,
      addr: unit.addr,
      partNumber: unit.partNumber,
      serial: unit.serial,
    };
  }

  _emit(type, data) {
    this.listeners.forEach((listener) => listener({ type, data }));
  }
}

export default new CanGatewayService();