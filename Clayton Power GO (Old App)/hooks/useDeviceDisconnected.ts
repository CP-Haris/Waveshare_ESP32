import { useEffect, useCallback, useRef } from 'react';
import { useNavigation } from '@react-navigation/native';
import { bleService } from '../ble/BLEService';
import { Toast } from 'react-native-toast-notifications';

const useDeviceDisconnected = (deviceId: string) => {
  const navigation = useNavigation();
  const intentionalDisconnectRef = useRef(false);

  const handleDisconnect = useCallback(async () => {
    try {
      // Set flag to indicate this is an intentional disconnect
      intentionalDisconnectRef.current = true;
      
      // Disconnect the device
      await bleService.disconnectFromDevice(deviceId);
      
      // Show toast notification
      Toast.show("Device disconnected", {
        type: "normal",
        placement: "bottom",
        duration: 3000,
      });
      
      // Navigate to Device List screen
      navigation.navigate('Device List' as never);
    } catch (error) {
      console.error("Error during disconnect:", error);
      // Still show toast and navigate to Device List even if there's an error
      Toast.show("Device disconnected", {
        type: "normal",
        placement: "bottom",
        duration: 3000,
      });
      navigation.navigate('Device List' as never);
    }
  }, [deviceId, navigation]);

  // Listen for unexpected disconnects with verification
  useEffect(() => {
    const sub = bleService.onDeviceDisconnected(deviceId, async () => {
      // Check if this was an intentional disconnect
      if (intentionalDisconnectRef.current) {
        console.log('Intentional disconnect detected, skipping automatic handling');
        // Reset the flag for future disconnects
        intentionalDisconnectRef.current = false;
        return;
      }
      
      // Improved disconnection detection: Add verification with grace period
      console.log('Unexpected disconnection event detected, verifying...');
      
      // Wait 3 seconds and verify connection is actually lost (increased from 2s)
      await new Promise(resolve => setTimeout(resolve, 3000));
      
      try {
        const isStillConnected = await bleService.isDeviceConnected(deviceId);
        
        if (!isStillConnected) {
          console.log('Unexpected disconnection verified after 3s grace period, navigating back to device list');
          
          // Show toast notification for unexpected disconnection
          Toast.show("Device connection lost", {
            type: "warning",
            placement: "bottom",
            duration: 4000,
          });
          
          navigation.navigate('Device List' as never);
        } else {
          console.log('False disconnection event - device is still connected after verification');
        }
      } catch (error) {
        console.error('Error verifying connection state:', error);
        // If we can't verify, assume disconnection happened
        console.log('Verification failed, assuming disconnection occurred');
        Toast.show("Device connection lost", {
          type: "warning",
          placement: "bottom",
          duration: 4000,
        });
        navigation.navigate('Device List' as never);
      }
    });
    return () => sub?.remove();
  }, [deviceId, navigation]);

  // Reset flag when device changes
  useEffect(() => {
    intentionalDisconnectRef.current = false;
  }, [deviceId]);

  return handleDisconnect;
};

export default useDeviceDisconnected;
