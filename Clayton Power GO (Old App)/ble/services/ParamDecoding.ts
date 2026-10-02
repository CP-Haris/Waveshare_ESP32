import {
    VOLTAGE, CURRENT, TEMPERATURE, PROCENT, POWER, STATE_OPERATION, STATE_FAILURE,
    STATE_BATTERY, STATE_TESTMODE, MAC, PASSKEY, VERSION_LONG, VERSION_SHORT, DATE,
    TIME_HHMMSS, KWH, OHM, MASKED_BITMAP, TIME_HEAD, STRING, ERROR, SERIAL
} from './ParamDefinitions';

interface ParamInfo {
    prefix: number;
    desc: string;
}

export function getParamInfo(block: number, id: number): ParamInfo | null {
    const key = `${block}:${id}`;
    return paramTable[key] || null;
}

export const BatteryStatus = {
    "0": "Idle",
    "1": "Time left",
    "2": "Discharging",
    "3": "Charging",
    "4": "Full",
    "5": "Unknown",
};

const paramTable: { [key: string]: ParamInfo } = {
    // Block 0
    "0:70": { prefix: VOLTAGE, desc: "IO1_REMOTE" },
    "0:71": { prefix: VOLTAGE, desc: "IO2_DATA" },
    "0:72": { prefix: VOLTAGE, desc: "IO3_DATAFRONT" },
    "0:73": { prefix: VOLTAGE, desc: "C2_TERMINAL" },
    "0:74": { prefix: VOLTAGE, desc: "C1_TERMINAL" },
    "0:140": { prefix: KWH, desc: "NRGMETER_ACIN" },
    "0:142": { prefix: KWH, desc: "NRGMETER_DCIN" },
    "0:144": { prefix: KWH, desc: "NRGMETER_DCOUT" },
    "0:146": { prefix: KWH, desc: "NRGMETER_SOLAR" },
    "0:201": { prefix: STATE_OPERATION, desc: "ACOUT_OPSTATE" },
    "0:205": { prefix: STATE_FAILURE, desc: "ACOUT_FLSTATE" },
    "0:107": { prefix: POWER, desc: "ACOUT_WATT" },
    "0:105": { prefix: VOLTAGE, desc: "ACOUT_VOLT" },
    "0:106": { prefix: CURRENT, desc: "ACOUT_AMP" },
    "0:226": { prefix: TIME_HHMMSS, desc: "ACOUT_AUTO_TIMEGLOBAL" },
    "0:200": { prefix: STATE_OPERATION, desc: "ACIN_OPSTATE" },
    "0:204": { prefix: STATE_FAILURE, desc: "ACIN_FLSTATE" },
    "0:104": { prefix: POWER, desc: "ACIN_WATT" },
    "0:102": { prefix: VOLTAGE, desc: "ACIN_VOLT" },
    "0:103": { prefix: CURRENT, desc: "ACIN_AMP" },
    "0:208": { prefix: STATE_OPERATION, desc: "SOLAR_OPSTATE" },
    "0:209": { prefix: STATE_FAILURE, desc: "SOLAR_FLSTATE" },
    "0:78": { prefix: CURRENT, desc: "SOLAR_AMP" },
    "0:79": { prefix: POWER, desc: "SOLAR_WATT" },
    "0:203": { prefix: STATE_OPERATION, desc: "DCOUT_OPSTATE" },
    "0:207": { prefix: STATE_FAILURE, desc: "DCOUT_FLSTATE" },
    "0:113": { prefix: POWER, desc: "DCOUT_WATT" },
    "0:111": { prefix: VOLTAGE, desc: "DCOUT_VOLT" },
    "0:112": { prefix: CURRENT, desc: "DCOUT_AMP" },
    "0:225": { prefix: TIME_HHMMSS, desc: "DCOUT_AUTO_TIMEGLOBAL" },
    "0:202": { prefix: STATE_OPERATION, desc: "DCIN_OPSTATE" },
    "0:206": { prefix: STATE_FAILURE, desc: "DCIN_FLSTATE" },
    "0:110": { prefix: POWER, desc: "DCIN_WATT" },
    "0:108": { prefix: VOLTAGE, desc: "DCIN_VOLT" },
    "0:109": { prefix: CURRENT, desc: "DCIN_AMP" },
    "0:170": { prefix: MASKED_BITMAP, desc: "DCIN_OPERATING_VOLTAGE" },
    "0:120": { prefix: TIME_HEAD, desc: "BATSTATUS_REMTIME" },
    "0:119": { prefix: PROCENT, desc: "BATSTATUS_SOC" },
    "0:127": { prefix: POWER, desc: "BATSTATUS_WATT" },
    "0:100": { prefix: VOLTAGE, desc: "BATSTATUS_VOLT" },
    "0:101": { prefix: CURRENT, desc: "BATSTATUS_AMP" },
    "0:114": { prefix: TEMPERATURE, desc: "BATSTATUS_TEMP" },
    "0:1": { prefix: VOLTAGE, desc: "BATSTATUS_CELL1" },
    "0:2": { prefix: VOLTAGE, desc: "BATSTATUS_CELL2" },
    "0:3": { prefix: VOLTAGE, desc: "BATSTATUS_CELL3" },
    "0:4": { prefix: VOLTAGE, desc: "BATSTATUS_CELL4" },
    "0:124": { prefix: STRING, desc: "BATSTATUS_CYCLES" },
    "0:116": { prefix: TEMPERATURE, desc: "TEMPERATURE_IGBT" },
    "0:117": { prefix: TEMPERATURE, desc: "TEMPERATURE_TRAFO" },
    "0:121": { prefix: TEMPERATURE, desc: "TEMPERATURE_CELL12" },
    "0:122": { prefix: TEMPERATURE, desc: "TEMPERATURE_CELL23" },
    "0:123": { prefix: TEMPERATURE, desc: "TEMPERATURE_CELL34" },
    "0:220": { prefix: SERIAL, desc: "ABOUT_SERIAL" },
    "0:221": { prefix: DATE, desc: "ABOUT_MANUDATE" },
    "0:222": { prefix: VERSION_SHORT, desc: "ABOUT_HWVERS" },
    "0:223": { prefix: VERSION_LONG, desc: "ABOUT_SWVERSLPS" },
    "0:157": { prefix: MASKED_BITMAP, desc: "WAKEUPFLAGS" },
    "0:210": { prefix: STATE_BATTERY, desc: "BATSTATUS" },
    "0:212": { prefix: STATE_TESTMODE, desc: "SYS_TESTMODE" },
    "0:230": { prefix: VERSION_SHORT, desc: "BOOT_VERSION" },

    // Block 1
    "1:1": { prefix: TIME_HHMMSS, desc: "FUNC_JUMPSTART" },
    "1:100": { prefix: POWER, desc: "FUNC_ACIN_MAX" },
    "1:101": { prefix: POWER, desc: "FUNC_ACIN_MIN" },
    "1:102": { prefix: POWER, desc: "FUNC_ACIN_HYST" },
    "1:110": { prefix: POWER, desc: "FUNC_ACOUT_MAX" },
    "1:111": { prefix: POWER, desc: "FUNC_ACOUT_MIN" },
    "1:112": { prefix: POWER, desc: "FUNC_ACOUT_HYST" },
    "1:120": { prefix: CURRENT, desc: "FUNC_DCIN_MAX" },
    "1:121": { prefix: CURRENT, desc: "FUNC_DCIN_MIN" },
    "1:122": { prefix: CURRENT, desc: "FUNC_DCIN_HYST" },
    "1:130": { prefix: CURRENT, desc: "FUNC_DCOUT_MAX" },
    "1:131": { prefix: CURRENT, desc: "FUNC_DCOUT_MIN" },
    "1:132": { prefix: CURRENT, desc: "FUNC_DCOUT_HYST" },

    // Block 3
    "3:3": { prefix: MASKED_BITMAP, desc: "DISP_BITMAP1" },
    "3:0": { prefix: TIME_HHMMSS, desc: "DISP_LIGHT_CRG" },
    "3:1": { prefix: TIME_HHMMSS, desc: "DISP_LIGHT_DISCRG" },
    "3:4": { prefix: STRING, desc: "DISP_LOCK" },
    "3:5": { prefix: PROCENT, desc: "DISP_CONTRAST" },
    "3:6": { prefix: STRING, desc: "DISP_SPLASH" },
    "3:7": { prefix: STRING, desc: "DISP_VIEW" },

    // Block 4
    "4:0": { prefix: VOLTAGE, desc: "C1_WAKEUP_LEVEL_ACTIVATE" },
    "4:3": { prefix: VOLTAGE, desc: "C1_WAKEUP_LEVEL_DEACTIVATE" },

    // Block 6
    "6:2": { prefix: MASKED_BITMAP, desc: "ACOUT_ON_C1_SIGNAL" },
    "6:3": { prefix: STRING, desc: "UNKNOWN_PARAM_6_3" },
    "6:6": { prefix: MASKED_BITMAP, desc: "DCOUT_ON_C1_SIGNAL" },

    // Block 7
    "7:0": { prefix: STRING, desc: "UNKNOWN_PARAM_7_0" },

    // Block 30 (DCIN Settings)
    "30:0": { prefix: MASKED_BITMAP, desc: "DCIN_CONTROL_BITMAP" },
    "30:1": { prefix: STRING, desc: "DCIN_SET_OPERATION_MODE" },
    "30:7": { prefix: CURRENT, desc: "DCIN_SET_INPUT_CURR" },
    "30:12": { prefix: VOLTAGE, desc: "DCIN_SET_START_VOLTAGE" },
    "30:13": { prefix: VOLTAGE, desc: "DCIN_SET_STOP_VOLTAGE" },

    // Block 31 (Some battery / DCIN extension settings)
    "31:0": { prefix: MASKED_BITMAP, desc: "ENABLE_CHARGE_OF_STARTBAT" },
    "31:1": { prefix: CURRENT, desc: "12V_CURRENT_1" },
    "31:2": { prefix: VOLTAGE, desc: "12V_VOLTAGE_2" },
    "31:3": { prefix: CURRENT, desc: "12V_CURRENT_3" },
    "31:4": { prefix: VOLTAGE, desc: "12V_VOLTAGE_4" },
    "31:5": { prefix: CURRENT, desc: "24V_CURRENT_5" },
    "31:6": { prefix: VOLTAGE, desc: "24V_VOLTAGE_6" },
    "31:7": { prefix: CURRENT, desc: "24V_CURRENT_7" },
    "31:8": { prefix: VOLTAGE, desc: "24V_VOLTAGE_8" },
    "31:10": { prefix: TIME_HHMMSS, desc: "SOME_TIME_PARAM_31_10" },

    // Block 40 (DCOUT Settings)
    "40:0": { prefix: TIME_HHMMSS, desc: "DCOUT_STB_TIME" },
    "40:1": { prefix: TIME_HHMMSS, desc: "DCOUT_SAVER_TIME" },
    "40:2": { prefix: CURRENT, desc: "DCOUT_SAVER_CURRENT" },

    // Block 50 (AC Output Settings)
    "50:0": { prefix: PROCENT, desc: "INVERTER_CUTOFF" },
    "50:1": { prefix: TIME_HHMMSS, desc: "ACOUT_SAVER_TIME" },
    "50:2": { prefix: POWER, desc: "ACOUT_SAVER_LIMIT" },

    // Block 60 (AC Input Settings)
    "60:2": { prefix: CURRENT, desc: "ACIN_MAX_CURRENT" },

    // Block 70 (Solar Variables)
    "70:0": { prefix: STRING, desc: "SOLAR_SET_OPERATION_MODE" },

    // Block 71 (Solar Selflearn)
    "71:0": { prefix: VOLTAGE, desc: "SOLAR_SELFLEARN_OC_VOLT" },
    "71:1": { prefix: VOLTAGE, desc: "SOLAR_SELFLEARN_MPP_VOLT" },
    "71:2": { prefix: VOLTAGE, desc: "SOLAR_SELFLEARN_START_VOLT" },
    "82:0": { prefix: STRING, desc: "UNKNOWN_PARAM_82_0" },
    "82:1": { prefix: STRING, desc: "UNKNOWN_PARAM_82_1" },
    "82:2": { prefix: STRING, desc: "UNKNOWN_PARAM_82_2" },
    "82:3": { prefix: STRING, desc: "UNKNOWN_PARAM_82_3" },
    "82:4": { prefix: STRING, desc: "UNKNOWN_PARAM_82_4" },
    "82:30": { prefix: STRING, desc: "UNKNOWN_PARAM_82_30" },

    "83:0": { prefix: STRING, desc: "UNKNOWN_PARAM_83_0" },
    "83:30": { prefix: STRING, desc: "UNKNOWN_PARAM_83_30" },

    // Block 200 (Internal variables)
    "200:0": { prefix: VERSION_LONG, desc: "INTERNAL_SW_VERSION" },
    "200:1": { prefix: MASKED_BITMAP, desc: "INTERNAL_RCON" },
    "200:2": { prefix: ERROR, desc: "INTERNAL_ERR_BUFF0" },
    "200:3": { prefix: ERROR, desc: "INTERNAL_ERR_BUFF1" },
    "200:4": { prefix: ERROR, desc: "INTERNAL_ERR_BUFF2" },
    "200:5": { prefix: ERROR, desc: "INTERNAL_ERR_BUFF3" },
    "200:6": { prefix: ERROR, desc: "INTERNAL_ERR_BUFF4" },
    "200:7": { prefix: ERROR, desc: "INTERNAL_ERR_BUFF5" },
    "200:8": { prefix: ERROR, desc: "INTERNAL_ERR_BUFF6" },
    "200:9": { prefix: ERROR, desc: "INTERNAL_ERR_BUFF7" },
    "200:10": { prefix: TIME_HHMMSS, desc: "INTERNAL_COM_TIMER" },
    "200:11": { prefix: STRING, desc: "INTERNAL_STATE" },
    "200:12": { prefix: STRING, desc: "INTERNAL_SUPPORT" },
    "200:13": { prefix: STRING, desc: "C1CustomValues" },
    // If BLUETOOTH_CODE is defined:
    "200:100": { prefix: MASKED_BITMAP, desc: "BLUETOOTH_ON" },
    "200:101": { prefix: STRING, desc: "BLUETOOTH_STATUS" },
    "200:102": { prefix: STRING, desc: "BLUETOOTH_PASSKEY" },
    "200:110": { prefix: MAC, desc: "WHITELIST_MAC_1A" },
    "200:111": { prefix: MAC, desc: "WHITELIST_MAC_1B" },
    "200:112": { prefix: MAC, desc: "WHITELIST_MAC_2A" },
    "200:113": { prefix: MAC, desc: "WHITELIST_MAC_2B" },
    "200:114": { prefix: MAC, desc: "WHITELIST_MAC_3A" },
    "200:115": { prefix: MAC, desc: "WHITELIST_MAC_3B" },
    "200:116": { prefix: MAC, desc: "WHITELIST_MAC_4A" },
    "200:117": { prefix: MAC, desc: "WHITELIST_MAC_4B" },
    "200:118": { prefix: MAC, desc: "WHITELIST_MAC_5A" },
    "200:119": { prefix: MAC, desc: "WHITELIST_MAC_5B" },

    // Block 241 (System Values)
    "241:119": { prefix: PROCENT, desc: "TOTAL_SYSTEM_SOC" },
    "241:127": { prefix: POWER, desc: "SYSTEM_POWER" },

    // Block 242
    "242:119": { prefix: PROCENT, desc: "CAPACITY_EXTENSION_SOC" },

    // Block 250 (Power board values)
    "250:222": { prefix: VERSION_SHORT, desc: "PWR_HWVERS" },
    "250:223": { prefix: VERSION_LONG, desc: "PWR_SWVERS" },
    "250:224": { prefix: STRING, desc: "PWR_224ASK_MADS" },
    "250:225": { prefix: STRING, desc: "PWR_225ASK_MADS" },
    "250:230": { prefix: VERSION_SHORT, desc: "PWR_BOOTVERSION" },

    // Block 251 (DCDC board values)
    "251:223": { prefix: VERSION_LONG, desc: "DCDC_SWVERS" },
    "251:230": { prefix: VERSION_SHORT, desc: "DCDC_BOOTVERSION" },

    // Block 252 (Display values)
    "252:230": { prefix: VERSION_SHORT, desc: "DISPLAY_BOOTVERSION" },

    // Block 255 (Test Values - if ENABLE_TEST_VALS)
    "255:0": { prefix: STRING, desc: "TESTVAL_0" },
    "255:1": { prefix: STRING, desc: "TESTVAL_1" },
    "255:2": { prefix: STRING, desc: "TESTVAL_2" },
    "255:3": { prefix: STRING, desc: "TESTVAL_3" },
    "255:4": { prefix: STRING, desc: "TESTVAL_4" },
    "255:5": { prefix: STRING, desc: "TESTVAL_5" },
    "255:6": { prefix: STRING, desc: "TESTVAL_6" },
    "255:7": { prefix: STRING, desc: "TESTVAL_7" },
    "255:8": { prefix: STRING, desc: "TESTVAL_8" },
    "255:9": { prefix: STRING, desc: "TESTVAL_9" },
    "255:10": { prefix: STRING, desc: "TESTVAL_10" },
    "255:11": { prefix: STRING, desc: "TESTVAL_11" },
    "255:12": { prefix: STRING, desc: "TESTVAL_12" },
    "255:13": { prefix: STRING, desc: "TESTVAL_13" },
    "255:14": { prefix: STRING, desc: "TESTVAL_14" },
    "255:15": { prefix: STRING, desc: "TESTVAL_15" },
    "255:255": { prefix: STRING, desc: "TESTVAL_ENABLE" },
};

