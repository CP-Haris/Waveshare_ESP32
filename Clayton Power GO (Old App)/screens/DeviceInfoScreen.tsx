import React, { useEffect, useState, useMemo, useCallback, useRef } from "react";
import {
  View,
  Text,
  SafeAreaView,
  Image,
  ScrollView,
  StyleSheet,
  Dimensions,
  TouchableOpacity,
  Alert,
  ActivityIndicator,
  Modal,
  Animated,
  StatusBar,
} from "react-native";
import { useSafeAreaInsets } from 'react-native-safe-area-context';
import { useTranslation } from 'react-i18next';
import DisplayIconButton from "../components/DisplayIconButton";
import DisplayTextButton, { OperationalState } from "../components/DisplayTextButton";
import DisconnectButton from "../components/DisconnectButton";
import PrimaryButton from "../components/PrimaryButton";
import SecondaryButton from "../components/SecondaryButton";
import useDeviceDisconnected from "../hooks/useDeviceDisconnected";
import useDeviceInfo, {
  scaleFont,
} from "../hooks/useDeviceInfo";
import { useError } from '../context/ErrorContext';
import { bleService } from "../ble/BLEService";
import { useNavigation, useRoute, RouteProp, useIsFocused } from '@react-navigation/native';
import { NativeStackNavigationProp } from '@react-navigation/native-stack';
import CommService from '../ble/services/CommService';
import { ErrorLevel, ErrorDefinition, LocalizedErrorDefinition, getLocalizedErrorDefinition } from '../ble/services/ErrorDefinitions';
import { Logger } from '../components/Logger';
import { useDeviceListContext } from '../context/DeviceListContext';

type RootStackParamList = {
  DeviceInfo: { deviceId: string; deviceSerial: string };
  ErrorList: { deviceId: string };
  'Device List': undefined;
};

type DeviceInfoScreenRouteProp = RouteProp<RootStackParamList, 'DeviceInfo'>;
type DeviceInfoScreenNavigationProp = NativeStackNavigationProp<RootStackParamList, 'DeviceInfo'>;

const { width } = Dimensions.get("window");
const guidelineBaseWidth = 375;

// Function to map raw operational state strings to OperationalState enum
const mapToOperationalState = (stateString: string): OperationalState => {
  const mappedState = (() => {
    switch (stateString) {
      case 'Disabled':
        return 'OS_DISABLED';
      case 'Off':
        return 'OS_OFF';
      case 'Wakeup':
        return 'OS_WAKEUP';
      case 'Ready to Start':
        return 'OS_READY_TO_START';
      case 'Starting':
        return 'OS_STARTING';
      case 'Stopping':
        return 'OS_STOPPING';
      case 'On':
        return 'OS_ON';
      default:
        Logger.warn('Unknown operational state:', stateString, 'defaulting to OS_OFF');
        return 'OS_OFF'; // Default fallback
    }
  })();
  
  // Log all state changes
  Logger.debug('Operational state transition:', stateString, '->', mappedState);
  
  return mappedState;
};

