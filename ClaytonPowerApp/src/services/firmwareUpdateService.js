import bleService from './bleService';
import { encodeSendCanFrame, encodeSendCanFrames, encodeSetCanPassthrough } from '../utils/protocol';
import {
  asciiFromCanData,
  canIdWithUnit,
  delay,
  digitsOnly,
  eqIgnoreCase,
  normalizeBridgeId,
  normalizeSerialForHistory,
  parseIntelHex,
  semverToInt,
  serialFromNumeric,
  serialToBytes,
  toHex,
  uniqueBridgeIds,
} from './firmwareUpdateHelpers';

const CAN_ID_STATUS = 0x19FF20;
const CAN_ID_BOOT_DATA = 0x19FF21;
const CAN_ID_BOOT_RESP = 0x19FF22;
const CAN_ID_ENTER_BOOT_HI = 0x19EF00;
const CAN_ID_INFO_BROADCAST = 0x18EAFFFE;

const CAN_MSG_SERIAL_A = 0x19FF00;
const CAN_MSG_SERIAL_B = 0x18FF07;
const CAN_MSG_PART_HIGH = 0x19FF03;
const CAN_MSG_PART_LOW = 0x19FF04;

const BOOT_START_BYTE = 0xAF;
const BOOT_READY = 199;
const BOOT_ACK = 99;

const CMD_READ_VERSION = 0x00;
const CMD_WRITE_FLASH = 0x02;
const CMD_ERASE_FLASH = 0x03;
const CMD_RESET_DEVICE = 0x09;

const STATUS_OK = 0x01;
const STATUS_BAD_LENGTH = 0xFD;
const STATUS_BAD_CRC = 0xFB;
const STATUS_TIMEOUT = 0xFA;

const CHUNK_CAN_DATA = 7;
const MAX_CAN_FRAMES_PER_BLE_WRITE = 16;
const WRITE_BLOCK_BYTES = 240;
const WRITE_CMD_TIMEOUT_MS = 900;
const CHUNK_DELAY_MS = 0;
const WRITE_RETRIES = 5;
const WRITE_CHUNK_DELAY_MS = 0;
const WRITE_BLOCK_SETTLE_DELAY_MS = 5;
const WRITE_BLOCK_RECOVERY_ATTEMPTS = 3;
const WRITE_RECOVERY_REOPEN_DELAY_MS = 80;
const FINAL_WRITE_SETTLE_DELAY_MS = 750;
const RESET_DEVICE_TIMEOUT_MS = 4000;
const RESET_DEVICE_RETRIES = 5;
const BRIDGE_OPEN_TIMEOUT_MS = 5000;
const BRIDGE_INIT_DELAY_MS = 5000;
const EXIT_BOOTLOADER_REPEAT_COUNT = 3;
const EXIT_BOOTLOADER_REPEAT_GAP_MS = 100;
const EXIT_BOOTLOADER_SETTLE_DELAY_MS = 1000;
const POST_UPDATE_APP_RESET_COUNT = 3;
const POST_UPDATE_APP_RESET_GAP_MS = 150;
const POST_UPDATE_APP_RESET_SETTLE_MS = 1500;
const HEARTBEAT_WAIT_MS = 10000;
const HEARTBEAT_PRESCAN_MS = 2000;
const ENTER_BOOT_RETRIES = 2;
const ENTER_BOOT_DELAY_MS = 120;
const ENTER_BOOT_BURST_COUNT = 3;
const ENTER_BOOT_BURST_GAP_MS = 120;
const ENTER_BOOT_MODE_COMMAND = 0x01;
const ENTER_BOOT_RESET_COMMAND = 0x06;
const MAX_SUPPORTED_BRIDGE_ID = 4;
const ENABLE_BOOTLOADER_SET_CANID_INIT = false;
const FIRMWARE_VERSION_REQUESTS = [
  { bridgeId: 1, block: 0, id: 223 },
  { bridgeId: 2, block: 9, id: 0 },
  { bridgeId: 3, block: 9, id: 1 },
  { bridgeId: 4, block: 9, id: 2 },
];

const NO_SERIAL_VALUE = 25757575755;
const NO_SERIAL_BYTES = [25, 75, 75, 75, 55];
const PC_UNIT_ID = 0xFE;

function versionFamily(versionString) {
  const m = String(versionString || '').trim().match(/^(\d+)\.(\d+)[\.:](\d+)$/);
  if (m) return `${Number(m[1])}.${Number(m[2])}`;
  return '';
}

function firmwareIntToVersion(versionInt) {
  const value = Number(versionInt);
  if (!Number.isFinite(value) || value <= 0) return '00.00.00';

  const major = Math.floor(value / 10000);
  const minor = Math.floor(value / 100) % 100;
  const patch = value % 100;
  return `${String(major).padStart(2, '0')}.${String(minor).padStart(2, '0')}.${String(patch).padStart(2, '0')}`;
}

function comparableVersionInt(item) {
  const fromString = semverToInt(item?.versionString);
  if (fromString > 0) return fromString;

  const raw = Number(item?.versionInt);
  if (!Number.isFinite(raw) || raw <= 0) return 0;

  const rawText = String(item?.versionInt || '').trim();
  const m = rawText.match(/^(\d{1,2})(\d{3})(\d{3})$/);
  if (m) {
    return Number(m[1]) * 10000 + Number(m[2]) * 100 + Number(m[3]);
  }

  return raw;
}

function crc16Boot(data, from = 1, len = data.length) {
  let crc = 0x6363;
  for (let i = from; i < len; i++) {
    let ch = data[i] & 0xff;
    const low = crc & 0xff;
    const high = (crc >> 8) & 0xff;
    ch ^= low;
    ch ^= (ch << 4) & 0xff;
    crc = (high ^ ((ch << 8) & 0xffff) ^ ((ch << 3) & 0xffff) ^ (ch >> 4)) & 0xffff;
  }
  return crc;
}

function finalizeWithCrc(msg) {
  if (msg.length >= 16) {
    msg[14] = 0x00;
    msg[15] = 0x00;
  }
  const crc = crc16Boot(msg);
  msg[14] = crc & 0xff;
  msg[15] = (crc >> 8) & 0xff;
  return Array.from(msg);
}

function buildRequestInfoMsg() {
  const msg = new Uint8Array(16);
  msg[0] = BOOT_START_BYTE;
  msg[1] = CMD_READ_VERSION;
  return finalizeWithCrc(msg);
}

function buildResetMsg() {
  const msg = new Uint8Array(16);
  msg[0] = BOOT_START_BYTE;
  msg[1] = CMD_RESET_DEVICE;
  return finalizeWithCrc(msg);
}

function buildEraseMsg(startAddress, pages) {
  const msg = new Uint8Array(16);
  msg[0] = BOOT_START_BYTE;
  msg[1] = CMD_ERASE_FLASH;
  msg[4] = 0x55;
  msg[6] = 0xaa;
  msg[8] = startAddress & 0xff;
  msg[9] = (startAddress >> 8) & 0xff;
  msg[10] = (startAddress >> 16) & 0xff;
  msg[11] = (startAddress >> 24) & 0xff;
  msg[12] = pages & 0xff;
  msg[13] = (pages >> 8) & 0xff;
  return finalizeWithCrc(msg);
}

