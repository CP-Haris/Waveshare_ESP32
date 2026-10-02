import { useState, useEffect, useMemo, useRef, useCallback } from "react";
import { Dimensions } from "react-native";
import { useIsFocused } from "@react-navigation/native";
import CommService from "../ble/services/CommService";
import { bleService } from "../ble/BLEService";
import { Logger } from "../components/Logger";
import { ErrorDefinition } from "../ble/services/ErrorDefinitions";
import { updateDeviceSoC as saveDeviceSoCToStorage, hasSoCChanged } from "../storage/DeviceStorage";

function removeMinusPrefix(value: string): string {
  return value.startsWith("-") ? value.slice(1) : value;
}

const { width } = Dimensions.get("window");
const guidelineBaseWidth = 375;
export const scaleFont = (size: number) => (width / guidelineBaseWidth) * size;

export interface DeviceInfoValues {
  dcinOpState: string;
  dcinWatt: string;
  acinOpState: string;
  acinWatt: string;
  solarOpState: string;
  solarWatt: string;
  dcOutOpState: string;
  dcOutWatt: string;
  dcOutAmp: string;
  acOutOpState: string;
  acOutWatt: string;
  batterySOC: string;
  timeRemaining: string;
  batteryStatus: string;
}

export interface ComponentFailures {
  dcInput: {
    hasFailure: boolean;
    failureCount: number;
    errors: ErrorDefinition[];
  };
  acInput: {
    hasFailure: boolean;
    failureCount: number;
    errors: ErrorDefinition[];
  };
  solar: {
    hasFailure: boolean;
    failureCount: number;
    errors: ErrorDefinition[];
  };
  dcOutput: {
    hasFailure: boolean;
    failureCount: number;
    errors: ErrorDefinition[];
  };
  acOutput: {
    hasFailure: boolean;
    failureCount: number;
    errors: ErrorDefinition[];
  };
}

const initialValues: DeviceInfoValues = {
  dcinOpState: "Off",
  dcinWatt: "-",
  acinOpState: "Off",
  acinWatt: "-",
  solarOpState: "Off",
  solarWatt: "-",
  dcOutOpState: "Off",
  dcOutWatt: "-",
  dcOutAmp: "-",
  acOutOpState: "Off",
  acOutWatt: "-",
  batterySOC: "-",
  timeRemaining: "-",
  batteryStatus: "",
};

const initialFailures: ComponentFailures = {
  dcInput: { hasFailure: false, failureCount: 0, errors: [] },
  acInput: { hasFailure: false, failureCount: 0, errors: [] },
  solar: { hasFailure: false, failureCount: 0, errors: [] },
  dcOutput: { hasFailure: false, failureCount: 0, errors: [] },
  acOutput: { hasFailure: false, failureCount: 0, errors: [] },
};

// Function to analyze errors and determine component failures
const analyzeComponentFailures = (activeErrors: ErrorDefinition[]): ComponentFailures => {
  const failures: ComponentFailures = {
    dcInput: { hasFailure: false, failureCount: 0, errors: [] },
    acInput: { hasFailure: false, failureCount: 0, errors: [] },
    solar: { hasFailure: false, failureCount: 0, errors: [] },
    dcOutput: { hasFailure: false, failureCount: 0, errors: [] },
    acOutput: { hasFailure: false, failureCount: 0, errors: [] },
  };

  activeErrors.forEach(error => {
    if (error.affects.dcInput) {
      failures.dcInput.errors.push(error);
      failures.dcInput.failureCount++;
      failures.dcInput.hasFailure = true;
    }
    if (error.affects.charger) { // AC Input is related to charger
      failures.acInput.errors.push(error);
      failures.acInput.failureCount++;
      failures.acInput.hasFailure = true;
    }
    if (error.affects.solar) {
      failures.solar.errors.push(error);
      failures.solar.failureCount++;
      failures.solar.hasFailure = true;
    }
    if (error.affects.dcOutput) {
      failures.dcOutput.errors.push(error);
      failures.dcOutput.failureCount++;
      failures.dcOutput.hasFailure = true;
    }
    if (error.affects.inverter) { // AC Output is related to inverter
      failures.acOutput.errors.push(error);
      failures.acOutput.failureCount++;
      failures.acOutput.hasFailure = true;
    }
  });

  return failures;
};

