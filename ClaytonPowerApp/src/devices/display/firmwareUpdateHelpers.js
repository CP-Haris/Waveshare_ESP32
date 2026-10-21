export const delay = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

export const toHex = (value, width = 2) =>
  value.toString(16).toUpperCase().padStart(width, '0');

export const canIdWithUnit = (top24, unit) =>
  (((top24 & 0xffffff) << 8) | (unit & 0xff)) >>> 0;

function hexLineToBytes(hexText) {
  const out = new Array(hexText.length / 2);
  for (let i = 0; i < out.length; i++) {
    out[i] = parseInt(hexText.slice(i * 2, i * 2 + 2), 16);
  }
  return out;
}

export async function parseIntelHex(text, appStart, appEnd) {
  const TYPE_DATA = 0;
  const TYPE_EOF = 1;
  const TYPE_ELA = 4;

  const image = new Uint8Array(appEnd - appStart + 1);
  image.fill(0xff);

  let highest = 0;
  let extHi = 0;
  let extLo = 0;

  const lines = text.split(/\r?\n/);
  for (let lineNo = 0; lineNo < lines.length; lineNo++) {
    const line = lines[lineNo].trim();
    if (!line) continue;
    if (!line.startsWith(':')) throw new Error(`Invalid HEX line ${lineNo + 1}`);

    const buf = hexLineToBytes(line.slice(1));
    if (buf.length < 5) throw new Error(`Short HEX line ${lineNo + 1}`);

    let sum = 0;
    for (let i = 0; i < buf.length - 1; i++) sum = (sum + buf[i]) & 0xff;
    if (((0x100 - sum) & 0xff) !== buf[buf.length - 1]) {
      throw new Error(`HEX checksum failed line ${lineNo + 1}`);
    }

    const length = buf[0];
    const addr = (buf[1] << 8) | buf[2];
    const type = buf[3];

    if (type === TYPE_EOF) break;
    if (type === TYPE_ELA) {
      extHi = buf[4] || 0;
      extLo = buf[5] || 0;
      continue;
    }
    if (type !== TYPE_DATA) continue;

    const fullAddr = ((extHi << 24) | (extLo << 16) | addr) >>> 0;
    if (fullAddr < appStart || fullAddr > appEnd) continue;

    const writeOff = fullAddr - appStart;
    const copyLen = Math.min(length, appEnd - fullAddr);
    for (let i = 0; i < copyLen; i++) image[writeOff + i] = buf[4 + i];
    highest = Math.max(highest, writeOff + copyLen);

    if ((lineNo & 0x1ff) === 0) await delay(0);
  }

  return image.slice(0, highest);
}

export function serialToBytes(serialText) {
  const clean = String(serialText || '')
    .replace(/[^0-9]/g, '')
    .padStart(10, '0')
    .slice(-10);
  return [
    parseInt(clean.slice(0, 2), 10),
    parseInt(clean.slice(2, 4), 10),
    parseInt(clean.slice(4, 6), 10),
    parseInt(clean.slice(6, 8), 10),
    parseInt(clean.slice(8, 10), 10),
  ];
}

export function serialFromNumeric(numericSerial) {
  const n = Number(numericSerial);
  if (!Number.isFinite(n) || n <= 0) return '';
  const major = Math.floor(n / 10000);
  const minor = n % 10000;
  return `${String(major).padStart(6, '0')}-${String(minor).padStart(4, '0')}`;
}

export function digitsOnly(value) {
  return String(value || '').replace(/[^0-9]/g, '');
}

export function normalizeSerialForHistory(serialText, noSerialValue = 25757575755) {
  const digits = digitsOnly(serialText);
  if (!digits) return '';
  const asNumber = Number(digits);
  if (Number.isFinite(asNumber) && asNumber === noSerialValue) return '';
  return digits.padStart(10, '0').slice(-10);
}

export function eqIgnoreCase(a, b) {
  return String(a || '').trim().toUpperCase() === String(b || '').trim().toUpperCase();
}

export function semverToInt(versionString) {
  const m = String(versionString || '').trim().match(/^(\d+)\.(\d+)[\.:](\d+)$/);
  return m ? Number(m[1]) * 10000 + Number(m[2]) * 100 + Number(m[3]) : 0;
}

export function normalizeBridgeId(value) {
  const id = Number(value);
  return Number.isInteger(id) && id > 0 ? id : null;
}

export function uniqueBridgeIds(items) {
  const out = [];
  const seen = new Set();
  for (const item of items || []) {
    const id = normalizeBridgeId(item?.bridgeId);
    if (id != null && !seen.has(id)) {
      seen.add(id);
      out.push(id);
    }
  }
  return out;
}

export function asciiFromCanData(data) {
  if (!Array.isArray(data) || !data.length) return '';
  let out = '';
  for (const b of data) {
    const value = b & 0xff;
    if (value === 0 || value === 0xff) break;
    if (value >= 32 && value <= 126) out += String.fromCharCode(value);
  }
  return out.trim();
}