function buildWritePackets(imageBytes, appStart) {
  const packets = [];
  for (let i = 0; i < imageBytes.length; i += WRITE_BLOCK_BYTES) {
    const dataSize = Math.min(WRITE_BLOCK_BYTES, imageBytes.length - i);
    const aligned = Math.round(dataSize / 8) * 8;

    const packet = new Uint8Array(aligned + 16);
    packet.fill(0xff);
    for (let b = 0; b < dataSize; b++) packet[16 + b] = imageBytes[i + b];

    let hasData = false;
    for (let p = 16; p < packet.length; p++) {
      if (packet[p] !== 0xff) {
        hasData = true;
        break;
      }
    }
    if (!hasData) continue;

    packet[0] = BOOT_START_BYTE;
    packet[1] = CMD_WRITE_FLASH;
    packet[2] = aligned & 0xff;
    packet[3] = (aligned >> 8) & 0xff;
    packet[4] = 0x55;
    packet[5] = 0x00;
    packet[6] = 0xaa;
    packet[7] = 0x00;

    const addr = appStart + i;
    packet[8] = addr & 0xff;
    packet[9] = (addr >> 8) & 0xff;
    packet[10] = (addr >> 16) & 0xff;
    packet[11] = (addr >> 24) & 0xff;
    packet[12] = 0x00;
    packet[13] = 0x00;

    packets.push(finalizeWithCrc(packet));
  }
  return packets;
}

class FirmwareUpdateService {
  constructor() {
    this._unsubBle = null;
    this._frameTaps = new Set();
    this._bootWaiters = [];
    this._bootPendingByUnit = new Map();
    this._frameWaiters = [];
    this._rxAssembly = new Map();
    this._active = false;
    this._commandLockToken = null;
    this._canTxQueue = Promise.resolve();
    this._canFrameBatchEnabled = true;
    this._suspendHeartbeatAck = false;
  }

  _attach() {
    if (this._unsubBle) return;
    this._unsubBle = bleService.onNotification((msg) => {
      if (msg.type === 'canFrame') this._onCanFrame(msg.data);
    });
  }

  _detach() {
    this._unsubBle?.();
    this._unsubBle = null;
    this._frameTaps.clear();
    this._frameWaiters.forEach((w) => clearTimeout(w.timer));
    this._frameWaiters = [];
    this._bootWaiters.forEach((w) => clearTimeout(w.timer));
    this._bootWaiters = [];
    this._bootPendingByUnit.clear();
    this._rxAssembly.clear();
    this._canTxQueue = Promise.resolve();
    this._suspendHeartbeatAck = false;
  }

  _lockCommandChannel() {
    if (!this._commandLockToken) {
      const token = bleService.acquireCommandLock('firmware-update');
      if (!token) throw new Error('BLE command channel is busy');
      this._commandLockToken = token;
    }
  }

  _unlockCommandChannel() {
    if (!this._commandLockToken) return;
    bleService.releaseCommandLock(this._commandLockToken);
    this._commandLockToken = null;
  }

  _pruneFrameWaiters() {
    this._frameWaiters = this._frameWaiters.filter((w) => !w.done);
  }

  _pruneBootWaiters() {
    this._bootWaiters = this._bootWaiters.filter((w) => !w.done);
  }

  _onCanFrame(frame) {
    this._frameTaps.forEach((tap) => {
      try {
        tap(frame);
      } catch {
      }
    });

    let resolvedFrameWaiter = false;
    for (const w of this._frameWaiters) {
      if (w.done) continue;
      try {
        if (w.match(frame)) {
          w.done = true;
          clearTimeout(w.timer);
          w.resolve(frame);
          resolvedFrameWaiter = true;
        }
      } catch {
      }
    }
    if (resolvedFrameWaiter) this._pruneFrameWaiters();

    if ((frame.canId >>> 8) !== CAN_ID_BOOT_RESP) return;

    const unitId = frame.canId & 0xff;
    const payload = frame.data || [];
    if (!payload.length) return;

    const bytesThisFrame = payload[0] & 0xff;
    if (bytesThisFrame <= 0) return;

    const asm = this._rxAssembly.get(unitId) || { bytes: [], expected: -1 };
    const count = Math.min(bytesThisFrame, payload.length - 1);
    if (asm.bytes.length === 0 && count > 0 && payload[1] !== BOOT_START_BYTE) {
      return;
    }
    for (let i = 0; i < count; i++) asm.bytes.push(payload[i + 1]);

    if (asm.bytes.length >= 7 && asm.expected < 0) {
      const dataLen = asm.bytes[2] | (asm.bytes[3] << 8);
      asm.expected = 7 + dataLen;
    }
    if (asm.expected > 0 && asm.bytes.length >= asm.expected) {
      const packet = asm.bytes.slice(0, asm.expected);
      this._rxAssembly.delete(unitId);
      this._resolveBootPacket(unitId, packet);
      return;
    }
    if (asm.bytes.length > 4096) {
      this._rxAssembly.delete(unitId);
      return;
    }
    this._rxAssembly.set(unitId, asm);
  }

  _resolveBootPacket(unitId, packet) {
    for (const w of this._bootWaiters) {
      if (!w.done && w.unitId === unitId) {
        w.done = true;
        clearTimeout(w.timer);
        w.resolve(packet);
        this._pruneBootWaiters();
        return;
      }
    }

    const pending = this._bootPendingByUnit.get(unitId) || [];
    pending.push(packet);
    if (pending.length > 8) pending.shift();
    this._bootPendingByUnit.set(unitId, pending);
  }

  _tap(callback) {
    this._frameTaps.add(callback);
    return () => this._frameTaps.delete(callback);
  }

  _waitFrame(match, timeoutMs) {
    return new Promise((resolve, reject) => {
      const w = { match, done: false, resolve, reject };
      w.timer = setTimeout(() => {
        if (!w.done) {
          w.done = true;
          this._pruneFrameWaiters();
          reject(new Error('CAN frame timeout'));
        }
      }, timeoutMs);
      this._frameWaiters.push(w);
    });
  }

  _waitBootPacket(unitId, timeoutMs) {
    const pending = this._bootPendingByUnit.get(unitId);
    if (pending?.length) {
      const packet = pending.shift();
      if (!pending.length) this._bootPendingByUnit.delete(unitId);
      else this._bootPendingByUnit.set(unitId, pending);
      return Promise.resolve(packet);
    }

    return new Promise((resolve, reject) => {
      const w = { unitId, done: false, resolve, reject };
      w.timer = setTimeout(() => {
        if (!w.done) {
          w.done = true;
          this._pruneBootWaiters();
          reject(new Error('Bootloader response timeout'));
        }
      }, timeoutMs);
      this._bootWaiters.push(w);
    });
  }

  async _sendCan(canId, data, reliable = false) {
    const payload = encodeSendCanFrame(canId, data);

    const writeOnce = async () => {
      const ok = reliable
        ? await bleService.writeCommandWithResponse(payload, this._commandLockToken)
        : await bleService.writeCommand(payload, this._commandLockToken);
      if (!ok) throw new Error('BLE write failed');
    };

    const writeTask = this._canTxQueue.then(writeOnce, writeOnce);
    this._canTxQueue = writeTask.catch(() => {});
    await writeTask;
  }

