// GATT layout of the two BLE chips the app can talk to. See CLAUDE.md
// ("two paths") and Docs/LPS2-BLE-Protocol.md.

export const DEVICE_KIND = {
  DISPLAY: 'display', // ClaytonDisplay (ESP32) — CAN gateway, many units
  LPS2: 'lps2', //       LPS2 built-in BLE module — one unit, no CAN
};

export const GATT = {
  [DEVICE_KIND.DISPLAY]: {
    service: '00001000-0000-1000-8000-00805f9b34fb',
    notify: '00001001-0000-1000-8000-00805f9b34fb',
    write: '00001002-0000-1000-8000-00805f9b34fb',
  },
  [DEVICE_KIND.LPS2]: {
    service: '0783b03e-8535-b5a0-7140-a304d2495cb7',
    notify: '0783b03e-8535-b5a0-7140-a304d2495cb8',
    write: '0783b03e-8535-b5a0-7140-a304d2495cba',
  },
};

export const SCAN_SERVICE_UUIDS = Object.values(GATT).map((profile) => profile.service);

/** Which chip a scanned device is, from its advertised service UUIDs. */
export function kindFromServiceUuids(uuids) {
  const advertised = (uuids || []).map((uuid) => uuid.toLowerCase());
  if (advertised.includes(GATT[DEVICE_KIND.LPS2].service)) return DEVICE_KIND.LPS2;
  return DEVICE_KIND.DISPLAY;
}

function bytesFromBase64(base64) {
  const raw = atob(base64);
  const bytes = new Uint8Array(raw.length);
  for (let i = 0; i < raw.length; i++) bytes[i] = raw.charCodeAt(i);
  return bytes;
}

function uint32Le(bytes, offset) {
  return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}

/** "250314-0021" style: the last four digits form the suffix. */
export function formatLps2Serial(value) {
  const digits = String(value >>> 0);
  if (value === 0 || digits.length < 5) return '';
  return `${digits.slice(0, -4)}-${digits.slice(-4)}`;
}

// Placeholders the LPS2 advertises before its display MCU has fed real values.
const SERIAL_PLACEHOLDERS = new Set([0xaabbccdd, 0xddccbbaa]);
const SOC_PLACEHOLDERS = new Set([0x40302010, 0x10203040]);

/**
 * LPS2 manufacturer data: bytes 2..5 serial (uint32 LE), bytes 6..9 SoC
 * (Q16.16 fraction). Returns { serial, soc } with '' / null when unknown.
 */
export function parseLps2Advert(manufacturerData) {
  if (!manufacturerData) return { serial: '', soc: null };
  const bytes = bytesFromBase64(manufacturerData);
  if (bytes.length < 10) return { serial: '', soc: null };

  const serialRaw = uint32Le(bytes, 2);
  const socRaw = uint32Le(bytes, 6);
  return {
    serial: SERIAL_PLACEHOLDERS.has(serialRaw) ? '' : formatLps2Serial(serialRaw),
    soc: SOC_PLACEHOLDERS.has(socRaw) ? null : Math.round((socRaw / 65536) * 100),
  };
}
