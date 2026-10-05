// Shared unit-type helpers.
// Classification mirrors the ESP32 firmware: part numbers starting with
// "CL" are LPS units, "CB" are BMS/battery units; the numeric type from
// display driver (1 = LPS, 2 = BMS) is the fallback when no part number
// has been received yet.

export const DEV_UNKNOWN = 0;
export const DEV_LPS = 1;
export const DEV_BMS = 2;

/**
 * Classify a unit as 'lps' | 'bms' | 'unknown' from its part number
 * (preferred) or its numeric device type.
 */
export function unitFamily({ type, partNumber } = {}) {
  const pn = String(partNumber || '').toUpperCase();
  if (pn.startsWith('CL')) return 'lps';
  if (pn.startsWith('CB')) return 'bms';
  if (type === DEV_BMS) return 'bms';
  if (type === DEV_LPS) return 'lps';
  return 'unknown';
}

export function isBmsUnit(unit) {
  return unitFamily(unit) === 'bms';
}
