// Error severity levels
export enum ErrorLevel {
    WARNING = 'W',  // Unit can still operate to some point
    CRITICAL = 'C', // Function is blocked
    ERROR = 'E'     // Additional level found in some errors like Battery Empty
}

// How the failure is shown on phone
export enum ErrorAppearance {
    AUTO = 'A',    // Automatically follows failure code buffer
    STATIC = 'S',  // Static until manually cleared
    DISABLED = 'D' // Not shown
}

// Error categories
export enum ErrorCategory {
    BASIC_SYSTEM = 'Basic System',
    BMS_DC = 'BMS & DC Input & DC Output',
    POWER_SUPPLY = 'Power Supply',
    DC_DC_CONVERTER = 'DC/DC Converter',
    POWER_BOARD = 'Power Board',
    INVERTER = 'Inverter',
    CHARGER = 'Charger',
    SYSTEM_LOCK = 'System Lock'
}

// Interface for error definitions
export interface ErrorDefinition {
    code: number;
    name: string;
    description: string;
    category: ErrorCategory;
    level: ErrorLevel;
    appearance: ErrorAppearance;
    affects: {
        inverter?: boolean;
        charger?: boolean;
        dcInput?: boolean;
        dcOutput?: boolean;
        solar?: boolean;
    };
}

// Interface for localized error definition
export interface LocalizedErrorDefinition extends Omit<ErrorDefinition, 'name' | 'description'> {
    name: string;
    description: string;
}

