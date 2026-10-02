import { useState, useCallback, useRef, useEffect } from 'react';
import { Alert } from 'react-native';
import { ErrorDefinition, ErrorLevel, getActiveErrors } from '../ble/services/ErrorDefinitions';
import CommService from '../ble/services/CommService';

export const useErrorManager = (deviceId: string, commService: CommService) => {
  const [activeErrors, setActiveErrors] = useState<ErrorDefinition[]>([]);
  const [ignoredErrors, setIgnoredErrors] = useState<ErrorDefinition[]>([]);
  const [isClearing, setIsClearing] = useState(false);
  const currentAlertRef = useRef<boolean>(false);
  const lastCheckTimeRef = useRef<number>(0);
  const checkIntervalRef = useRef<NodeJS.Timeout>();

  // Process incoming error buffer
  const processErrorBuffer = useCallback((errorBuffer: Uint8Array) => {
    if (isClearing) return;
    
    const errors = getActiveErrors(errorBuffer);
    setActiveErrors(errors);
  }, [isClearing]);

  // Find the first non-ignored error
  const getNextError = useCallback(() => {
    return activeErrors.find(error => !ignoredErrors.some(ignored => ignored.code === error.code));
  }, [activeErrors, ignoredErrors]);

  // Handle clearing all errors
  const handleClearErrors = useCallback(async () => {
    try {
      setIsClearing(true);
      await commService.clearErrors();
      setActiveErrors([]);
      setIgnoredErrors([]);
      // Wait for 2 seconds before resuming error checks
      await new Promise(resolve => setTimeout(resolve, 2000));
      return true;
    } catch (error) {
      console.error('Failed to clear errors:', error);
      return false;
    } finally {
      setIsClearing(false);
    }
  }, [commService]);

  // Show error alert
  const showErrorAlert = useCallback((error: ErrorDefinition) => {
    if (currentAlertRef.current || !error) return;

    const title = error.level === ErrorLevel.WARNING ? "Warning" : "Error";
    const message = `${error.name}\n\n${error.description}`;

    currentAlertRef.current = true;

    Alert.alert(
      title,
      message,
      [
        {
          text: 'Ignore',
          onPress: () => {
            setIgnoredErrors(prev => [...prev, error]);
            currentAlertRef.current = false;
          },
        },
        {
          text: 'Clear',
          onPress: async () => {
            setIsClearing(true);
            currentAlertRef.current = false;
            try {
              await commService.clearErrors();
              // Wait for 2 seconds before resuming error checks
              await new Promise(resolve => setTimeout(resolve, 2000));
            } catch (error) {
              console.error('Failed to clear errors:', error);
            } finally {
              setIsClearing(false);
            }
          },
        },
      ],
      {
        cancelable: false,
        onDismiss: () => {
          currentAlertRef.current = false;
        },
      }
    );
  }, [commService]);

  // Check for errors periodically
  useEffect(() => {
    const checkForErrors = () => {
      const now = Date.now();
      if (now - lastCheckTimeRef.current < 1000) return; // Debounce 1 second
      
      lastCheckTimeRef.current = now;
      if (isClearing || currentAlertRef.current) return;

      const nextError = getNextError();
      if (nextError) {
        showErrorAlert(nextError);
      }
    };

    checkIntervalRef.current = setInterval(checkForErrors, 1000);

    return () => {
      if (checkIntervalRef.current) {
        clearInterval(checkIntervalRef.current);
      }
    };
  }, [getNextError, showErrorAlert, isClearing]);

  // Clean up on unmount
  useEffect(() => {
    return () => {
      if (checkIntervalRef.current) {
        clearInterval(checkIntervalRef.current);
      }
      currentAlertRef.current = false;
    };
  }, []);

  return {
    activeErrors,
    ignoredErrors,
    processErrorBuffer,
    getNextError,
    showErrorAlert,
    handleClearErrors,
    isClearing,
  };
}; 