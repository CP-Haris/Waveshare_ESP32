import React, { createContext, useContext } from 'react';
import { useDeviceList } from '../hooks/useDeviceList';

const DeviceListContext = createContext<ReturnType<typeof useDeviceList> | null>(null);

export function DeviceListProvider({ children }: { children: React.ReactNode }) {
  // This single instance holds the real state
  const deviceList = useDeviceList();
  return (
    <DeviceListContext.Provider value={deviceList}>
      {children}
    </DeviceListContext.Provider>
  );
}

export function useDeviceListContext() {
  const context = useContext(DeviceListContext);
  if (!context) {
    throw new Error("useDeviceListContext must be used within DeviceListProvider");
  }
  return context;
}