  async _sendCanBatch(frames) {
    if (!frames?.length) return;
    const payload = encodeSendCanFrames(frames);

    const writeOnce = async () => {
      const ok = await bleService.writeCommand(payload, this._commandLockToken);
      if (!ok) throw new Error('BLE batch write failed');
    };

    const writeTask = this._canTxQueue.then(writeOnce, writeOnce);
    this._canTxQueue = writeTask.catch(() => {});
    await writeTask;
  }

  async _setCanPassthrough(enabled) {
    const ok = await bleService.writeCommandWithResponse(
      encodeSetCanPassthrough(enabled),
      this._commandLockToken,
    );
    if (!ok) throw new Error(enabled ? 'Failed to enable CAN passthrough' : 'Failed to disable CAN passthrough');
  }

  async _sendBootPayload(unitId, payloadBytes, chunkDelayMs = CHUNK_DELAY_MS, reliable = false, useBatch = false) {
    const canId = canIdWithUnit(CAN_ID_BOOT_DATA, unitId);
    this._suspendHeartbeatAck = true;
    try {
      let batch = [];
      for (let i = 0; i < payloadBytes.length; i += CHUNK_CAN_DATA) {
        const chunk = payloadBytes.slice(i, i + CHUNK_CAN_DATA);
        const dataBytes = [chunk.length, ...chunk];
        while (dataBytes.length < 8) dataBytes.push(0);

        if (useBatch && !reliable) {
          batch.push({ canId, dataBytes });
          if (batch.length >= MAX_CAN_FRAMES_PER_BLE_WRITE) {
            await this._sendCanBatch(batch);
            batch = [];
            if (chunkDelayMs) await delay(chunkDelayMs);
          }
        } else {
          await this._sendCan(canId, dataBytes, reliable);
          if (chunkDelayMs) await delay(chunkDelayMs);
        }
      }
      if (batch.length) await this._sendCanBatch(batch);
    } finally {
      this._suspendHeartbeatAck = false;
    }
  }

  async _sendControl(unitId, serialBytes, productId, bridgeId, command) {
    const data = [
      serialBytes[0] & 0xff,
      serialBytes[1] & 0xff,
      serialBytes[2] & 0xff,
      serialBytes[3] & 0xff,
      serialBytes[4] & 0xff,
      productId & 0xff,
      bridgeId & 0xff,
      command & 0xff,
    ];
    await this._sendCan(canIdWithUnit(CAN_ID_STATUS, unitId), data, true);
  }

  async _ackHeartbeat(unitId, statusData) {
    if (this._suspendHeartbeatAck) return;
    const ack = [...statusData.slice(0, 7), BOOT_ACK];
    await this._sendCan(canIdWithUnit(CAN_ID_STATUS, unitId), ack);
  }

  _parseStatusFrame(frame) {
    if (!frame || (frame.canId >>> 8) !== CAN_ID_STATUS) return null;
    const d = frame.data || [];
    if (d.length < 8) return null;

    const serial = d[0] * 100000000 + d[1] * 1000000 + d[2] * 10000 + d[3] * 100 + d[4];
    return {
      unitId: frame.canId & 0xff,
      serial,
      productId: d[5] & 0xff,
      bridgeId: d[6] & 0xff,
      state: d[7] & 0xff,
      data: d,
    };
  }

  async _enterBootloader(applicationCanId, onLog) {
    const canId = canIdWithUnit((CAN_ID_ENTER_BOOT_HI | (applicationCanId & 0xff)) >>> 0, PC_UNIT_ID);
    onLog?.(
      `Enter-bootloader frame CAN 0x${toHex(canId, 8)} cmd=0x${toHex(ENTER_BOOT_MODE_COMMAND)} + reset 0x${toHex(ENTER_BOOT_RESET_COMMAND)} (x${ENTER_BOOT_BURST_COUNT})`,
    );

    for (let i = 0; i < ENTER_BOOT_BURST_COUNT; i++) {
      await this._sendCan(canId, [0x00, ENTER_BOOT_MODE_COMMAND, 0, 0, 0, 0, 0, 0], true);
      if (i + 1 < ENTER_BOOT_BURST_COUNT) await delay(ENTER_BOOT_BURST_GAP_MS);
    }
    for (let i = 0; i < ENTER_BOOT_BURST_COUNT; i++) {
      await this._sendCan(canId, [0x00, ENTER_BOOT_RESET_COMMAND, 0, 0, 0, 0, 0, 0], true);
      if (i + 1 < ENTER_BOOT_BURST_COUNT) await delay(ENTER_BOOT_BURST_GAP_MS);
    }
    await delay(ENTER_BOOT_DELAY_MS);
  }

  async _waitHeartbeat(expectedCanId, expectedSerial, timeoutMs, onLog) {
    const start = Date.now();
    let fallback = null;
    let seen = 0;
    let mismatchLogs = 0;

    while (Date.now() - start < timeoutMs) {
      let frame;
      try {
        frame = await this._waitFrame((f) => {
          const c = this._parseStatusFrame(f);
          return !!c && c.state === BOOT_READY;
        }, Math.max(100, timeoutMs - (Date.now() - start)));
      } catch {
        continue;
      }

      const c = this._parseStatusFrame(frame);
      if (!c) continue;

      seen++;
      if (!fallback) fallback = c;
      await this._ackHeartbeat(c.unitId, c.data);

      if (expectedSerial != null && c.serial === expectedSerial) {
        onLog?.(`Heartbeat (serial match) CAN ${c.unitId}`);
        return c;
      }

      const canMatch = expectedCanId == null || c.unitId === expectedCanId;
      const serialMatch = expectedSerial == null || c.serial === expectedSerial || c.serial === NO_SERIAL_VALUE;
      if (canMatch && serialMatch) {
        if (seen <= 3) onLog?.(`Heartbeat accepted CAN ${c.unitId}`);
        return c;
      }

      if (mismatchLogs < 6) {
        mismatchLogs++;
        const expected = expectedSerial == null ? '-' : String(expectedSerial).padStart(10, '0');
        const actual = c.serial === NO_SERIAL_VALUE ? 'NO_SERIAL' : String(c.serial).padStart(10, '0');
        onLog?.(`Heartbeat mismatch: CAN ${c.unitId}, serial ${actual} (expected serial ${expected})`);
      }
    }

    if (fallback) {
      onLog?.(`Using fallback heartbeat candidate CAN ${fallback.unitId}`);
      return fallback;
    }

    throw new Error(
      `No bootloader heartbeat (expected CAN ${expectedCanId ?? '-'}, serial ${expectedSerial ?? '-'})`,
    );
  }

  async _scanForHeartbeat(expectedCanId, expectedSerial, durationMs, onLog) {
    const deadline = Date.now() + durationMs;
    let fallback = null;
    const expectedUnitId = expectedCanId == null ? null : Number(expectedCanId) & 0xff;

    while (Date.now() < deadline) {
      let frame;
      try {
        frame = await this._waitFrame((f) => {
          if ((f.canId >>> 8) !== CAN_ID_STATUS) return false;
          const d = f.data || [];
          return d.length >= 8 && d[7] === BOOT_READY;
        }, Math.max(50, deadline - Date.now()));
      } catch {
        return fallback;
      }

      const d = frame.data;
      const serial = d[0] * 100000000 + d[1] * 1000000 + d[2] * 10000 + d[3] * 100 + d[4];
      const unitId = frame.canId & 0xff;
      const candidate = { unitId, serial, productId: d[5] & 0xff, bridgeId: d[6] & 0xff };
      await this._ackHeartbeat(unitId, d);
      const canMatches = expectedUnitId == null || unitId === expectedUnitId;

      if (expectedSerial != null && serial === expectedSerial) {
        onLog?.(`Pre-scan: already in bootloader (CAN ${unitId})`);
        return candidate;
      }
      if (canMatches && (expectedSerial == null || serial === NO_SERIAL_VALUE)) {
        onLog?.(`Pre-scan: already in bootloader (CAN ${unitId})`);
        return candidate;
      }
      if (canMatches && !fallback) fallback = candidate;
    }
    return fallback;
  }