export default function DeviceInfoScreen() {
  const navigation = useNavigation<DeviceInfoScreenNavigationProp>();
  const route = useRoute<DeviceInfoScreenRouteProp>();
  const { deviceId, deviceSerial } = route.params;
  const { t } = useTranslation();
  const insets = useSafeAreaInsets();
  const { ignoredErrors, processErrorBuffer, getNextError, setIgnoredErrors, activeErrors } = useError();
  const [currentAlert, setCurrentAlert] = useState<boolean>(false);
  const [currentError, setCurrentError] = useState<LocalizedErrorDefinition | null>(null);
  const [isClearing, setIsClearing] = useState<boolean>(false);
  const [showDebugInfo, setShowDebugInfo] = useState<boolean>(false);
  const slideAnim = useRef(new Animated.Value(300)).current; // Start off-screen
  
  // SoC fix: Get access to device list context
  const { updateDeviceSoC } = useDeviceListContext();
  
  // Function to dismiss current alert programmatically
  const dismissCurrentAlert = useCallback(() => {
    // Animate out
    Animated.timing(slideAnim, {
      toValue: 300,
      duration: 300,
      useNativeDriver: true,
    }).start(() => {
      setCurrentAlert(false);
      setCurrentError(null);
    });
  }, [slideAnim]);
  
  // Function to show alert with animation
  const showAlert = useCallback((error: LocalizedErrorDefinition) => {
    setCurrentError(error);
    setCurrentAlert(true);
    // Animate in
    Animated.timing(slideAnim, {
      toValue: 0,
      duration: 300,
      useNativeDriver: true,
    }).start();
  }, [slideAnim]);
  
  // Ref to store the current error checking function
  const checkErrorsRef = useRef<(() => void) | null>(null);
  
  const handleErrorBuffer = useCallback((errorBuffer: Uint8Array) => {
    processErrorBuffer(errorBuffer);
    
    // Check if error buffer is all zeros (no active errors)
    const hasActiveErrors = errorBuffer.some(byte => byte !== 0);
    if (!hasActiveErrors && currentAlert) {
      dismissCurrentAlert();
    }
    
    // Trigger immediate error check after processing buffer
    setTimeout(() => {
      if (checkErrorsRef.current) {
        checkErrorsRef.current();
      }
    }, 100);
  }, [processErrorBuffer, dismissCurrentAlert]);
  
  const { values, failures, debugInfo, handleToggle12V, handleToggle24V, commService, setUpdateDeviceListSoC } = useDeviceInfo(deviceId, handleErrorBuffer, activeErrors);

  // SoC fix: Set up the sync function when component mounts
  useEffect(() => {
    if (setUpdateDeviceListSoC && updateDeviceSoC) {
      setUpdateDeviceListSoC(updateDeviceSoC);
    }
  }, [setUpdateDeviceListSoC, updateDeviceSoC]);

  // Memoize operational states to prevent excessive mapToOperationalState calls
  const dcOutOperationalState = useMemo(() => 
    mapToOperationalState(values.dcOutOpState), 
    [values.dcOutOpState]
  );
  
  const acOutOperationalState = useMemo(() => 
    mapToOperationalState(values.acOutOpState === "On" || values.acinOpState === "On" ? "On" : values.acOutOpState), 
    [values.acOutOpState, values.acinOpState]
  );

  // Memoize operational states for input sources (DC IN, Grid, Solar)
  const dcinOperationalState = useMemo(() => 
    mapToOperationalState(values.dcinOpState), 
    [values.dcinOpState]
  );
  
  const acinOperationalState = useMemo(() => 
    mapToOperationalState(values.acinOpState), 
    [values.acinOpState]
  );
  
  const solarOperationalState = useMemo(() => 
    mapToOperationalState(values.solarOpState), 
    [values.solarOpState]
  );

  const handleDisconnect = useDeviceDisconnected(deviceId);
  const isFocused = useIsFocused();

  // Handle device disconnect (manual or automatic)
  useEffect(() => {
    // Only run this effect once on mount
    let didRun = false;
    if (!didRun) {
      // Patch handleDisconnect to also clear error state
      const originalHandleDisconnect = handleDisconnect;
      const wrappedHandleDisconnect = async () => {
        dismissCurrentAlert(); // Dismiss any current alert
        await originalHandleDisconnect();
      };
      // Replace handleDisconnect with wrapped version for this render
      // (if you use handleDisconnect as a callback elsewhere, use wrappedHandleDisconnect instead)
    }
    // No cleanup needed
  }, [handleDisconnect, dismissCurrentAlert]);

  // Check for errors periodically
  useEffect(() => {
    let cancelled = false;
    let errorCheckCount = 0;

    const checkErrors = () => {
      if (!isFocused || cancelled) return;
      const nextError = getNextError();
      
      errorCheckCount++;
      
      // If no error found and alert is currently showing, dismiss it
      if (!nextError && currentAlert) {
        Logger.info('No errors found, dismissing current alert');
        dismissCurrentAlert();
        return;
      }
      
      if (nextError && !currentAlert) {
        // Get localized version of the error
        const localizedError = getLocalizedErrorDefinition(nextError.code, t);
        if (localizedError) {
          Logger.info('Showing alert for error:', localizedError.name);
          showAlert(localizedError);
        }
      } else {
        setTimeout(checkErrors, 1000);
      }
    };

    // Store the checkErrors function in ref so it can be called immediately
    checkErrorsRef.current = checkErrors;

    if (isFocused) {
      setTimeout(checkErrors, 1000);
    }

    return () => {
      cancelled = true;
      checkErrorsRef.current = null;
    };
  }, [getNextError, currentAlert, commService, isFocused, setIgnoredErrors, activeErrors, ignoredErrors, dismissCurrentAlert, showAlert]);

  // Optionally update the title if you have a valid serial (or other info)
  useEffect(() => {
    if (deviceSerial) {
      navigation.setOptions({ title: deviceSerial });
    }
  }, [deviceSerial, navigation]);

  // Disconnect handler
  const handleDisconnectAndClear = async () => {
    // Stop listening for messages and reset values
    commService.stopListening();
    
    // Call the disconnect handler
    await handleDisconnect();
  };

  // Dismiss current alert when deviceId changes
  useEffect(() => {
    dismissCurrentAlert(); // Dismiss any current alert when device changes
  }, [deviceId, dismissCurrentAlert]);

  useEffect(() => {
    const sub = bleService.onDeviceDisconnected(deviceId, () => {
      dismissCurrentAlert(); // Dismiss alert on disconnection
    });
    return () => sub?.remove();
  }, [deviceId, dismissCurrentAlert]);

  return (
    <View style={styles.safeArea}>
      <StatusBar barStyle="light-content" backgroundColor="#202020" translucent={false} />
      <ScrollView 
        style={[styles.container, { paddingTop: insets.top }]} 
        scrollEnabled={!isClearing} 
        pointerEvents={isClearing ? 'none' : 'auto'}
        contentContainerStyle={styles.scrollContent}
        showsVerticalScrollIndicator={true}
      >
        {/* Top Section */}
        <View style={styles.topSection}>
          <DisplayIconButton
            title="DC IN"
            icon="alternator"
            value={values.dcinWatt !== "-" ? values.dcinWatt : "-"}
            active={values.dcinOpState === "On"}
            operationalState={dcinOperationalState}
            hasFailure={failures.dcInput.hasFailure}
            failureCount={failures.dcInput.failureCount}
            superCharge={true}
          />
          <DisplayIconButton
            title={t('deviceInfo.grid')}
            icon="grid"
            value={values.acinWatt !== "-" ? values.acinWatt : "-"}
            active={values.acinOpState === "On"}
            operationalState={acinOperationalState}
            hasFailure={failures.acInput.hasFailure}
            failureCount={failures.acInput.failureCount}
          />
          <DisplayIconButton
            title={t('deviceInfo.solar')}
            icon="solar"
            value={values.solarWatt !== "-" ? values.solarWatt : "-"}
            active={values.solarOpState === "On"}
            operationalState={solarOperationalState}
            hasFailure={failures.solar.hasFailure}
            failureCount={failures.solar.failureCount}
          />
        </View>

        {/* Middle Image */}
        <View style={styles.middleImageContainer}>
          <Image
            source={require("../assets/images/LPS2.jpg")}
            style={styles.middleImage}
          />
        </View>

        {/* Bottom Section */}
        <View style={styles.bottomSection}>
          <DisplayTextButton
            title="12 V"
            value={values.dcOutWatt !== "-" ? values.dcOutWatt : "-"}
            operationalState={dcOutOperationalState}
            hasFailure={failures.dcOutput.hasFailure}
            failureCount={failures.dcOutput.failureCount}
            onPress={handleToggle12V}
          />
          <View style={styles.batteryContainer}>
            <Text
              style={styles.percentageLeftLabel}
              numberOfLines={1}
              adjustsFontSizeToFit
            >
              {values.batterySOC === "100.0" ? "100%" : values.batterySOC}
            </Text>
            {values.batteryStatus && values.batteryStatus !== "Unknown" && (
              <Text style={styles.smallLabel}>{values.batteryStatus}</Text>
            )}
            {values.batteryStatus !== "Full" && values.batteryStatus !== "Low battery" && (
              <Text style={styles.timeRemainingLabel}>
                {values.timeRemaining}
              </Text>
            )}
          </View>
          <DisplayTextButton
            title="230 V"
            value={values.acOutWatt !== "-" ? values.acOutWatt : "-"}
            operationalState={acOutOperationalState}
            onPress={handleToggle24V}
            hasFailure={failures.acOutput.hasFailure}
            failureCount={failures.acOutput.failureCount}
            isCharging={values.acinOpState === "On"}
            acOutState={values.acOutOpState}
          />
        </View>

        {/* Debug Information Block */}
        {showDebugInfo && (
          <View style={styles.debugSection}>
            <Text style={styles.debugSectionTitle}>Debug Information</Text>
            
            <View style={styles.debugRow}>
              <Text style={styles.debugLabel}>Operational States:</Text>
            </View>
            <View style={styles.debugGrid}>
              <Text style={styles.debugItem}>DC IN: {debugInfo.operational.dcInput}</Text>
              <Text style={styles.debugItem}>AC IN: {debugInfo.operational.acInput}</Text>
              <Text style={styles.debugItem}>Solar: {debugInfo.operational.solar}</Text>
              <Text style={styles.debugItem}>DC OUT: {debugInfo.operational.dcOutput}</Text>
              <Text style={styles.debugItem}>AC OUT: {debugInfo.operational.acOutput}</Text>
            </View>
            
            <View style={styles.debugRow}>
              <Text style={styles.debugLabel}>Power Values:</Text>
            </View>
            <View style={styles.debugGrid}>
              <Text style={styles.debugItem}>DC IN: {debugInfo.power.dcInput}</Text>
              <Text style={styles.debugItem}>AC IN: {debugInfo.power.acInput}</Text>
              <Text style={styles.debugItem}>Solar: {debugInfo.power.solar}</Text>
              <Text style={styles.debugItem}>DC OUT: {debugInfo.power.dcOutput}</Text>
              <Text style={styles.debugItem}>AC OUT: {debugInfo.power.acOutput}</Text>
            </View>
            
            <View style={styles.debugRow}>
              <Text style={styles.debugLabel}>Failure States:</Text>
            </View>
            <View style={styles.debugGrid}>
              <Text style={[styles.debugItem, failures.dcInput.hasFailure && styles.debugItemError]}>
                DC IN: {debugInfo.failures.dcInput}
              </Text>
              <Text style={[styles.debugItem, failures.acInput.hasFailure && styles.debugItemError]}>
                AC IN: {debugInfo.failures.acInput}
              </Text>
              <Text style={[styles.debugItem, failures.solar.hasFailure && styles.debugItemError]}>
                Solar: {debugInfo.failures.solar}
              </Text>
              <Text style={[styles.debugItem, failures.dcOutput.hasFailure && styles.debugItemError]}>
                DC OUT: {debugInfo.failures.dcOutput}
              </Text>
              <Text style={[styles.debugItem, failures.acOutput.hasFailure && styles.debugItemError]}>
                AC OUT: {debugInfo.failures.acOutput}
              </Text>
            </View>
            
            <View style={styles.debugRow}>
              <Text style={styles.debugLabel}>Battery:</Text>
            </View>
            <View style={styles.debugGrid}>
              <Text style={styles.debugItem}>SOC: {debugInfo.battery.soc}</Text>
              <Text style={styles.debugItem}>Status: {debugInfo.battery.status}</Text>
              <Text style={styles.debugItem}>Time: {debugInfo.battery.timeRemaining}</Text>
            </View>
          </View>
        )}

        {/* Error Button */}
        {ignoredErrors.length > 0 && (
          <TouchableOpacity
            style={styles.actionButton}
            onPress={() => navigation.navigate('ErrorList', { deviceId })}
            disabled={isClearing}
          >
            <Text style={styles.actionButtonText}>{t('deviceInfo.errors', { count: ignoredErrors.length })}</Text>
          </TouchableOpacity>
        )}

        {/* Disconnect Button */}
        <TouchableOpacity
          style={styles.actionButton}
          onPress={handleDisconnectAndClear}
          disabled={isClearing}
        >
          <Text style={styles.actionButtonText}>{t('deviceList.disconnect')}</Text>
        </TouchableOpacity>
      </ScrollView>
      
      {/* Bottom Error Popup */}
      {currentAlert && currentError && (
        <Modal
          visible={true}
          transparent={true}
          animationType="none"
          onRequestClose={() => {}} // Prevent dismissing with back button
        >
          <View style={styles.bottomModalOverlay}>
            <Animated.View 
              style={[
                styles.bottomPopupContainer,
                {
                  transform: [{ translateY: slideAnim }]
                }
              ]}
            >
              <SafeAreaView style={styles.popupSafeArea}>
                <View style={styles.popupContent}>
                  <View style={styles.popupTopSpacer} />
                  <Text style={styles.popupTitle}>
                    {currentError?.level === ErrorLevel.WARNING ? `⚠️ ${t('deviceInfo.warning')}` : `❌ ${t('common.error')}`}
                  </Text>
                  
                  <Text style={styles.popupErrorName}>
                    {currentError?.name}
                  </Text>
                  
                  <Text style={styles.popupErrorCode}>
                    {t('deviceInfo.errorCode')}: {currentError?.code}
                  </Text>
                  
                  <Text style={styles.popupDescription}>
                    {currentError?.description}
                  </Text>
                  
                  <View style={styles.popupButtons}>
                    <TouchableOpacity
                      style={[styles.popupPrimaryButton, isClearing && styles.popupPrimaryButtonDisabled]}
                      onPress={async () => {
                        setIsClearing(true);
                        try {
                          await commService.clearErrors();
                          await new Promise(resolve => setTimeout(resolve, 1000));
                          setIgnoredErrors([]);
                        } catch (error) {
                          console.error('Failed to clear errors:', error);
                        } finally {
                          setIsClearing(false);
                          dismissCurrentAlert();
                          setTimeout(() => {
                            const checkErrors = () => {
                              if (!isFocused) return;
                              const nextError = getNextError();
                              if (nextError && !currentAlert) {
                                showAlert(nextError);
                              }
                            };
                            checkErrors();
                          }, 1000);
                        }
                      }}
                      disabled={isClearing}
                    >
                      <Text style={styles.popupPrimaryButtonText}>{t('deviceInfo.clearAll')}</Text>
                    </TouchableOpacity>
                    
                    <TouchableOpacity
                      style={styles.popupSecondaryButton}
                      onPress={() => {
                        if (currentError) {
                          // Convert back to ErrorDefinition for the ignored errors list
                          const originalError: ErrorDefinition = {
                            code: currentError.code,
                            name: currentError.name,
                            description: currentError.description,
                            category: currentError.category,
                            level: currentError.level,
                            appearance: currentError.appearance,
                            affects: currentError.affects
                          };
                          setIgnoredErrors((prev: ErrorDefinition[]) => [...prev, originalError]);
                        }
                        dismissCurrentAlert();
                        setTimeout(() => {
                          const checkErrors = () => {
                            if (!isFocused) return;
                            const nextError = getNextError();
                            if (nextError && !currentAlert) {
                              showAlert(nextError);
                            }
                          };
                          checkErrors();
                        }, 1000);
                      }}
                    >
                      <Text style={styles.popupSecondaryButtonText}>{t('deviceInfo.ignore')}</Text>
                    </TouchableOpacity>
                  </View>
                </View>
              </SafeAreaView>
            </Animated.View>
          </View>
        </Modal>
      )}
      
      {isClearing && (
        <View style={styles.loadingOverlay}>
          <ActivityIndicator size="large" color="#fff" />
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
    backgroundColor: "#202020",
  },
  container: {
    padding: 16,
    backgroundColor: "#202020",
  },
  topSection: {
    padding: 10,
    flexDirection: "row",
    justifyContent: "space-between",
  },
  middleImageContainer: {
    marginTop: -50,
    marginBottom: -80,
    zIndex: -1,
    padding: 10,
    alignItems: "center",
    justifyContent: "center",
  },
  middleImage: {
    height: 350,
    width: 350,
  },
  bottomSection: {
    padding: 10,
    flexDirection: "row",
    justifyContent: "space-between",
  },
  batteryContainer: {
    flex: 1,
    alignItems: "center",
  },
  smallLabel: {
    color: "#ffffff",
    marginRight: 10,
    fontSize: scaleFont(12),
  },
  percentageLeftLabel: {
    fontSize: scaleFont(32),
    fontWeight: "bold",
    color: "#ffffff",
    marginBottom: 16,
    marginTop: 30,
  },
  timeRemainingLabel: {
    fontSize: scaleFont(16),
    fontWeight: "bold",
    color: "#ffffff",
    marginBottom: 16,
  },
  actionButton: {
    backgroundColor: '#fff',
    padding: 16,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 16,
    marginBottom: 16,
    marginHorizontal: 8,
  },
  actionButtonText: {
    color: '#000',
    fontSize: 16,
    fontWeight: 'bold',
  },
  loadingOverlay: {
    ...StyleSheet.absoluteFillObject,
    backgroundColor: 'rgba(0,0,0,0.4)',
    justifyContent: 'center',
    alignItems: 'center',
    zIndex: 10,
  },
  bottomModalOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.5)',
    justifyContent: 'flex-end',
  },
  bottomPopupContainer: {
    width: '100%',
    backgroundColor: '#fff',
    borderTopLeftRadius: 20,
    borderTopRightRadius: 20,
    maxHeight: '80%',
    minHeight: 300,
  },
  bottomPopup: {
    backgroundColor: 'transparent',
    width: '100%',
    flex: 1,
  },
  popupHandle: {
    width: 40,
    height: 5,
    backgroundColor: '#ccc',
    borderRadius: 5,
    marginTop: 10,
    marginBottom: 10,
    alignSelf: 'center',
  },
  popupSafeArea: {
    paddingHorizontal: 20,
    paddingBottom: 20,
  },
  popupContent: {
    alignItems: 'center',
    paddingBottom: 20,
  },
  popupTopSpacer: {
    height: 20,
  },
  popupTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 15,
    color: '#333',
  },
  popupErrorName: {
    fontSize: 16,
    fontWeight: '600',
    marginBottom: 10,
    color: '#333',
    textAlign: 'center',
  },
  popupErrorCode: {
    fontSize: 12,
    marginBottom: 15,
    color: '#666',
    backgroundColor: '#f5f5f5',
    paddingHorizontal: 8,
    paddingVertical: 4,
    borderRadius: 4,
    fontFamily: 'monospace',
  },
  popupDescription: {
    fontSize: 14,
    marginBottom: 25,
    color: '#555',
    textAlign: 'center',
    lineHeight: 20,
  },
  popupButtons: {
    flexDirection: 'column',
    width: '100%',
    gap: 10,
    alignItems: 'center',
  },
  popupSecondaryButton: {
    paddingVertical: 16,
    paddingHorizontal: 20,
    borderRadius: 8,
    borderColor: '#000000',
    borderWidth: 1,
    backgroundColor: '#ffffff',
    justifyContent: 'center',
    alignItems: 'center',
    width: '80%',
  },
  popupSecondaryButtonText: {
    color: '#000000',
    fontSize: 16,
    fontWeight: '600',
  },
  popupPrimaryButton: {
    paddingVertical: 16,
    paddingHorizontal: 20,
    borderRadius: 8,
    backgroundColor: '#0075c1',
    justifyContent: 'center',
    alignItems: 'center',
    width: '80%',
    shadowColor: '#000',
    shadowOffset: {
      width: 0,
      height: 2,
    },
    shadowOpacity: 0.25,
    shadowRadius: 3.84,
    elevation: 5,
  },
  popupPrimaryButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: '600',
  },
  popupPrimaryButtonDisabled: {
    backgroundColor: '#ccc',
  },
  testSection: {
    padding: 10,
    marginTop: 20,
    backgroundColor: '#303030',
    borderRadius: 8,
    margin: 10,
  },
  testSectionTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 10,
    color: '#ffffff',
    textAlign: 'center',
  },
  testButtonRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 10,
    gap: 10,
  },
  debugSection: {
    padding: 16,
    marginTop: 20,
    marginBottom: 20,
    backgroundColor: '#303030',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#404040',
  },
  debugSectionTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 12,
    color: '#ffffff',
    textAlign: 'center',
  },
  debugRow: {
    marginTop: 8,
    marginBottom: 4,
  },
  debugLabel: {
    fontSize: 14,
    fontWeight: '600',
    color: '#cccccc',
    marginBottom: 4,
  },
  debugGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    marginBottom: 8,
  },
  debugItem: {
    fontSize: 12,
    color: '#ffffff',
    backgroundColor: '#404040',
    paddingHorizontal: 8,
    paddingVertical: 4,
    marginRight: 8,
    marginBottom: 4,
    borderRadius: 4,
    fontFamily: 'monospace',
    minWidth: '30%',
  },
  debugItemError: {
    backgroundColor: '#8B0000',
    color: '#ffcccc',
  },
  scrollContent: {
    paddingBottom: 100,
    flexGrow: 1,
  },
});
