import React, { createContext, useContext, useRef, useState, useMemo } from 'react';
import CommService from '../ble/services/CommService';
import { ErrorDefinition } from '../ble/services/ErrorDefinitions';

interface DeviceConnectionContextType {
  commService: CommService | null;
  isConnected: React.MutableRefObject<boolean>;
  ignoredErrors: Set<number>;
  ignoredErrorsList: ErrorDefinition[];
  addIgnoredError: (error: ErrorDefinition) => void;
  clearIgnoredErrors: () => void;
}

const DeviceConnectionContext = createContext<DeviceConnectionContextType | null>(null);

export function DeviceConnectionProvider({ children, deviceId }: { children: React.ReactNode; deviceId: string }) {
  const commService = useMemo(() => new CommService(deviceId), [deviceId]);
  const isConnected = useRef(false);
  const [ignoredErrors, setIgnoredErrors] = useState<Set<number>>(new Set());
  const [ignoredErrorsList, setIgnoredErrorsList] = useState<ErrorDefinition[]>([]);

  const addIgnoredError = (error: ErrorDefinition) => {
    setIgnoredErrors(prev => {
      const newSet = new Set(prev);
      newSet.add(error.code);
      return newSet;
    });
    setIgnoredErrorsList(prev => [...prev, error]);
  };

  const clearIgnoredErrors = () => {
    setIgnoredErrors(new Set());
    setIgnoredErrorsList([]);
  };

  return (
    <DeviceConnectionContext.Provider value={{ 
      commService, 
      isConnected, 
      ignoredErrors, 
      ignoredErrorsList,
      addIgnoredError,
      clearIgnoredErrors
    }}>
      {children}
    </DeviceConnectionContext.Provider>
  );
}

export function useDeviceConnection() {
  const context = useContext(DeviceConnectionContext);
  if (!context) {
    throw new Error('useDeviceConnection must be used within a DeviceConnectionProvider');
  }
  return context;
} 