  _startKeepAlive(unitId, onLog) {
    let first = true;
    return this._tap((frame) => {
      if ((frame.canId >>> 8) !== CAN_ID_STATUS) return;
      if ((frame.canId & 0xff) !== unitId) return;
      const d = frame.data || [];
      if (d.length < 8 || d[7] !== BOOT_READY) return;
      this._ackHeartbeat(unitId, d).catch(() => {});
      if (first) {
        first = false;
        onLog?.(`Keepalive ACK active on CAN ${unitId}`);
      }
    });
  }

  async _openBridge(unitId, serialBytes, productId, bridgeId, onLog, timeoutMs = BRIDGE_OPEN_TIMEOUT_MS) {
    onLog?.(`Opening bridge ${bridgeId} on CAN ${unitId}`);
    const deadline = Date.now() + timeoutMs;
    let nextControlAt = 0;
    let lastSeenBridge = null;

    while (Date.now() < deadline) {
      if (Date.now() >= nextControlAt) {
        await this._sendControl(unitId, serialBytes, productId, bridgeId, 0x02);
        nextControlAt = Date.now() + 700;
      }

      let frame;
      try {
        frame = await this._waitFrame((f) => {
          if ((f.canId >>> 8) !== CAN_ID_STATUS) return false;
          if ((f.canId & 0xff) !== unitId) return false;
          const d = f.data || [];
          return d.length >= 8 && d[7] === BOOT_READY;
        }, Math.min(450, Math.max(60, deadline - Date.now())));
      } catch {
        continue;
      }

      lastSeenBridge = frame.data[6] & 0xff;
      if (lastSeenBridge === bridgeId) {
        onLog?.(`Bridge ${bridgeId} confirmed`);
        return;
      }
    }

    throw new Error(
      `Bridge open timeout for bridge ${bridgeId}` +
      (lastSeenBridge != null ? ` (last seen bridge ${lastSeenBridge})` : ''),
    );
  }

  async _requestBootInfo(unitId, timeoutMs = 3000) {
    await this._sendBootPayload(unitId, buildRequestInfoMsg(), CHUNK_DELAY_MS, true);
    const packet = await this._waitBootPacket(unitId, timeoutMs);

    if (packet[0] !== BOOT_START_BYTE || packet[1] !== CMD_READ_VERSION) {
      throw new Error('Unexpected bootloader info response');
    }
    if ((packet[4] & 0xff) !== STATUS_OK) {
      throw new Error(`Bootloader info status 0x${toHex(packet[4])}`);
    }

    const dataLen = packet[2] | (packet[3] << 8);
    if (dataLen !== 16 || packet.length < 23) {
      throw new Error('Invalid bootloader info payload');
    }

    return {
      version: packet[7] | (packet[8] << 8),
      deviceId: packet[9] | (packet[10] << 8),
      erasePageSize: packet[11] | (packet[12] << 8),
      minWriteBlock: packet[13] | (packet[14] << 8),
      flashStart: (packet[15] | (packet[16] << 8) | (packet[17] << 16) | (packet[18] << 24)) >>> 0,
      flashEnd: (packet[19] | (packet[20] << 8) | (packet[21] << 16) | (packet[22] << 24)) >>> 0,
    };
  }

  async _sendAndAwait(
    unitId,
    payload,
    expectedCmd,
    timeoutMs,
    retries,
    chunkDelayMs = CHUNK_DELAY_MS,
  ) {
    for (let attempt = 0; attempt <= retries; attempt++) {
      try {
        this._rxAssembly.delete(unitId);
        this._bootPendingByUnit.delete(unitId);

        await this._sendBootPayload(
          unitId,
          payload,
          chunkDelayMs,
          expectedCmd !== CMD_WRITE_FLASH,
          expectedCmd === CMD_WRITE_FLASH && this._canFrameBatchEnabled,
        );

        const deadline = Date.now() + timeoutMs;

        while (true) {
          const remaining = deadline - Date.now();
          if (remaining <= 0) throw new Error('Bootloader response timeout');

          const packet = await this._waitBootPacket(unitId, remaining);
          if (!packet?.length || packet[0] !== BOOT_START_BYTE) continue;

          if (packet[1] !== expectedCmd) {
            if (expectedCmd === CMD_WRITE_FLASH) {
              continue;
            }

            throw new Error(
              `Unexpected response cmd 0x${toHex(packet[1])} (expected 0x${toHex(expectedCmd)})`,
            );
          }

          const status = packet[4] & 0xff;
          if (status === STATUS_OK) {
            if (expectedCmd === CMD_WRITE_FLASH && WRITE_BLOCK_SETTLE_DELAY_MS) {
              await delay(WRITE_BLOCK_SETTLE_DELAY_MS);
            }
            return;
          }
          if (status === STATUS_BAD_LENGTH && expectedCmd === CMD_WRITE_FLASH) throw new Error('BAD_LENGTH');
          if (status === STATUS_BAD_CRC) throw new Error('BAD_CRC');
          if (status === STATUS_TIMEOUT) {
            throw new Error(`Bootloader timeout status 0x${toHex(status)} on cmd 0x${toHex(expectedCmd)}`);
          }
          throw new Error(`Bootloader status 0x${toHex(status)} on cmd 0x${toHex(expectedCmd)}`);
        }
      } catch (err) {
        if (attempt >= retries) throw err;
        const msg = String(err?.message || '');
        if (
          msg.includes('timeout') ||
          msg === 'BAD_LENGTH' ||
          msg === 'BAD_CRC' ||
          (expectedCmd === CMD_WRITE_FLASH && msg.includes('Unexpected response cmd'))
        ) {
          if (expectedCmd === CMD_WRITE_FLASH && this._canFrameBatchEnabled && msg.includes('timeout')) {
            this._canFrameBatchEnabled = false;
          }
          if (expectedCmd === CMD_WRITE_FLASH && WRITE_BLOCK_SETTLE_DELAY_MS) {
            await delay(WRITE_BLOCK_SETTLE_DELAY_MS);
          }
          continue;
        }
        throw err;
      }
    }
    throw new Error(`Cmd 0x${toHex(expectedCmd)} failed`);
  }

  async _exitBootloader(unitId, serialBytes, productId, onLog = null) {
    try {
      for (let i = 0; i < EXIT_BOOTLOADER_REPEAT_COUNT; i++) {
        await this._sendControl(unitId, serialBytes, productId, 0x00, 0x04);
        if (i + 1 < EXIT_BOOTLOADER_REPEAT_COUNT) await delay(EXIT_BOOTLOADER_REPEAT_GAP_MS);
      }
      await delay(EXIT_BOOTLOADER_SETTLE_DELAY_MS);
      onLog?.('Bootloader exit sent');
    } catch (err) {
      onLog?.(`Bootloader exit send failed: ${err?.message || err}`);
    }
  }