export function decodeValue(prefix: number, rawValue: number, desc: string): string {

    function fixedToFloat(rv: number): number {
        return rv / 65536.0;
    }

    switch (prefix) {
        case VOLTAGE: {
            const val = fixedToFloat(rawValue);
            return `${desc}: ${val.toFixed(2)}V`;
        }
        case CURRENT: {
            const val = fixedToFloat(rawValue);
            return `${desc}: ${val.toFixed(2)}A`;
        }
        case TEMPERATURE: {
            const val = fixedToFloat(rawValue);
            return `${desc}: ${val.toFixed(1)}°C`;
        }
        case PROCENT: {
            const val = fixedToFloat(rawValue) * 100;
            return `${desc}: ${val.toFixed(1)}%`;
        }
        case POWER: {
            const val = fixedToFloat(rawValue);
            return `${desc}: ${val.toFixed(2)}W`;
        }
        case KWH: {
            const val = fixedToFloat(rawValue);
            return `${desc}: ${val.toFixed(3)}kWh`;
        }
        case OHM: {
            const val = fixedToFloat(rawValue);
            return `${desc}: ${val.toFixed(3)}Ω`;
        }
        case TIME_HHMMSS: {
            // rawValue represents a fixed-point hours value: val * 3600 = seconds.
            // Convert to h, m, s
            const hoursFloat = fixedToFloat(rawValue);
            let totalSeconds = Math.abs(hoursFloat) * 3600;

            // Round to nearest second
            totalSeconds = Math.round(totalSeconds);

            const sign = rawValue < 0 ? "-" : "";
            const hours = Math.floor(totalSeconds / 3600);
            const remainder = totalSeconds % 3600;
            const minutes = Math.floor(remainder / 60);
            const seconds = remainder % 60;

            return `${desc}: ${sign}${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
        }
        case TIME_HEAD: {
            const hoursFloat = fixedToFloat(rawValue);
            
            let totalSeconds = Math.abs(hoursFloat) * 3600;

            totalSeconds = Math.round(totalSeconds);

            const sign = rawValue < 0 ? "-" : "";
            const hours = Math.floor(totalSeconds / 3600);
            const remainder = totalSeconds % 3600;
            const minutes = Math.floor(remainder / 60);

            return `${desc}: ${sign}${hours} hr : ${minutes} min`;
        }
        case DATE: {
            return `${desc}: 0x${rawValue.toString(16)}`;
        }
        case SERIAL:
        case STRING:
        case VERSION_LONG:
        case VERSION_SHORT:
        case MAC:
        case PASSKEY: {
            return `${desc}: Raw=0x${rawValue.toString(16)}`;
        }

        case STATE_OPERATION: {
            const val = rawValue;
            let stateStr = "Unknown";
            switch (val) {
                case 0x00000: stateStr = "Off"; break;
                case 0x10000: stateStr = "Wakeup"; break;
                case 0x20000: stateStr = "Ready to Start"; break;
                case 0x30000: stateStr = "Starting"; break;
                case 0x40000: stateStr = "Stopping"; break;
                case 0x50000: stateStr = "On"; break;
                case 0xFFFF0000: stateStr = "Disabled"; break;
            }
            return `${desc}: ${stateStr}`;
        }
        case STATE_FAILURE: {
            const val = rawValue;
            let flStr = "Unknown";
            switch (val) {
                case 0x00000: flStr = "Okay"; break;
                case 0x10000: flStr = "Warning"; break;
                case 0x20000: flStr = "Simple Failure"; break;
                case 0x30000: flStr = "Empty"; break;
                case 0x40000: flStr = "Critical Failure"; break;
                case 0xFFFF0000: flStr = "Disabled"; break;
            }
            return `${desc}: ${flStr}`;
        }
        case STATE_BATTERY: {
            const val = rawValue;
            let bsStr = "Unknown";
            switch (val) {
                case 0x10000: bsStr = "Idle"; break;
                case 0x20000: bsStr = "Time left"; break;
                case 0x30000: bsStr = "Charging time"; break;
                case 0x40000: bsStr = "Full"; break;
                case 0x50000: bsStr = "Discharge"; break;
                case 0xFFFF0000: bsStr = "Low battery"; break;
                default: bsStr = "Unknown";
            }
            return `${desc}: ${bsStr}`;
        }
        case STATE_TESTMODE: {
            const val = rawValue;
            let tmStr = `Unknown State 0x${val.toString(16)}`;
            switch (val) {
                case 0x00000: tmStr = "Normal"; break;
                case 0x20000: tmStr = "BIOS"; break;
                case 0x30000: tmStr = "Application"; break;
                case 0x40000: tmStr = "No Protection"; break;
            }
            return `${desc}: ${tmStr}`;
        }

        case MASKED_BITMAP: {
            return `${desc}: Raw 0x${rawValue.toString(16)}`;
        }

        default:
            return `${desc}: Raw=0x${rawValue.toString(16)}`;
    }
}

export { paramTable };
