import AsyncStorage from '@react-native-async-storage/async-storage';

// Small persistent app preferences.

const KEYS = {
  backgroundEnabled: 'pref.backgroundEnabled',
  lastDevice: 'pref.lastDevice', // { id, name, kind } of the last connected unit
};

async function readJson(key, fallback) {
  try {
    const raw = await AsyncStorage.getItem(key);
    return raw == null ? fallback : JSON.parse(raw);
  } catch (error) {
    console.warn('[Prefs] read failed:', key, error.message);
    return fallback;
  }
}

async function writeJson(key, value) {
  try {
    if (value == null) await AsyncStorage.removeItem(key);
    else await AsyncStorage.setItem(key, JSON.stringify(value));
  } catch (error) {
    console.warn('[Prefs] write failed:', key, error.message);
  }
}

export const prefs = {
  getBackgroundEnabled: () => readJson(KEYS.backgroundEnabled, false),
  setBackgroundEnabled: (enabled) => writeJson(KEYS.backgroundEnabled, !!enabled),
  getLastDevice: () => readJson(KEYS.lastDevice, null),
  setLastDevice: (device) => writeJson(KEYS.lastDevice, device),
};