  async _resetAfterUpload(unitId, onLog) {
    try {
      await this._sendAndAwait(
        unitId,
        buildResetMsg(),
        CMD_RESET_DEVICE,
        RESET_DEVICE_TIMEOUT_MS,
        RESET_DEVICE_RETRIES,
      );
      return { acked: true, statusTimeout: false };
    } catch (err) {
      const msg = String(err?.message || '');
      if (/Bootloader response timeout/i.test(msg)) {
        onLog?.('Reset response timed out after upload; target may already be restarting');
        return { acked: false, statusTimeout: false };
      }
      if (/Bootloader timeout status 0xFA/i.test(msg)) {
        onLog?.('Reset command returned bootloader status 0xFA after retries; continuing with bridge exit');
        return { acked: false, statusTimeout: true };
      }
      throw err;
    }
  }

  async _sendApplicationReset(applicationCanId, onLog) {
    const canId = canIdWithUnit((CAN_ID_ENTER_BOOT_HI | (applicationCanId & 0xff)) >>> 0, PC_UNIT_ID);
    onLog?.(`Sending application reset CAN 0x${toHex(canId, 8)} cmd=0x${toHex(ENTER_BOOT_RESET_COMMAND)}`);

    for (let i = 0; i < POST_UPDATE_APP_RESET_COUNT; i++) {
      await this._sendCan(canId, [0x00, ENTER_BOOT_RESET_COMMAND, 0, 0, 0, 0, 0, 0], true);
      if (i + 1 < POST_UPDATE_APP_RESET_COUNT) await delay(POST_UPDATE_APP_RESET_GAP_MS);
    }
    await delay(POST_UPDATE_APP_RESET_SETTLE_MS);
  }

  _updateRawDevice(frame, devicesByAddr) {
    if (!frame || !Number.isInteger(frame.canId)) return;

    const idMsg = (frame.canId >>> 8) & 0xffffff;
    const unitId = frame.canId & 0xff;
    if (unitId < 0 || unitId > 255) return;

    const data = Array.isArray(frame.data) ? frame.data : [];
    const current = devicesByAddr.get(unitId) || {
      applicationCanId: unitId,
      partNumber: '',
      serialNumber: '',
      bridgeFirmwareVersions: {},
      _partHi: '',
      _partLo: '',
    };

    if ((idMsg === CAN_MSG_SERIAL_A || idMsg === CAN_MSG_SERIAL_B) && data.length >= 8) {
      const major = (data[3] & 0xff) * 10000 + (data[4] & 0xff) * 100 + (data[5] & 0xff);
      const minor = (data[6] & 0xff) * 100 + (data[7] & 0xff);
      current.serialNumber = `${String(major).padStart(6, '0')}-${String(minor).padStart(4, '0')}`;
    }
    if (idMsg === CAN_MSG_PART_HIGH) {
      current._partHi = asciiFromCanData(data);
      current.partNumber = `${current._partHi}${current._partLo}`.trim();
    }
    if (idMsg === CAN_MSG_PART_LOW) {
      current._partLo = asciiFromCanData(data);
      current.partNumber = `${current._partHi}${current._partLo}`.trim();
    }

    if ((idMsg >>> 8) === 0x19ef && data.length >= 8 && data[1] === 0x40) {
      const block = data[2] & 0xff;
      const paramId = data[3] & 0xff;
      const request = FIRMWARE_VERSION_REQUESTS.find((r) => r.block === block && r.id === paramId);

      if (request) {
        const versionInt = (
          (data[4] & 0xff) |
          ((data[5] & 0xff) << 8) |
          ((data[6] & 0xff) << 16) |
          ((data[7] & 0xff) << 24)
        ) >>> 0;

        if (versionInt > 0) {
          current.bridgeFirmwareVersions[request.bridgeId] = versionInt;
        }
      }
    }

    devicesByAddr.set(unitId, current);
  }

  async _discoverTargets(onLog, { keepPassthrough = false } = {}) {
    const devices = new Map();
    const removeTap = this._tap((f) => this._updateRawDevice(f, devices));
    let enabledHere = false;

    try {
      await this._setCanPassthrough(true);
      enabledHere = true;
      await delay(120);

      await this._sendCan(CAN_ID_INFO_BROADCAST, [0x01, 0xff, 0x00, 0, 0, 0, 0, 0]);
      await delay(250);
      await this._sendCan(CAN_ID_INFO_BROADCAST, [0x01, 0xff, 0x00, 0, 0, 0, 0, 0]);
      await delay(2000);

      for (const unitId of devices.keys()) {
        for (const request of FIRMWARE_VERSION_REQUESTS) {
          await this._sendCan(
            canIdWithUnit(0x19ef00 | (unitId & 0xff), PC_UNIT_ID),
            [0x00, 0x40, request.block, request.id, 0x00, 0x00, 0x00, 0x00],
          );
          await delay(80);
        }
      }

      if (devices.size > 0) {
        await delay(900);
      }
    } finally {
      removeTap();
      if (enabledHere && !keepPassthrough) {
        try {
          await this._setCanPassthrough(false);
        } catch {
        }
      }
    }

    const targets = Array.from(devices.values()).map((d) => ({
      applicationCanId: d.applicationCanId,
      partNumber: String(d.partNumber || '').trim(),
      serialNumber: String(d.serialNumber || '').trim(),
      bridgeFirmwareVersions: { ...(d.bridgeFirmwareVersions || {}) },
      source: 'raw-can',
    }));
    onLog?.(`Discovery found ${targets.length} target(s)`);
    return targets;
  }

  _buildBridgeUpdatePlan(firmwareItems, bridgeFirmwareVersions) {
    const byBridge = new Map();
    for (const item of firmwareItems || []) {
      const bridgeId = normalizeBridgeId(item?.bridgeId);
      if (bridgeId == null) continue;

      const versionInt = comparableVersionInt(item);
      if (!Number.isFinite(versionInt) || versionInt <= 0) continue;

      if (!byBridge.has(bridgeId)) byBridge.set(bridgeId, []);
      byBridge.get(bridgeId).push({ ...item, versionInt });
    }

    const rows = [];
    for (let bridgeId = 1; bridgeId <= MAX_SUPPORTED_BRIDGE_ID; bridgeId++) {
      const currentRaw = bridgeFirmwareVersions?.[bridgeId] ?? bridgeFirmwareVersions?.[String(bridgeId)] ?? 0;
      const currentVersionInt = Number(currentRaw);
      if (!Number.isFinite(currentVersionInt) || currentVersionInt <= 0) continue;

      const allForBridge = (byBridge.get(bridgeId) || [])
        .slice()
        .sort((a, b) => b.versionInt - a.versionInt);

      const latestAny = allForBridge[0] || null;
      const currentHardware = Math.floor(currentVersionInt / 100);
      const latestCompatible = allForBridge
        .filter((item) => Math.floor(item.versionInt / 100) === currentHardware)
        .sort((a, b) => b.versionInt - a.versionInt)[0] || null;

      const latestVersionInt = latestCompatible?.versionInt || 0;
      const latestVersionString = latestCompatible?.versionString || '';

      let status = 'up-to-date';
      let targetVersionString = latestVersionString || firmwareIntToVersion(currentVersionInt);
      let statusText = `Latest installed (v${targetVersionString})`;

      if (latestVersionInt > currentVersionInt) {
        status = 'updatable';
        statusText = `Can update to v${latestVersionString}`;
        targetVersionString = latestVersionString;
      }

      rows.push({
        bridgeId,
        currentVersionInt,
        currentVersionString: firmwareIntToVersion(currentVersionInt),
        latestVersionInt,
        latestVersionString,
        latestAnyVersionInt: latestAny?.versionInt || 0,
        latestAnyVersionString: latestAny?.versionString || '',
        status,
        statusText,
        targetVersionString,
        updateAvailable: latestVersionInt > currentVersionInt,
      });
    }

    return rows;
  }

