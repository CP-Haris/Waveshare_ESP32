// LPS2 UART framing, carried 1:1 over the built-in BLE module:
//   <SOH> payload… <CRCL> <CRCH> <EOT>
// Control bytes inside payload/CRC are escaped with DLE. CRC-16-CCITT
// (XMODEM: poly 0x1021, init 0) over the unescaped payload, low byte first.
// Mirrors SendMSG()/COM_GetRXData() in LPS2 Display Firmware/firmware/src/UART_App.c.

const SOH = 0x01;
const EOT = 0x04;
const DLE = 0x10;

// The LPS2 display drops frames longer than this (UART_App.c).
const MAX_FRAME_BYTES = 20;

export function crc16(bytes) {
  let crc = 0;
  for (let i = 0; i < bytes.length; i++) {
    crc ^= (bytes[i] & 0xff) << 8;
    for (let bit = 0; bit < 8; bit++) {
      crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
      crc &= 0xffff;
    }
  }
  return crc;
}

function pushEscaped(out, byte) {
  if (byte === SOH || byte === EOT || byte === DLE) out.push(DLE);
  out.push(byte);
}

/** Frame a payload (array of bytes) for sending. */
export function encodeFrame(payload) {
  const crc = crc16(payload);
  const out = [SOH];
  payload.forEach((byte) => pushEscaped(out, byte & 0xff));
  pushEscaped(out, crc & 0xff);
  pushEscaped(out, (crc >> 8) & 0xff);
  out.push(EOT);
  return out;
}

/**
 * Byte-stream parser. BLE notifications may split or join frames, so feed
 * every notification to push(); it returns the payloads of all complete,
 * CRC-valid frames seen so far.
 */
export class FrameParser {
  constructor() {
    this._buffer = null; // null = waiting for SOH
    this._escaped = false;
  }

  push(bytes) {
    const payloads = [];
    for (let i = 0; i < bytes.length; i++) {
      const byte = bytes[i];

      if (!this._escaped && byte === DLE) {
        this._escaped = true;
        continue;
      }
      if (!this._escaped && byte === SOH) {
        this._buffer = [];
        continue;
      }
      if (!this._escaped && byte === EOT) {
        const frame = this._buffer;
        this._buffer = null;
        if (frame && frame.length >= 3) {
          const payload = frame.slice(0, -2);
          const crc = frame[frame.length - 2] | (frame[frame.length - 1] << 8);
          if (crc16(payload) === crc) payloads.push(payload);
        }
        continue;
      }

      this._escaped = false;
      if (this._buffer) {
        this._buffer.push(byte);
        if (this._buffer.length > MAX_FRAME_BYTES) this._buffer = null;
      }
    }
    return payloads;
  }
}

export function bytesFromBase64(base64) {
  const raw = atob(base64);
  const bytes = new Uint8Array(raw.length);
  for (let i = 0; i < raw.length; i++) bytes[i] = raw.charCodeAt(i);
  return bytes;
}

export function base64FromBytes(bytes) {
  return btoa(String.fromCharCode(...bytes));
}
