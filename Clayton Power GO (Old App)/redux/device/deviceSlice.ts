import { createSlice, PayloadAction } from '@reduxjs/toolkit';

interface Characteristic {
  id: string;
  name: string;
  value: string;
  image: string;
}

export interface Device {
  id: string;
  name: string;
  serialNumber: string;
  stateOfCharge: string;
  serviceUUID: string;
  characteristics: Characteristic[];
}

interface DeviceState {
  discoveredDevices: Device[];
  connectedDevices: Device[];
}

const initialState: DeviceState = {
  discoveredDevices: [],
  connectedDevices: [],
};

const deviceSlice = createSlice({
  name: 'device',
  initialState,
  reducers: {
    addDiscoveredDevice(state, action: PayloadAction<Device>) {
      const index = state.discoveredDevices.findIndex(
        (device) => device.id === action.payload.id
      );
      if (index === -1) {
        state.discoveredDevices.push(action.payload);
      } else {
        state.discoveredDevices[index] = {
          ...state.discoveredDevices[index],
          ...action.payload,
        };
      }
    },    
    addConnectedDevice(state, action: PayloadAction<Device>) {
      const existingDevice = state.connectedDevices.find(device => device.id === action.payload.id);
      if (!existingDevice) {
        state.connectedDevices.push(action.payload);
      }
    },
    disconnectDevice(state, action: PayloadAction<string>) {
      state.connectedDevices = state.connectedDevices.filter(device => device.id !== action.payload);
    },
    clearDevices(state) {
      state.discoveredDevices = [];
    },
  },
});

export const { addDiscoveredDevice, addConnectedDevice, disconnectDevice, clearDevices } = deviceSlice.actions;
export default deviceSlice.reducer;
