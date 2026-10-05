// The dashboard snapshot every device driver emits ('dashboard' messages).
// Screens only read these fields, so they never need to know which chip the
// data came from. Drivers may add extra raw fields of their own.

/** Normalized function fault: what the dashboard shows, independent of protocol. */
export const FAULT = {
  NONE: 'none',
  WARNING: 'warning', // icon yellow, function still runs
  BLOCKED: 'blocked', // icon red, function stopped by a failure
};

/**
 * One controllable or measured function (AC in, DC in, solar, AC out, DC out).
 * `on` = running (or starting); `powerW` = live power, always >= 0.
 */
export function functionStatus(on = false, fault = FAULT.NONE, powerW = 0) {
  return { on: !!on, fault, powerW: Number.isFinite(powerW) ? Math.max(0, powerW) : 0 };
}

export function emptyFunctions() {
  return {
    acIn: functionStatus(),
    dcIn: functionStatus(),
    solar: functionStatus(),
    acOut: functionStatus(),
    dcOut: functionStatus(),
  };
}

/**
 * Snapshot fields the UI relies on:
 * - soc: 0..100
 * - batteryVoltage (V), batteryCurrent (A, + while charging, − while discharging)
 * - socTimeMin: minutes until full (charging) or empty (discharging), >= 0
 * - functions: { acIn, dcIn, solar, acOut, dcOut } of functionStatus()
 * - errorCodes: active error codes (src/utils/errorCodes.js)
 * - unitType, partNumber, serial: identity (src/utils/units.js)
 */
export function emptyDashboard() {
  return {
    soc: 0,
    batteryVoltage: 0,
    batteryCurrent: 0,
    socTimeMin: 0,
    functions: emptyFunctions(),
    errorCodes: [],
    unitType: 0,
    partNumber: '',
    serial: '',
  };
}
