export interface ClaytonPowerDevice {
    deviceId: string;
    serialNumber: string;
    name: string;
    stateOfCharge: string;
    isVisible: boolean;
    isConnected?: boolean;
    lastSeenTimestamp?: number;
    lastSoCUpdate?: number;
  }
  