  _pickTarget(targets, { preferredCanId, preferredPartNumber, preferredSerialNumber } = {}) {
    if (!Array.isArray(targets) || !targets.length) return null;

    if (preferredCanId != null) {
      const hit = targets.find((t) => t.applicationCanId === Number(preferredCanId));
      if (hit) return hit;

      if (preferredSerialNumber) {
        const hint = digitsOnly(preferredSerialNumber);
        const serialHit = targets.find((t) => digitsOnly(t.serialNumber) === hint);
        if (serialHit) return serialHit;
      }

      return null;
    }
    if (preferredSerialNumber) {
      const hint = digitsOnly(preferredSerialNumber);
      const hit = targets.find((t) => digitsOnly(t.serialNumber) === hint);
      if (hit) return hit;
    }
    if (preferredPartNumber) {
      const hit = targets.find((t) => eqIgnoreCase(t.partNumber, preferredPartNumber));
      if (hit) return hit;
    }
    return targets.find((t) => t.partNumber || t.serialNumber) || targets[0];
  }

  async detectTarget(options = {}) {
    if (!bleService.isConnected) throw new Error('BLE not connected');
    if (this._active) throw new Error('Update already running');

    this._lockCommandChannel();
    try {
      this._attach();
      const targets = await this._discoverTargets(options.onLog);
      const picked = this._pickTarget(targets, options);
      if (!picked) throw new Error('No targets discovered');
      options.onLog?.(
        `Auto target: CAN ${picked.applicationCanId}, PN ${picked.partNumber || '-'}, SN ${picked.serialNumber || '-'}`,
      );
      return picked;
    } finally {
      this._detach();
      this._unlockCommandChannel();
    }
  }

  async _fetchLastKnownFirmware(apiBaseUrl, apiKey, serialNumber) {
    const formattedSerial = String(serialNumber || '').trim();
    if (!formattedSerial) return null;

    const url =
      `${apiBaseUrl.replace(/\/$/, '')}/api/v1/bootloader/logs/last-known/` +
      `${encodeURIComponent(formattedSerial)}`;

    try {
      const res = await fetch(url, { headers: { 'X-Api-Key': apiKey } });
      if (!res.ok) return null;

      const body = await res.json();
      const part = String(body?.partNumber || '').trim();
      const bridge = normalizeBridgeId(body?.bridgeId);
      const version = String(body?.version || '').trim();

      if (!part || bridge == null) return null;
      return { serial: formattedSerial, partNumber: part, bridgeId: bridge, version };
    } catch {
      return null;
    }
  }

  async _downloadFirmware(apiBaseUrl, apiKey, partNumber, bridgeId, versionString) {
    const url =
      `${apiBaseUrl.replace(/\/$/, '')}/api/v1/firmware/download` +
      `?partNumber=${encodeURIComponent(partNumber)}` +
      `&bridgeId=${bridgeId}` +
      `&version=${encodeURIComponent(versionString || '')}`;
    const res = await fetch(url, { headers: { 'X-Api-Key': apiKey } });
    if (!res.ok) throw new Error(`Download HTTP ${res.status}`);
    return new Uint8Array(await res.arrayBuffer());
  }

  async getAvailableFirmware({ apiBaseUrl, apiKey, partNumber }) {
    const qs = partNumber
      ? `?partNumber=${encodeURIComponent(partNumber)}&includePrototype=false`
      : '?includePrototype=false';
    const url = `${apiBaseUrl.replace(/\/$/, '')}/api/v1/firmware/catalog${qs}`;
    const res = await fetch(url, { headers: { 'X-Api-Key': apiKey } });
    if (!res.ok) throw new Error(`Catalog HTTP ${res.status}`);

    const body = await res.json();
    const items = Array.isArray(body?.items) ? body.items : [];
    const releasedItems = items.filter(
      (item) => !(item?.isPrototype || item?.prototype || item?.isUnreleased || item?.unreleased),
    );

    return releasedItems
      .filter((i) => !partNumber || eqIgnoreCase(i.partNumber, partNumber))
      .sort((a, b) =>
        comparableVersionInt(b) - comparableVersionInt(a),
      );
  }

  async getTargetUpdatePlan({ apiBaseUrl, apiKey, partNumber, bridgeFirmwareVersions }) {
    if (!apiBaseUrl || !apiKey) throw new Error('API base URL and API key required');

    const normalizedPart = String(partNumber || '').trim();
    if (!normalizedPart) return [];

    const firmwareItems = await this.getAvailableFirmware({
      apiBaseUrl,
      apiKey,
      partNumber: normalizedPart,
    });

    return this._buildBridgeUpdatePlan(firmwareItems, bridgeFirmwareVersions || {});
  }

