import { AppState } from 'react-native';
import deviceSession from '../devices/deviceSession';
import { activeErrorDefinitions } from '../utils/errorCodes';
import { postNotification } from './notifier';

// Turns dashboard snapshots into notifications while the app is in the
// background: new errors (all levels), low battery and fully charged.
// State is tracked in the foreground too, so returning to the background
// never re-announces something the user has already seen in the app.

const LOW_SOC = 20; //  notify when SoC drops below this…
const LOW_REARM = 25; // …and again only after it has been back above this
const FULL_SOC = 99.5;
const FULL_REARM = 95;

const freshState = () => ({ codes: new Set(), lowArmed: true, fullArmed: true });

let enabled = false;
let state = freshState();
let unitKey = null;

const inBackground = () => AppState.currentState !== 'active';

function notify(title, body) {
  if (enabled && inBackground()) postNotification(title, body);
}

function checkErrors(errorCodes) {
  const defs = activeErrorDefinitions(errorCodes);
  const active = new Set(defs.map((d) => d.code));
  defs
    .filter((d) => !state.codes.has(d.code))
    .forEach((d) => notify(d.title, d.description));
  state.codes = active; // a code that clears and returns is announced again
}

function checkBattery(soc, charging) {
  if (!(soc > 0)) return; // no real value yet (drivers start at 0)

  if (soc < LOW_SOC && state.lowArmed) {
    state.lowArmed = false;
    notify('Battery low', `State of charge is ${Math.round(soc)} %.`);
  } else if (soc > LOW_REARM) {
    state.lowArmed = true;
  }

  if (soc >= FULL_SOC && charging && state.fullArmed) {
    state.fullArmed = false;
    notify('Battery fully charged', 'The battery has reached 100 %.');
  } else if (soc < FULL_REARM) {
    state.fullArmed = true;
  }
}

function onDashboard(data) {
  const key = data.serial || data.partNumber || 'unit';
  if (key !== unitKey) {
    unitKey = key;
    state = freshState();
  }
  checkErrors(data.errorCodes);
  checkBattery(data.soc, data.batteryCurrent > 0.5);
}

let started = false;

/** Subscribe once at app start; setEnabled() follows the user's setting. */
export function startAlertMonitor() {
  if (started) return;
  started = true;
  // State is kept per unit across reconnects, so a flaky link does not
  // re-announce errors that were already notified.
  deviceSession.onNotification((message) => {
    if (message.type === 'dashboard') onDashboard(message.data);
  });
}

export function setAlertsEnabled(value) {
  enabled = !!value;
}
