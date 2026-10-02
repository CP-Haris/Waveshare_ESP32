export const VOLTAGE = 1;
export const CURRENT = 2;
export const TEMPERATURE = 3;
export const PROCENT = 4;
export const ERROR = 8;
export const POWER = 9;
export const STATE_OPERATION = 16;
export const STATE_FAILURE = 17;
export const STATE_BATTERY = 18;
export const STATE_TESTMODE = 23;
export const MAC = 24;
export const PASSKEY = 25;
export const VERSION_LONG = 12;
export const VERSION_SHORT = 13;
export const DATE = 14;
export const TIME_HHMMSS = 7;
export const KWH = 20;
export const OHM = 19;
export const MASKED_BITMAP = 21;
export const TIME_HEAD = 5;
export const STRING = 15;
export const SERIAL = 11;

interface ParamInfo {
    prefix: number;
    desc: string;
}

const paramTable: { [key: string]: ParamInfo } = {
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
    "0:200": { prefix: STATE_OPERATION, desc: "ACIN_OPSTATE" },
    "0:204": { prefix: STATE_FAILURE, desc: "ACIN_FLSTATE" },
    "0:104": { prefix: POWER, desc: "ACIN_WATT" },
    "0:102": { prefix: VOLTAGE, desc: "ACIN_VOLT" },
    "0:103": { prefix: CURRENT, desc: "ACIN_AMP" },
    "0:208": { prefix: STATE_OPERATION, desc: "SOLAR_OPSTATE" },
    "0:209": { prefix: STATE_FAILURE, desc: "SOLAR_FLSTATE" },
    "0:78": { prefix: CURRENT, desc: "SOLAR_AMP" },
    "0:79": { prefix: POWER, desc: "SOLAR_WATT" },
    "0:110": { prefix: POWER, desc: "DCIN_WATT" },
    "0:157": { prefix: MASKED_BITMAP, desc: "WAKEUPFLAGS" },
    "0:203": { prefix: STATE_OPERATION, desc: "DCOUT_OPSTATE" },
    "240:245": { prefix: POWER, desc: "UNKNOWN_PARAM_240_245" },
    "0:119": { prefix: PROCENT, desc: "BATSTATUS_SOC" },
    "0:127": { prefix: POWER, desc: "BATSTATUS_WATT" },
    "0:120": { prefix: TIME_HEAD, desc: "BATSTATUS_REMTIME" },
    "0:225": { prefix: TIME_HHMMSS, desc: "DCOUT_AUTO_TIMEGLOBAL" },
    "0:226": { prefix: TIME_HHMMSS, desc: "ACOUT_AUTO_TIMEGLOBAL" },
    "1:1": { prefix: TIME_HHMMSS, desc: "FUNC_JUMPSTART" },
    "7:0": { prefix: STRING, desc: "UNKNOWN_PARAM_7_0" },
    "0:112": { prefix: CURRENT, desc: "DCOUT_AMP" },
    "0:113": { prefix: POWER, desc: "DCOUT_WATT" },
    "0:109": { prefix: CURRENT, desc: "DCIN_AMP" },
    "0:210": { prefix: STATE_BATTERY, desc: "BATSTATUS" },
    "0:207": { prefix: STATE_FAILURE, desc: "DCOUT_FLSTATE" },
    "0:202": { prefix: STATE_OPERATION, desc: "DCIN_OPSTATE" },
    "0:206": { prefix: STATE_FAILURE, desc: "DCIN_FLSTATE" },
    "0:212": { prefix: STATE_TESTMODE, desc: "SYS_TESTMODE" },
};

export function getParamInfo(block: number, id: number): ParamInfo | null {
    const key = `${block}:${id}`;
    return paramTable[key] || null;
}