// Main error definitions map
export const ErrorDefinitions: { [key: number]: ErrorDefinition } = {
    // Basic System (0-49)
    1: {
        code: 1,
        name: 'EEPROM CRC failure',
        description: 'Data in EEPROM is corrupted, checksum is not correct.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    2: {
        code: 2,
        name: 'EEPROM Offline',
        description: 'EEPROM is not available on the communication',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    4: {
        code: 4,
        name: 'Internal Under Temperature Warning',
        description: 'Electronics temperature is soon too low',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.WARNING,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    5: {
        code: 5,
        name: 'Internal Under Temperature Failure',
        description: 'Electronics temperature is too low',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.STATIC,
        affects: {
            inverter: true,
            charger: true
        }
    },
    6: {
        code: 6,
        name: 'Internal Over Temperature Warning',
        description: 'Electronics temperature is soon too high',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.WARNING,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    7: {
        code: 7,
        name: 'Internal Over Temperature Failure',
        description: 'Electronics temperature is too high',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.STATIC,
        affects: {
            inverter: true,
            charger: true
        }
    },
    8: {
        code: 8,
        name: 'Intern Temperature Sensor failure',
        description: 'Temperature sensor is out of value, short circuit, or open loop. (Power Board)',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.STATIC,
        affects: {
            inverter: true,
            charger: true
        }
    },
    9: {
        code: 9,
        name: 'Cell Temperature Sensor failure',
        description: 'Temperature sensor is out of value, short circuit, or open loop. (Cells)',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.STATIC,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    11: {
        code: 11,
        name: 'IO 1 (remote) Overload',
        description: 'IO 1 is overloaded or short circuit.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {}
    },
    12: {
        code: 12,
        name: 'IO 2 (data) Overload',
        description: 'IO 2 is overloaded or short circuit.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {}
    },
    13: {
        code: 13,
        name: 'IO 3 (front data) Overload',
        description: 'IO 3 is overloaded or short circuit.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {}
    },
    14: {
        code: 14,
        name: 'IO Terminal Overload',
        description: 'IO Terminal is overloaded or short circuit.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {}
    },
    20: {
        code: 20,
        name: 'IGBT H-Bridge failure',
        description: 'Driver for IGBT for H-Bridge is generating failure',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    22: {
        code: 22,
        name: 'IGBT CHG failure',
        description: 'Driver for IGBT for CHG is generating failure. (Power Board)',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    23: {
        code: 23,
        name: 'IGBT H-Bridge failure Chr',
        description: 'Driver for IGBT for H-Bridge is generating failure during 230vac Charge',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    30: {
        code: 30,
        name: 'Calibration of Cell Voltage',
        description: 'Calibration of Cell Voltage are not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    31: {
        code: 31,
        name: 'Calibration of DC Current',
        description: 'Calibration of DC Currents are not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    32: {
        code: 32,
        name: 'Calibration of Battery Voltage',
        description: 'Calibration of Battery Voltage is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    33: {
        code: 33,
        name: 'Calibration of AC Input Voltage',
        description: 'Calibration of AC Input Voltage is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    34: {
        code: 34,
        name: 'Calibration of AC Output Voltage',
        description: 'Calibration of AC Output Voltage is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    35: {
        code: 35,
        name: 'Calibration of AC Input Current',
        description: 'Calibration of AC Input Current is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    36: {
        code: 36,
        name: 'Calibration of AC Output Current',
        description: 'Calibration of AC Output Current is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    37: {
        code: 37,
        name: 'Calibration of DC Input Voltage',
        description: 'Calibration of DC Input Voltage is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcInput: true,
            dcOutput: true
        }
    },
    38: {
        code: 38,
        name: 'Calibration of DC Output Voltage',
        description: 'Calibration of DC Output Voltage is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcInput: true,
            dcOutput: true
        }
    },
    39: {
        code: 39,
        name: 'Calibration of DC Output Current',
        description: 'Calibration of DC Output Current is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcInput: true,
            dcOutput: true
        }
    },
    40: {
        code: 40,
        name: 'Calibration of Solar Current',
        description: 'Calibration of Solar Current is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            solar: true
        }
    },
    41: {
        code: 41,
        name: 'Calibration of IO Terminal Voltage',
        description: 'Calibration of IO Terminal Voltage is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    42: {
        code: 42,
        name: 'Calibration of IN Terminal Voltage',
        description: 'Calibration of IN Terminal Voltage is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true
        }
    },
    43: {
        code: 43,
        name: 'Calibration of IO 1 Voltage',
        description: 'Calibration of IO 1 Voltage is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true
        }
    },
    44: {
        code: 44,
        name: 'Calibration of IO 2 Voltage',
        description: 'Calibration of IO 2 Voltage is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true
        }
    },
    45: {
        code: 45,
        name: 'Calibration of IO 3 Voltage',
        description: 'Calibration of IO 3l Voltage is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true
        }
    },
    46: {
        code: 46,
        name: 'Calibration of 12.8V Voltage',
        description: 'Calibration of 12.8V Voltage is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    47: {
        code: 47,
        name: 'Calibration of 12.8V Current',
        description: 'Calibration of 12.8V Current is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    48: {
        code: 48,
        name: 'Calibration of Fan Voltage',
        description: 'Calibration of Fan Voltage is not performed.',
        category: ErrorCategory.BASIC_SYSTEM,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    
    // BMS & DC Input & DC Output (50-99)
    50: {
        code: 50,
        name: 'Cell missing',
        description: 'One or more cells are missing (<0.7V or > 4.5V)',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.STATIC,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    51: {
        code: 51,
        name: 'Battery Empty',
        description: 'SOC Calculation is below 20% (Empty battery)',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.ERROR,
        appearance: ErrorAppearance.STATIC,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    52: {
        code: 52,
        name: 'Cell Voltage Low Warning',
        description: 'One or more cells are soon having too low voltage',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.WARNING,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    53: {
        code: 53,
        name: 'Cell Voltage Low failure',
        description: 'One or more cells are having too low voltage',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.ERROR,
        appearance: ErrorAppearance.STATIC,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    54: {
        code: 54,
        name: 'Cell Voltage High Warning',
        description: 'One or more cells are soon having too high voltage',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.WARNING,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    55: {
        code: 55,
        name: 'Cell Voltage High Failure',
        description: 'One or more cells are having too high voltage',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.STATIC,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    56: {
        code: 56,
        name: 'Cell Temperature Low Warning',
        description: 'One or more cells are having too low temperature',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.WARNING,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    57: {
        code: 57,
        name: 'Cell Temperature Low Failure',
        description: 'One or more cells are having too low temperature',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.STATIC,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    58: {
        code: 58,
        name: 'Cell Temperature High Warning',
        description: 'One or more cells are having too high temperature',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.WARNING,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    59: {
        code: 59,
        name: 'Cell Temperature High Failure',
        description: 'One or more cells are having too high temperature',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.STATIC,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    60: {
        code: 60,
        name: 'Battery Voltage Low',
        description: 'The Total battery voltage is too low for the electronics',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    61: {
        code: 61,
        name: 'Battery Discharge Current Too High',
        description: 'Battery is being discharged with too high current',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    62: {
        code: 62,
        name: 'Battery Charge Current Too High',
        description: 'Battery is being charged with too high current',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    70: {
        code: 70,
        name: 'Solar Over Voltage',
        description: 'Input voltage for solar is too high (>55V)',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            solar: true
        }
    },
    71: {
        code: 71,
        name: 'Solar Over Current',
        description: 'Solar output current is measured above 50A',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            solar: true
        }
    },
    72: {
        code: 72,
        name: 'Solar Broken fuse',
        description: 'Solar fuse on control board is defective',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            solar: true
        }
    },
    73: {
        code: 73,
        name: 'Solar Current Offset failure',
        description: 'Solar Current offset out of range',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            solar: true
        }
    },
    74: {
        code: 74,
        name: 'Solar Disconnect Failure',
        description: 'Solar unable to disconnect solar charge power',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            solar: true
        }
    },
    75: {
        code: 75,
        name: 'Solar Self Protect',
        description: 'Solar has self-protected, and service is required',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            solar: true
        }
    },
    88: {
        code: 88,
        name: 'DC Output Overload Warning',
        description: 'Overload timer for DC Output is running and overload failure can soon occur',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.WARNING,
        appearance: ErrorAppearance.DISABLED,
        affects: {
            dcOutput: true
        }
    },
    89: {
        code: 89,
        name: 'Jump Start failure',
        description: 'Jump Start has been stopped.',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.WARNING,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcOutput: true
        }
    },
    90: {
        code: 90,
        name: 'DC Input Voltage Too Low',
        description: 'The DC input voltage is too low for charging from engine.',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.STATIC,
        affects: {
            dcInput: true
        }
    },
    91: {
        code: 91,
        name: 'DC Input Voltage Too High',
        description: 'The DC input voltage is too high for charging from engine.',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.STATIC,
        affects: {
            dcInput: true
        }
    },
    92: {
        code: 92,
        name: 'DC Input Voltage Low',
        description: 'DC/DC Converter is not running caused by low voltage',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.WARNING,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcInput: true
        }
    },
    93: {
        code: 93,
        name: 'DC Input General Failure',
        description: 'DC/DC Converter is not performing as expected',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcInput: true
        }
    },
    94: {
        code: 94,
        name: 'DC Output Relay Connect failure',
        description: 'The DC output relay is unable to connect: Output voltage too low',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcOutput: true
        }
    },
    95: {
        code: 95,
        name: 'DC Output Relay Disconnect failure',
        description: 'The DC output relay is unable to disconnect: Output voltage and current still present.',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcOutput: true
        }
    },
    96: {
        code: 96,
        name: 'DC Output Charge Current Too High',
        description: 'DC Output charge current too high',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcOutput: true
        }
    },
    97: {
        code: 97,
        name: 'DC Output Discharge Current Too High',
        description: 'DC Output discharge current too high',
        category: ErrorCategory.BMS_DC,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcOutput: true
        }
    },

    // Power Supply (100-109)
    101: {
        code: 101,
        name: 'AC Current Offset',
        description: 'AC Current Offset value is too high',
        category: ErrorCategory.POWER_SUPPLY,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true
        }
    },
    102: {
        code: 102,
        name: 'DC Current Offset',
        description: 'DC Current Offset value is too high',
        category: ErrorCategory.POWER_SUPPLY,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    105: {
        code: 105,
        name: 'High voltage missing',
        description: '400V is missing, power supply not running or broken',
        category: ErrorCategory.POWER_SUPPLY,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },

    // DC/DC Converter (120-129)
    120: {
        code: 120,
        name: 'DC/DC Internal Critical failure',
        description: '',
        category: ErrorCategory.DC_DC_CONVERTER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.STATIC,
        affects: {
            dcOutput: true
        }
    },
    121: {
        code: 121,
        name: 'DC/DC Master Communication',
        description: '',
        category: ErrorCategory.DC_DC_CONVERTER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcOutput: true
        }
    },
    122: {
        code: 122,
        name: 'DC/DC Temperature Warning',
        description: '',
        category: ErrorCategory.DC_DC_CONVERTER,
        level: ErrorLevel.WARNING,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcOutput: true
        }
    },
    123: {
        code: 123,
        name: 'DC/DC Temperature Too High',
        description: '',
        category: ErrorCategory.DC_DC_CONVERTER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcOutput: true
        }
    },
    124: {
        code: 124,
        name: 'DC/DC Voltage 12VDC IN Too High',
        description: 'DC/DC Voltage A Too High',
        category: ErrorCategory.DC_DC_CONVERTER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcOutput: true
        }
    },
    125: {
        code: 125,
        name: 'DC/DC Voltage OUT Too High',
        description: 'DC/DC Voltage B Too High',
        category: ErrorCategory.DC_DC_CONVERTER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcOutput: true
        }
    },
    126: {
        code: 126,
        name: 'DC/DC Current 12VDC IN Too High',
        description: 'DC/DC Current A Too High',
        category: ErrorCategory.DC_DC_CONVERTER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcOutput: true
        }
    },
    127: {
        code: 127,
        name: 'DC/DC Current OUT Too High',
        description: 'DC/DC Current B Too High',
        category: ErrorCategory.DC_DC_CONVERTER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            dcOutput: true
        }
    },

    // Power Board (130-149)
    130: {
        code: 130,
        name: 'PSU Control Voltage too low',
        description: '',
        category: ErrorCategory.POWER_BOARD,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    131: {
        code: 131,
        name: 'PSU LVPS failure',
        description: '',
        category: ErrorCategory.POWER_BOARD,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    132: {
        code: 132,
        name: 'PSU IGBT Temp Sensor failure',
        description: '',
        category: ErrorCategory.POWER_BOARD,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    133: {
        code: 133,
        name: 'PSU Mosfet Temp Sensor failure',
        description: '',
        category: ErrorCategory.POWER_BOARD,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    135: {
        code: 135,
        name: 'PSU Clamp Discharge',
        description: '',
        category: ErrorCategory.POWER_BOARD,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    136: {
        code: 136,
        name: 'PSU Clamp Charge failure',
        description: '',
        category: ErrorCategory.POWER_BOARD,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    137: {
        code: 137,
        name: 'PSU Trafo Ratio',
        description: '',
        category: ErrorCategory.POWER_BOARD,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    138: {
        code: 138,
        name: 'PSU 400V Feedback',
        description: '',
        category: ErrorCategory.POWER_BOARD,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    139: {
        code: 139,
        name: 'PSU 400V Too high',
        description: '',
        category: ErrorCategory.POWER_BOARD,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    140: {
        code: 140,
        name: 'PSU Communication Timeout',
        description: 'Communication Timeout by PSU',
        category: ErrorCategory.POWER_BOARD,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    141: {
        code: 141,
        name: 'PSU Communication Timeout',
        description: 'Communication Timeout by main micro',
        category: ErrorCategory.POWER_BOARD,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    142: {
        code: 142,
        name: 'PSU General Failure',
        description: 'PSU not performing as expected',
        category: ErrorCategory.POWER_BOARD,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },

    // Inverter (150-199)
    150: {
        code: 150,
        name: 'Output Overload Watt',
        description: '230vac output is overloaded with too many watt during Inverter mode',
        category: ErrorCategory.INVERTER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true
        }
    },
    151: {
        code: 151,
        name: 'Output Overload MCSIA',
        description: '230vac output is overloaded with peak currents too long time during Inverter mode',
        category: ErrorCategory.INVERTER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true
        }
    },
    152: {
        code: 152,
        name: 'AC Output Over Current',
        description: 'AC current is measured too high in peak',
        category: ErrorCategory.INVERTER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true
        }
    },
    153: {
        code: 153,
        name: 'PE/N No Operation',
        description: 'PE/N Relay has no operation',
        category: ErrorCategory.INVERTER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    154: {
        code: 154,
        name: 'PE/N Relay welded',
        description: 'PE/N Relay not back in normal position',
        category: ErrorCategory.INVERTER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    155: {
        code: 155,
        name: 'AC Output Overload Warning',
        description: 'Overload timer for AC output is running and overload failure can soon occur',
        category: ErrorCategory.INVERTER,
        level: ErrorLevel.WARNING,
        appearance: ErrorAppearance.DISABLED,
        affects: {
            inverter: true
        }
    },
    156: {
        code: 156,
        name: 'AC Output General Failure',
        description: 'AC Output is not performing as expected',
        category: ErrorCategory.INVERTER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true
        }
    },

    // Charger (200-219)
    200: {
        code: 200,
        name: 'Charger Over AC Current',
        description: 'AC Input Current Too high',
        category: ErrorCategory.CHARGER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            charger: true
        }
    },
    202: {
        code: 202,
        name: 'High Voltage Too High',
        description: 'Internal High Voltage is more than 550VDC',
        category: ErrorCategory.CHARGER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            charger: true
        }
    },
    203: {
        code: 203,
        name: 'Output Overload Watt',
        description: '230vac output is overloaded with too many watt during Charge mode',
        category: ErrorCategory.CHARGER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    204: {
        code: 204,
        name: 'Transfer Relay No Operation',
        description: 'Transfer relay is still in normal position and has no function',
        category: ErrorCategory.CHARGER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    205: {
        code: 205,
        name: 'Transfer Relay is welded',
        description: 'Transfer relay is not back in normal position',
        category: ErrorCategory.CHARGER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true
        }
    },
    206: {
        code: 206,
        name: '230VAC Mains too low',
        description: '230VAC Mains is above 90vac but below 207VAC',
        category: ErrorCategory.CHARGER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.STATIC,
        affects: {
            charger: true
        }
    },
    207: {
        code: 207,
        name: '230VAC Mains too high',
        description: '230VAC Mains is above 253VAC',
        category: ErrorCategory.CHARGER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.STATIC,
        affects: {
            charger: true
        }
    },
    208: {
        code: 208,
        name: 'AC Input General Failure',
        description: 'AC Input is not performing as expected',
        category: ErrorCategory.CHARGER,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            charger: true
        }
    },

    // System Lock (240-247)
    240: {
        code: 240,
        name: 'Cell Voltage too Low',
        description: 'System Lock due to Cell Voltage too low. Battery verification and unlock sequence required',
        category: ErrorCategory.SYSTEM_LOCK,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    241: {
        code: 241,
        name: 'Cell Voltage too High',
        description: 'System Lock due to Cell Voltage too high. Battery verification and unlock sequence required',
        category: ErrorCategory.SYSTEM_LOCK,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    242: {
        code: 242,
        name: 'Battery Voltage too Low',
        description: 'System Lock due to Battery Voltage too low. Battery verification and unlock sequence required',
        category: ErrorCategory.SYSTEM_LOCK,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    243: {
        code: 243,
        name: 'Battery Voltage too High',
        description: 'System Lock due to Battery Voltage too high. Battery verification and unlock sequence required',
        category: ErrorCategory.SYSTEM_LOCK,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    244: {
        code: 244,
        name: 'Cell Temperature too Low',
        description: 'System Lock due to Cell Temperature too low. Battery verification and unlock sequence required',
        category: ErrorCategory.SYSTEM_LOCK,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    245: {
        code: 245,
        name: 'Cell Temperature too High',
        description: 'System Lock due to Cell Temperature too high. Battery verification and unlock sequence required',
        category: ErrorCategory.SYSTEM_LOCK,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    246: {
        code: 246,
        name: 'Charge Current too high',
        description: 'System Lock due to Charge Current too high. Battery verification and unlock sequence required',
        category: ErrorCategory.SYSTEM_LOCK,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    },
    247: {
        code: 247,
        name: 'Discharge Current too High',
        description: 'System Lock due to Discharge Current too high. Battery verification and unlock sequence required',
        category: ErrorCategory.SYSTEM_LOCK,
        level: ErrorLevel.CRITICAL,
        appearance: ErrorAppearance.AUTO,
        affects: {
            inverter: true,
            charger: true,
            dcInput: true,
            dcOutput: true,
            solar: true
        }
    }
};

// Helper function to get error definition by code
export function getErrorDefinition(code: number): ErrorDefinition | undefined {
    return ErrorDefinitions[code];
}

// Helper function to check if a specific error code is active in the error buffer
export function isErrorActive(errorBuffer: Uint8Array, errorCode: number): boolean {
    // Check if the error code exists as a byte value in the buffer
    for (let i = 0; i < errorBuffer.length; i++) {
        if (errorBuffer[i] === errorCode) {
            return true;
        }
    }
    return false;
}

// Function to get all active errors from an error buffer
export function getActiveErrors(errorBuffer: Uint8Array): ErrorDefinition[] {
    const activeErrors: ErrorDefinition[] = [];
    
    // Each byte in the buffer represents a direct error code (if non-zero)
    for (let i = 0; i < errorBuffer.length; i++) {
        const errorCode = errorBuffer[i];
        if (errorCode !== 0) {
            const errorDef = getErrorDefinition(errorCode);
            if (errorDef) {
                activeErrors.push(errorDef);
            } else {
                console.warn(`Unknown error code: ${errorCode} (0x${errorCode.toString(16).padStart(2, '0')})`);
            }
        }
    }
    
    return activeErrors;
}

// Function to format error for display
export function formatError(error: ErrorDefinition): string {
    return `[${error.code}] ${error.name}: ${error.description} (${error.level})`;
}

// Function to get error severity level
export function getErrorSeverity(errors: ErrorDefinition[]): ErrorLevel {
    if (errors.some(e => e.level === ErrorLevel.CRITICAL)) return ErrorLevel.CRITICAL;
    if (errors.some(e => e.level === ErrorLevel.ERROR)) return ErrorLevel.ERROR;
    if (errors.some(e => e.level === ErrorLevel.WARNING)) return ErrorLevel.WARNING;
    return ErrorLevel.WARNING; // Default case
}

// Helper function to get localized error definition
export function getLocalizedErrorDefinition(
    code: number, 
    t: (key: string, options?: any) => string
): LocalizedErrorDefinition | undefined {
    const errorDef = getErrorDefinition(code);
    if (!errorDef) return undefined;

    // Try to get localized name and description
    const localizedName = t(`deviceErrors.${code}.name`, { defaultValue: errorDef.name });
    const localizedDescription = t(`deviceErrors.${code}.description`, { defaultValue: errorDef.description });

    return {
        ...errorDef,
        name: localizedName,
        description: localizedDescription
    };
}

// Function to get all active localized errors from an error buffer
export function getActiveLocalizedErrors(
    errorBuffer: Uint8Array, 
    t: (key: string, options?: any) => string
): LocalizedErrorDefinition[] {
    const activeErrors: LocalizedErrorDefinition[] = [];
    
    // Each byte in the buffer represents a direct error code (if non-zero)
    for (let i = 0; i < errorBuffer.length; i++) {
        const errorCode = errorBuffer[i];
        if (errorCode !== 0) {
            const localizedErrorDef = getLocalizedErrorDefinition(errorCode, t);
            if (localizedErrorDef) {
                activeErrors.push(localizedErrorDef);
            } else {
                console.warn(`Unknown error code: ${errorCode} (0x${errorCode.toString(16).padStart(2, '0')})`);
            }
        }
    }
    
    return activeErrors;
}

// Function to format localized error for display
export function formatLocalizedError(error: LocalizedErrorDefinition): string {
    return `[${error.code}] ${error.name}: ${error.description} (${error.level})`;
} 