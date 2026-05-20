export const MSG = {
  CAN_FRAME: 0x08,
};

export const CMD = {
  SET_CAN_PASSTHROUGH: 0x18,
  SEND_CAN_FRAME: 0x19,
  SEND_CAN_FRAMES: 0x1A,
};

export const BLE_SERVICE_UUID = '00001000-0000-1000-8000-00805f9b34fb';
export const BLE_TX_CHAR_UUID = '00001001-0000-1000-8000-00805f9b34fb';
export const BLE_RX_CHAR_UUID = '00001002-0000-1000-8000-00805f9b34fb';

// --- Decoders ---

export function decodeCanFrame(bytes) {
  const dv = new DataView(new Uint8Array(bytes).buffer);
  const canId = dv.getUint32(1, true);
  const dlc = Math.min(8, dv.getUint8(5));
  const data = [];
  for (let i = 0; i < dlc; i++) data.push(dv.getUint8(6 + i));
  return { canId, dlc, data };
}

// --- Encoders ---

function encodeBytes(...vals) {
  return btoa(String.fromCharCode(...vals));
}

export function encodeSetCanPassthrough(enabled) {
  return encodeBytes(CMD.SET_CAN_PASSTHROUGH, enabled ? 1 : 0);
}

export function encodeSendCanFrame(canId, dataBytes) {
  const payload = new Uint8Array(14);
  payload[0] = CMD.SEND_CAN_FRAME;

  const dv = new DataView(payload.buffer);
  dv.setUint32(1, canId >>> 0, true);

  const src = Array.isArray(dataBytes) ? dataBytes : Array.from(dataBytes || []);
  const dlc = Math.min(8, src.length);
  payload[5] = dlc;
  for (let i = 0; i < dlc; i++) payload[6 + i] = src[i] & 0xff;

  return btoa(String.fromCharCode(...payload));
}

export function encodeSendCanFrames(frames) {
  const safeFrames = Array.isArray(frames) ? frames : [];
  const payload = new Uint8Array(2 + safeFrames.length * 13);
  payload[0] = CMD.SEND_CAN_FRAMES;
  payload[1] = safeFrames.length & 0xff;

  const dv = new DataView(payload.buffer);
  let offset = 2;
  safeFrames.forEach((frame) => {
    dv.setUint32(offset, frame.canId >>> 0, true);
    offset += 4;

    const src = Array.isArray(frame.dataBytes) ? frame.dataBytes : Array.from(frame.dataBytes || []);
    const dlc = Math.min(8, src.length);
    payload[offset++] = dlc;
    for (let i = 0; i < 8; i++) payload[offset++] = i < dlc ? src[i] & 0xff : 0;
  });

  return btoa(String.fromCharCode(...payload));
}

export function decodeNotification(base64) {
  const raw = atob(base64);
  const bytes = new Uint8Array(raw.length);
  for (let i = 0; i < raw.length; i++) bytes[i] = raw.charCodeAt(i);

  const msgType = bytes[0];
  switch (msgType) {
    case MSG.CAN_FRAME:
      return { type: 'canFrame', data: decodeCanFrame(bytes) };
    default:
      return { type: 'unknown', data: bytes };
  }
}