const useDeviceInfo = (deviceId: string, onErrorBuffer?: (errorBuffer: Uint8Array) => void, activeErrors: ErrorDefinition[] = []) => {
  const [values, setValues] = useState<DeviceInfoValues>(initialValues);
  const [failures, setFailures] = useState<ComponentFailures>(initialFailures);
  const isFocused = useIsFocused();
  const commService = useMemo(() => new CommService(deviceId), [deviceId]);
  const pendingUpdatesRef = useRef<DeviceInfoValues>(initialValues);
  const animationFrameRef = useRef<number>();
  const lastUpdateTimeRef = useRef<number>(0);
  const isConnectedRef = useRef<boolean>(false);

  // SoC sync: Get access to device list context to sync SoC back
  const [updateDeviceListSoC, setUpdateDeviceListSoC] = useState<((deviceId: string, soc: string) => void) | null>(null);
  
  // Refs for logging throttling
  const lastSyncedSoC = useRef<string>('');
  const lastLogTime = useRef<number>(0);
  const hasLoggedMissingUpdate = useRef<boolean>(false);

  // Update component failures when active errors change
  useEffect(() => {
    const newFailures = analyzeComponentFailures(activeErrors);
    setFailures(newFailures);
  }, [activeErrors]);

  const updateUI = useCallback(() => {
    const now = Date.now();
    if (now - lastUpdateTimeRef.current < 50) {
      animationFrameRef.current = requestAnimationFrame(updateUI);
      return;
    }

    lastUpdateTimeRef.current = now;
    setValues(pendingUpdatesRef.current);
    animationFrameRef.current = undefined;
  }, []);

  const messageHandler = useCallback((message: string) => {
    // Handle error buffer messages
    if (message.startsWith("ERROR_BUFFER:") && onErrorBuffer) {
      const errorBuffer = new Uint8Array(
        message.split(":")[1].split(" ").map(hex => parseInt(hex, 16))
      );
      onErrorBuffer(errorBuffer);
      return;
    }

    const parts = message.split(":");
    if (parts.length < 2) return;
    const paramName = parts[0].trim();
    const paramValue = parts.slice(1).join(":").trim();
    
    const newValues = { ...pendingUpdatesRef.current };

    switch (paramName) {
      case "DCIN_OPSTATE":
        if (newValues.dcinOpState !== paramValue) {
          Logger.debug('DCIN_OPSTATE changed:', newValues.dcinOpState, '->', paramValue);
        }
        newValues.dcinOpState = paramValue;
        break;
      case "DCIN_WATT": {
        const num = parseFloat(removeMinusPrefix(paramValue));
        newValues.dcinWatt = isNaN(num) ? "-" : Math.round(num).toString() + "W";
        break;
      }
      case "ACIN_OPSTATE":
        if (newValues.acinOpState !== paramValue) {
          Logger.debug('ACIN_OPSTATE changed:', newValues.acinOpState, '->', paramValue);
        }
        newValues.acinOpState = paramValue;
        break;
      case "ACIN_WATT": {
        const num = parseFloat(removeMinusPrefix(paramValue));
        newValues.acinWatt = isNaN(num) ? "-" : Math.round(num).toString() + "W";
        break;
      }
      case "SOLAR_OPSTATE":
        if (newValues.solarOpState !== paramValue) {
          Logger.debug('SOLAR_OPSTATE changed:', newValues.solarOpState, '->', paramValue);
        }
        newValues.solarOpState = paramValue;
        break;
      case "SOLAR_WATT": {
        const num = parseFloat(removeMinusPrefix(paramValue));
        newValues.solarWatt = isNaN(num) ? "-" : Math.round(num).toString() + "W";
        break;
      }
      case "DCOUT_OPSTATE":
        if (newValues.dcOutOpState !== paramValue) {
          Logger.debug('DCOUT_OPSTATE changed:', newValues.dcOutOpState, '->', paramValue);
        }
        newValues.dcOutOpState = paramValue;
        break;
      case "DCOUT_AMP": {
        const num = parseFloat(removeMinusPrefix(paramValue));
        newValues.dcOutAmp = isNaN(num) ? "-" : num.toFixed(1) + "A";
        break;
      }
      case "DCOUT_WATT": {
        const num = parseFloat(removeMinusPrefix(paramValue));
        newValues.dcOutWatt = isNaN(num) ? "-" : Math.round(num).toString() + "W";
        break;
      }
      case "ACOUT_OPSTATE":
        if (newValues.acOutOpState !== paramValue) {
          Logger.debug('ACOUT_OPSTATE changed:', newValues.acOutOpState, '->', paramValue);
        }
        newValues.acOutOpState = paramValue;
        break;
      case "ACOUT_WATT": {
        const num = parseFloat(removeMinusPrefix(paramValue));
        newValues.acOutWatt = isNaN(num) ? "-" : Math.round(num).toString() + "W";
        break;
      }
      case "BATSTATUS_SOC": {
        const num = parseFloat(paramValue);
        const socRounded = isNaN(num) ? "-" : Math.round(num);
        const socString = socRounded === 100 ? "100%" : socRounded + "%";
        newValues.batterySOC = socString;
        
        // Performance optimization: Only sync when SoC actually changes
        if (updateDeviceListSoC && deviceId && socString && !isNaN(num) && num >= 0 && num <= 100) {
          // Check if value has changed before doing expensive operations
          if (hasSoCChanged(deviceId, socString)) {
            // Save to storage first (with built-in change detection)
            saveDeviceSoCToStorage(deviceId, socString);
            
            // Then sync to device list
            updateDeviceListSoC(deviceId, socString);
            lastSyncedSoC.current = socString;
          }
          // No need for verbose logging when value hasn't changed
        } else if (!updateDeviceListSoC) {
          // Only log this error once
          if (!hasLoggedMissingUpdate.current) {
            console.log(`[DeviceInfo] updateDeviceListSoC not available yet`);
            hasLoggedMissingUpdate.current = true;
          }
        } else if (isNaN(num) || num < 0 || num > 100) {
          // Only log validation errors occasionally
          if (Date.now() - lastLogTime.current > 5000) {
            console.log(`[DeviceInfo] SoC sync skipped: deviceId=${deviceId}, socString=${socString}, num=${num}`);
            lastLogTime.current = Date.now();
          }
        }
        break;
      }
      case "BATSTATUS_REMTIME": {
        const match = paramValue.match(/(-?\d+)\s*hr\s*:\s*(\d+)\s*min/);
        if (match) {
          let hours = parseInt(match[1], 10);
          let minutes = parseInt(match[2], 10);
          if (hours < 0) hours = Math.abs(hours);
          if (hours > 99) hours = 99;
          newValues.timeRemaining = `${hours} hr : ${minutes} min`;
        } else {
          newValues.timeRemaining = paramValue;
        }
        break;
      }
      case "BATSTATUS":
        newValues.batteryStatus = paramValue;
        break;
    }
    pendingUpdatesRef.current = newValues;

    if (!animationFrameRef.current) {
      animationFrameRef.current = requestAnimationFrame(updateUI);
    }
  }, [updateUI, onErrorBuffer, updateDeviceListSoC, deviceId]);

  const resetValues = useCallback(() => {
    setValues(initialValues);
    setFailures(initialFailures);
    pendingUpdatesRef.current = initialValues;
    if (animationFrameRef.current) {
      cancelAnimationFrame(animationFrameRef.current);
      animationFrameRef.current = undefined;
    }
    isConnectedRef.current = false;
  }, []);

  useEffect(() => {
    if (!isFocused) {
      commService.stopListening();
      resetValues();
      return;
    }

    resetValues();

    const checkAndConnect = async () => {
      try {
        const device = await bleService.getDevice(deviceId);
        if (device?.isConnected()) {
          isConnectedRef.current = true;
          return true;
        }

        await bleService.connectToDevice(deviceId);
        isConnectedRef.current = true;
        return true;
      } catch (error) {
        console.error("Error connecting to device:", error);
        resetValues();
        return false;
      }
    };

    if (isConnectedRef.current) {
      commService.listenForMessages(messageHandler, false);
    } else {
      checkAndConnect().then((isConnected) => {
        if (isConnected) {
          commService.listenForMessages(messageHandler, false);
        }
      });
    }

    return () => {
      commService.stopListening();
      resetValues();
    };
  }, [isFocused, deviceId, updateUI, resetValues, messageHandler]);

  useEffect(() => {
    resetValues();
  }, [deviceId, resetValues]);

  const handleToggle12V = async () => {
    try {
      Logger.debug('12V button pressed - sending toggle command');
      await commService.sendMessage({ block: 0, id: 203, rawValue: 0x50000 });
    } catch (error) {
      console.error("Error toggling 12 V:", error);
    }
  };

  const handleToggle24V = async () => {
    try {
      Logger.debug('230V button pressed - sending toggle command');
      await commService.sendMessage({ block: 0, id: 201, rawValue: 0x00000 });
    } catch (error) {
      console.error("Error toggling 230 V:", error);
    }
  };

  // Debug information object
  const debugInfo = useMemo(() => ({
    operational: {
      dcInput: values.dcinOpState,
      acInput: values.acinOpState,
      solar: values.solarOpState,
      dcOutput: values.dcOutOpState,
      acOutput: values.acOutOpState,
    },
    power: {
      dcInput: values.dcinWatt,
      acInput: values.acinWatt,
      solar: values.solarWatt,
      dcOutput: values.dcOutWatt,
      acOutput: values.acOutWatt,
    },
    battery: {
      soc: values.batterySOC,
      status: values.batteryStatus,
      timeRemaining: values.timeRemaining,
    },
    failures: {
      dcInput: failures.dcInput.hasFailure ? `${failures.dcInput.failureCount} errors` : 'OK',
      acInput: failures.acInput.hasFailure ? `${failures.acInput.failureCount} errors` : 'OK',
      solar: failures.solar.hasFailure ? `${failures.solar.failureCount} errors` : 'OK',
      dcOutput: failures.dcOutput.hasFailure ? `${failures.dcOutput.failureCount} errors` : 'OK',
      acOutput: failures.acOutput.hasFailure ? `${failures.acOutput.failureCount} errors` : 'OK',
    },
    communication: {
      crcStats: commService.getCrcStats ? commService.getCrcStats() : null
    }
  }), [values, failures, commService]);

  return { 
    values, 
    failures,
    debugInfo,
    handleToggle12V, 
    handleToggle24V, 
    resetValues,
    commService,
    isConnectedRef,
    setUpdateDeviceListSoC, // SoC fix: Allow setting the sync function from outside
  };
};

export default useDeviceInfo;
