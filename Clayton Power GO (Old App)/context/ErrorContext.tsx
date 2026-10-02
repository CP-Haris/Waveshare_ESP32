import React, { createContext, useContext, useState, useCallback, useEffect, useRef } from 'react';
import { getActiveErrors, isErrorActive, ErrorDefinition } from '../ble/services/ErrorDefinitions';
import CommService from '../ble/services/CommService';
import { useDeviceConnection } from './DeviceConnectionContext';

interface ErrorContextType {
  activeErrors: ErrorDefinition[];
  ignoredErrors: ErrorDefinition[];
  isClearing: boolean;
  processErrorBuffer: (errorBuffer: Uint8Array) => void;
  getNextError: () => ErrorDefinition | undefined;
  handleClearErrors: () => Promise<boolean>;
  setIgnoredErrors: React.Dispatch<React.SetStateAction<ErrorDefinition[]>>;
}

const ErrorContext = createContext<ErrorContextType | null>(null);

export function ErrorProvider({ children, deviceId = "" }: { children: React.ReactNode; deviceId?: string }) {
  const { commService } = useDeviceConnection();
  const [activeErrors, setActiveErrors] = useState<ErrorDefinition[]>([]);
  const [ignoredErrors, setIgnoredErrors] = useState<ErrorDefinition[]>([]);
  const [isClearing, setIsClearing] = useState(false);
  
  // Track last logged state to prevent spam
  const lastLoggedErrorBuffer = useRef<string>('');
  const lastLoggedErrors = useRef<string>('');

  // Reset error state when deviceId changes
  useEffect(() => {
    console.log('Device changed, resetting error state for device:', deviceId);
    setActiveErrors([]);
    setIgnoredErrors([]);
    setIsClearing(false);
    lastLoggedErrorBuffer.current = '';
    lastLoggedErrors.current = '';
  }, [deviceId]);

  // Process incoming error buffer
  const processErrorBuffer = useCallback((errorBuffer: Uint8Array) => {
    if (isClearing) return;
    
    const bufferString = Array.from(errorBuffer).map(b => `0x${b.toString(16).padStart(2, '0')}`).join(' ');
    const errors = getActiveErrors(errorBuffer);
    const errorsString = errors.map(e => `Code ${e.code}: ${e.name}`).join(', ');
    
    // Only log if buffer or errors changed
    if (bufferString !== lastLoggedErrorBuffer.current || errorsString !== lastLoggedErrors.current) {
      console.log('Error buffer changed:', bufferString);
      if (errors.length > 0) {
        console.log('Active errors:', errorsString);
      } else {
        console.log('No active errors');
      }
      
      lastLoggedErrorBuffer.current = bufferString;
      lastLoggedErrors.current = errorsString;
    }
    
    setActiveErrors(errors);
    
    // Check if any ignored errors are no longer active and remove them
    setIgnoredErrors(prevIgnored => {
      const stillActiveIgnored = prevIgnored.filter(ignoredError => 
        isErrorActive(errorBuffer, ignoredError.code)
      );
      
      // Log if any ignored errors were removed
      if (stillActiveIgnored.length !== prevIgnored.length) {
        const removedErrors = prevIgnored.filter(ignoredError => 
          !isErrorActive(errorBuffer, ignoredError.code)
        );
        console.log('Removed ignored errors that are no longer active:', removedErrors.map(e => e.name));
      }
      
      return stillActiveIgnored;
    });
  }, [isClearing]);

  // Find the first non-ignored error
  const getNextError = useCallback(() => {
    return activeErrors.find(error => !ignoredErrors.some(ignored => ignored.code === error.code));
  }, [activeErrors, ignoredErrors]);

  // Handle clearing all errors
  const handleClearErrors = useCallback(async () => {
    if (!commService) return false;
    
    try {
      setIsClearing(true);
      await commService.clearErrors();
      setActiveErrors([]);
      setIgnoredErrors([]);
      // Wait for 1 second before resuming error checks
      await new Promise(resolve => setTimeout(resolve, 1000));
      return true;
    } catch (error) {
      console.error('Failed to clear errors:', error);
      return false;
    } finally {
      setIsClearing(false);
    }
  }, [commService]);

  return (
    <ErrorContext.Provider value={{
      activeErrors,
      ignoredErrors,
      isClearing,
      processErrorBuffer,
      getNextError,
      handleClearErrors,
      setIgnoredErrors,
    }}>
      {children}
    </ErrorContext.Provider>
  );
}

export function useError() {
  const context = useContext(ErrorContext);
  if (!context) {
    throw new Error('useError must be used within an ErrorProvider');
  }
  return context;
}

// Wrapper component that provides a default error context
export function ErrorProviderWrapper({ children, deviceId }: { children: React.ReactNode; deviceId: string }) {
  return (
    <ErrorProvider deviceId={deviceId}>
      {children}
    </ErrorProvider>
  );
} 