  async runFirmwareUpdate({
    applicationCanId,
    serialNumber,
    partNumber,
    bridgeId,
    apiBaseUrl,
    apiKey,
    versionString,
    onProgress,
    onLog,
    signal,
  }) {
    if (!bleService.isConnected) throw new Error('BLE not connected');
    if (this._active) throw new Error('Update already running');
    if (!apiBaseUrl || !apiKey) throw new Error('API base URL and API key required');

    const report = (p, msg) => {
      onProgress?.({ progress: Math.max(0, Math.min(100, p)), message: msg || '' });
      if (msg) onLog?.(msg);
    };
    const checkCancel = () => {
      if (signal?.aborted) throw new Error('Update cancelled');
    };

    this._lockCommandChannel();
    this._active = true;
    this._canFrameBatchEnabled = true;

    let heartbeat = null;
    let serialBytes = serialToBytes(serialNumber);
    let stopKeepAlive = null;
    let bootloaderExitSent = false;
    let passthroughOn = false;
    let resolvedCanId = applicationCanId == null ? null : Number(applicationCanId);
    let resolvedPart = String(partNumber || '').trim();
    let resolvedSerial = String(serialNumber || '').trim();
    let resolvedBridgeId = bridgeId == null ? null : normalizeBridgeId(bridgeId);
    let lastKnownVersion = '';

    try {
      this._attach();

      const gatewayInfo = bleService.getConnectedDeviceInfo?.();
      if (gatewayInfo) {
        onLog?.(`BLE gateway ${gatewayInfo.name} (${gatewayInfo.id}), MTU ${gatewayInfo.mtu || '-'}`);
      }

      if (resolvedCanId == null || !resolvedPart || !resolvedSerial) {
        report(2, 'Auto-detecting target...');
        const targets = await this._discoverTargets(onLog, { keepPassthrough: true });
        passthroughOn = true;
        const picked = this._pickTarget(targets, {
          preferredCanId: resolvedCanId,
          preferredPartNumber: resolvedPart,
          preferredSerialNumber: resolvedSerial,
        });
        if (!picked) throw new Error('No target found on CAN bus');

        if (resolvedCanId == null) resolvedCanId = picked.applicationCanId;
        if (!resolvedPart) resolvedPart = picked.partNumber || '';
        if (!resolvedSerial) resolvedSerial = picked.serialNumber || '';
        serialBytes = serialToBytes(resolvedSerial);
      }

      if (resolvedCanId == null || !Number.isInteger(resolvedCanId)) {
        throw new Error('Application CAN ID required');
      }

      if (!passthroughOn) {
        report(5, 'Enabling CAN passthrough...');
        await this._setCanPassthrough(true);
        passthroughOn = true;
        await delay(150);
      }

      report(6, `Target CAN ${resolvedCanId}, PN ${resolvedPart || '-'}, SN ${resolvedSerial || '-'}`);

      const expectedSerialNum = resolvedSerial
        ? Number(digitsOnly(resolvedSerial).padStart(10, '0'))
        : null;

      checkCancel();
      report(8, 'Scanning for existing bootloader heartbeat...');
      heartbeat = await this._scanForHeartbeat(resolvedCanId, expectedSerialNum, HEARTBEAT_PRESCAN_MS, onLog);

      if (!heartbeat) {
        let lastErr = null;
        for (let attempt = 1; attempt <= ENTER_BOOT_RETRIES; attempt++) {
          checkCancel();
          report(10, `Entering bootloader (attempt ${attempt}/${ENTER_BOOT_RETRIES})...`);
          await this._enterBootloader(resolvedCanId, onLog);

          if (ENABLE_BOOTLOADER_SET_CANID_INIT && digitsOnly(resolvedSerial).length >= 10) {
            onLog?.(`Sending bootloader init SET_CANID for CAN ${resolvedCanId}`);
            for (let i = 0; i < 3; i++) {
              await this._sendControl(resolvedCanId, serialBytes, 0x00, 0x00, 0x00);
              await delay(90);
            }
          }

          try {
            onLog?.('Waiting for bootloader heartbeat (CAN ID may change after enter)');
            heartbeat = await this._waitHeartbeat(null, expectedSerialNum, HEARTBEAT_WAIT_MS, onLog);
            break;
          } catch (err) {
            lastErr = err;
            onLog?.(`No heartbeat on attempt ${attempt}: ${err?.message || err}`);

            if (attempt < ENTER_BOOT_RETRIES) {
              try {
                onLog?.('Re-discovering target before retry...');
                const rediscovered = await this._discoverTargets(onLog, { keepPassthrough: true });
                passthroughOn = true;
                const remapped = this._pickTarget(rediscovered, {
                  preferredPartNumber: resolvedPart,
                  preferredSerialNumber: resolvedSerial,
                });
                if (remapped?.applicationCanId != null && remapped.applicationCanId !== resolvedCanId) {
                  onLog?.(
                    `Target CAN remapped ${resolvedCanId} -> ${remapped.applicationCanId} from rediscovery`,
                  );
                  resolvedCanId = remapped.applicationCanId;
                }
              } catch (rediscoveryErr) {
                onLog?.(`Rediscovery skipped: ${rediscoveryErr?.message || rediscoveryErr}`);
              }
            }
          }
        }
        if (!heartbeat) throw lastErr || new Error('No bootloader heartbeat');
      }

      if (heartbeat.serial === NO_SERIAL_VALUE && resolvedSerial) {
        await this._sendControl(heartbeat.unitId, serialBytes, heartbeat.productId, 0x00, 0x01);
        await delay(400);
        onLog?.(`Copied serial ${resolvedSerial} to bootloader`);
      } else if (heartbeat.serial !== NO_SERIAL_VALUE) {
        const fromHb = serialFromNumeric(heartbeat.serial);
        if (fromHb) {
          serialBytes = serialToBytes(fromHb);
          if (!resolvedSerial) resolvedSerial = fromHb;
        }
      } else {
        serialBytes = [...NO_SERIAL_BYTES];
      }

      if (!resolvedPart || resolvedBridgeId == null || !lastKnownVersion) {
        const historySerial = normalizeSerialForHistory(resolvedSerial, NO_SERIAL_VALUE);
        if (historySerial) {
          onLog?.(`Looking up device history for S/N: ${historySerial}...`);
          const lastKnown = await this._fetchLastKnownFirmware(apiBaseUrl, apiKey, historySerial);
          if (lastKnown) {
            onLog?.(`Found device history: ${lastKnown.partNumber} (Bridge ${lastKnown.bridgeId})`);
            if (lastKnown.version) {
              lastKnownVersion = String(lastKnown.version || '').trim();
              onLog?.(`Last known version: v${lastKnownVersion}`);
            }
            if (!resolvedPart) {
              resolvedPart = lastKnown.partNumber;
              onLog?.(`Recovered part number from history: ${resolvedPart}`);
            }
            if (resolvedBridgeId == null) {
              resolvedBridgeId = lastKnown.bridgeId;
              onLog?.(`Recovered bridge ID from history: ${resolvedBridgeId}`);
            }
          } else {
            onLog?.('No device history found');
          }
        } else if (!resolvedPart) {
          onLog?.('Device serial unavailable in bootloader mode; history-based recovery unavailable');
        }
      }

      if (resolvedBridgeId == null && heartbeat.bridgeId > 0) {
        resolvedBridgeId = heartbeat.bridgeId;
      }

      stopKeepAlive = this._startKeepAlive(heartbeat.unitId, onLog);

      checkCancel();
      report(18, 'Loading firmware catalog...');
      if (!resolvedPart) {
        const all = await this.getAvailableFirmware({ apiBaseUrl, apiKey, partNumber: '' });
        const scoped = resolvedBridgeId != null
          ? all.filter((i) => Number(i.bridgeId) === Number(resolvedBridgeId))
          : all;
        const uniqueParts = Array.from(new Set(scoped.map((i) => String(i.partNumber || '').trim()).filter(Boolean)));

        if (uniqueParts.length === 1) {
          resolvedPart = uniqueParts[0];
          onLog?.(`Part number resolved: ${resolvedPart}`);
        } else if (uniqueParts.length > 1) {
          const historySerial = normalizeSerialForHistory(resolvedSerial, NO_SERIAL_VALUE);
          const recoveryHint = historySerial
            ? ` History lookup for serial ${historySerial} did not resolve a unique part.`
            : ' Serial number is unavailable, so history-based recovery cannot resolve part number.';
          throw new Error(`Part number ambiguous for bridge ${resolvedBridgeId}: ${uniqueParts.join(', ')}.${recoveryHint}`);
        } else {
          throw new Error('Could not resolve part number from catalog');
        }
      }

      const fwList = await this.getAvailableFirmware({ apiBaseUrl, apiKey, partNumber: resolvedPart });
      if (!fwList.length) throw new Error(`No firmware available for ${resolvedPart}`);

      const catalogBridgeIds = uniqueBridgeIds(fwList).filter((id) => id >= 1 && id <= MAX_SUPPORTED_BRIDGE_ID);
      if (resolvedBridgeId == null) {
        if (catalogBridgeIds.length === 1) {
          resolvedBridgeId = catalogBridgeIds[0];
          onLog?.(`Bridge ID resolved from catalog: ${resolvedBridgeId}`);
        } else {
          resolvedBridgeId = normalizeBridgeId(fwList[0]?.bridgeId);
          if (resolvedBridgeId != null) onLog?.(`Bridge ID resolved from firmware metadata: ${resolvedBridgeId}`);
        }
      }

      if (!Number.isInteger(resolvedBridgeId) || resolvedBridgeId <= 0) {
        throw new Error('Could not resolve bridge ID from firmware metadata');
      }
      if (resolvedBridgeId > MAX_SUPPORTED_BRIDGE_ID) {
        throw new Error(`Unsupported bridge ${resolvedBridgeId}. Supported bridges: 1-${MAX_SUPPORTED_BRIDGE_ID}`);
      }

      let firmwareCandidates = fwList.filter((i) => normalizeBridgeId(i.bridgeId) === resolvedBridgeId);
      if (!firmwareCandidates.length) {
        throw new Error(`No firmware available for bridge ${resolvedBridgeId} (${resolvedPart})`);
      }

      let firmware = null;
      if (versionString) {
        firmware = firmwareCandidates.find((i) => i.versionString === versionString);
        if (!firmware) throw new Error(`Firmware ${versionString} not found for ${resolvedPart}`);
      } else {
        const targetFamily = versionFamily(lastKnownVersion);
        if (!targetFamily) {
          throw new Error('Cannot auto-select firmware family without known device version. Provide version explicitly or ensure device history exists.');
        }

        firmwareCandidates = firmwareCandidates.filter((i) => versionFamily(i.versionString) === targetFamily);
        if (!firmwareCandidates.length) {
          throw new Error(`No released firmware found in family ${targetFamily}.x for bridge ${resolvedBridgeId} (${resolvedPart})`);
        }

        onLog?.(`Auto-select constrained to family ${targetFamily}.x`);
        firmware = firmwareCandidates[0];
      }

      checkCancel();
      report(26, `Opening bridge ${resolvedBridgeId}...`);
      await this._openBridge(heartbeat.unitId, serialBytes, heartbeat.productId, resolvedBridgeId, onLog);
      await delay(BRIDGE_INIT_DELAY_MS);

      checkCancel();
      report(34, `Downloading firmware ${firmware.versionString}...`);
      const hexBytes = await this._downloadFirmware(
        apiBaseUrl,
        apiKey,
        firmware.partNumber,
        normalizeBridgeId(firmware.bridgeId) || resolvedBridgeId,
        firmware.versionString,
      );

      checkCancel();
      report(40, 'Requesting bootloader info...');
      const info = await this._requestBootInfo(heartbeat.unitId, 3000);
      if (!info.erasePageSize || info.flashEnd <= info.flashStart) {
        throw new Error('Invalid bootloader flash parameters');
      }

      onLog?.(`Flash 0x${toHex(info.flashStart, 8)}-0x${toHex(info.flashEnd, 8)}, page ${info.erasePageSize}B`);

      checkCancel();
      report(46, 'Decoding HEX image...');
      const hexText = new TextDecoder().decode(hexBytes);
      const image = await parseIntelHex(hexText, info.flashStart, info.flashEnd);
      if (!image?.length) throw new Error('HEX image is empty');

      checkCancel();
      const pages = Math.ceil(image.length / info.erasePageSize);
      report(50, `Erasing ${pages} pages...`);
      await this._sendAndAwait(
        heartbeat.unitId,
        buildEraseMsg(info.flashStart, pages),
        CMD_ERASE_FLASH,
        pages * 20 + 1500,
        1,
      );

      checkCancel();
      report(54, 'Packing firmware blocks...');
      const packets = buildWritePackets(image, info.flashStart);
      if (!packets.length) throw new Error('No writable packets generated');
      const queue = packets.length > 1 ? [...packets.slice(1), packets[0]] : packets;

      let lastUploadPct = null;
      for (let i = 0; i < queue.length; i++) {
        checkCancel();
        const pct = 54 + Math.round(((i + 1) / queue.length) * 40);
        if (i === 0 || i + 1 === queue.length || pct !== lastUploadPct) {
          report(pct, `Uploading block ${i + 1}/${queue.length}`);
          lastUploadPct = pct;
        }

        let uploaded = false;
        for (let attempt = 0; attempt < WRITE_BLOCK_RECOVERY_ATTEMPTS && !uploaded; attempt++) {
          try {
            await this._sendAndAwait(
              heartbeat.unitId,
              queue[i],
              CMD_WRITE_FLASH,
              WRITE_CMD_TIMEOUT_MS,
              WRITE_RETRIES,
              WRITE_CHUNK_DELAY_MS,
            );
            uploaded = true;
          } catch (err) {
            if (/(timeout|0xFA|BAD_LENGTH|bad length|0xFD)/i.test(String(err?.message || '')) && attempt + 1 < WRITE_BLOCK_RECOVERY_ATTEMPTS) {
              onLog?.(
                `Block ${i + 1} write recovery; reopening bridge and retrying quickly (${attempt + 1}/${WRITE_BLOCK_RECOVERY_ATTEMPTS - 1})`,
              );
              await this._openBridge(
                heartbeat.unitId,
                serialBytes,
                heartbeat.productId,
                resolvedBridgeId,
                onLog,
                BRIDGE_OPEN_TIMEOUT_MS,
              );
              await delay(WRITE_RECOVERY_REOPEN_DELAY_MS);
              continue;
            }
            throw err;
          }
        }
      }

      checkCancel();
      report(96, 'Resetting target device...');
      if (FINAL_WRITE_SETTLE_DELAY_MS) {
        onLog?.(`Final write accepted; waiting ${FINAL_WRITE_SETTLE_DELAY_MS}ms before reset`);
        await delay(FINAL_WRITE_SETTLE_DELAY_MS);
      }
      const resetResult = await this._resetAfterUpload(heartbeat.unitId, onLog);

      checkCancel();
      report(98, 'Closing bootloader bridge...');
      await this._exitBootloader(heartbeat.unitId, serialBytes, heartbeat.productId, onLog);
      bootloaderExitSent = true;

      if (resolvedBridgeId === 3 && resetResult?.statusTimeout && resolvedCanId != null) {
        checkCancel();
        report(99, 'Restarting display application...');
        await this._sendApplicationReset(resolvedCanId, onLog);
      }

      report(100, `Firmware update complete (${firmware.versionString})`);
      return {
        success: true,
        bridgeId: resolvedBridgeId,
        partNumber: resolvedPart,
        version: firmware.versionString,
        bootloaderCanId: heartbeat.unitId,
      };
    } finally {
      if (stopKeepAlive) {
        stopKeepAlive();
        stopKeepAlive = null;
      }
      if (heartbeat && !bootloaderExitSent) {
        await this._exitBootloader(heartbeat.unitId, serialBytes, heartbeat.productId, onLog);
      }
      if (passthroughOn) {
        try {
          await this._setCanPassthrough(false);
        } catch {
        }
      }
      this._active = false;
      this._detach();
      this._unlockCommandChannel();
    }
  }
}

export default new FirmwareUpdateService();
