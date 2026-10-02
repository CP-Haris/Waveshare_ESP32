import React, { useCallback, memo, useMemo, useState, useEffect } from 'react';
import {
  View,
  Text,
  FlatList,
  StyleSheet,
  SafeAreaView,
  StatusBar,
  Modal,
  ActivityIndicator,
  Image,
  TouchableOpacity,
  Platform,
} from 'react-native';
import { useSafeAreaInsets } from 'react-native-safe-area-context';
import { useTranslation } from 'react-i18next';
import PrimaryButton from '../components/PrimaryButton';
import { NavigationProp, useFocusEffect } from '@react-navigation/native';
import { Swipeable } from 'react-native-gesture-handler';
import { ClaytonPowerDevice } from '@/models/ClaytonPowerDevice';
import DestructiveButton from '../components/DestructiveButton';
import DisconnectButton from '../components/DisconnectButton';
import { useDeviceListContext } from '../context/DeviceListContext';
import { getFullVersionString } from '../constants/AppVersion';

/**
 * Get color for charge indicator based on number of filled segments
 */
const getChargeIndicatorColor = (filledSegments: number, totalSegments: number): string => {
  // Color based on number of filled segments
  if (filledSegments >= 5) return '#4CAF50';      // Green - full (5 segments)
  if (filledSegments >= 4) return '#8BC34A';      // Light Green - high (4 segments)
  if (filledSegments >= 3) return '#FFC107';      // Amber - medium (3 segments)
  if (filledSegments >= 2) return '#FF9800';      // Orange - low (2 segments)
  if (filledSegments >= 1) return '#F44336';      // Red - critical (1 segment)
  
  // Default gray for no data
  return '#E0E0E0';
};

const getChargeLevel = (stateOfCharge: string): number => {
  // Handle empty strings and undefined values
  if (!stateOfCharge || stateOfCharge === "" || stateOfCharge === "-") {
    return 0;
  }
  
  const soc = parseFloat(stateOfCharge);
  const level = isNaN(soc) ? 0 : Math.max(0, Math.min(100, soc));
  return level;
};

/**
 * Charge indicator component
 */
const ChargeIndicator = memo(function ChargeIndicator({
  stateOfCharge,
  isVisible,
  isConnected,
}: {
  stateOfCharge: string;
  isVisible: boolean;
  isConnected: boolean;
}) {
  const chargeLevel = getChargeLevel(stateOfCharge);
  const emptyColor = '#F0F0F0';
  
  // Create segments for a more detailed visual representation
  const segments = 5;
  const filledSegments = Math.ceil((chargeLevel / 100) * segments);
  const fillColor = getChargeIndicatorColor(filledSegments, segments);
  
  // Determine opacity based on data freshness
  const hasData = chargeLevel > 0;
  const dataOpacity = hasData ? (isConnected ? 1.0 : 0.8) : 0.3; // Live data: full opacity, cached: slightly dimmed, no data: very dim
  
  return (
    <View style={styles.chargeIndicatorContainer}>
      {Array.from({ length: segments }, (_, index) => {
        // Since flexDirection: 'column' and justifyContent: 'flex-end', 
        // index 0 is at the bottom, index 4 is at the top
        // We want to fill from top down, so reverse the logic
        const segmentFromTop = segments - 1 - index; // index 0 = top segment (4), index 4 = bottom segment (0)
        const isSegmentFilled = segmentFromTop < filledSegments;
        
        let segmentColor = emptyColor;
        let segmentOpacity = 0.3;
        
        if (isSegmentFilled && hasData) {
          segmentColor = fillColor;
          segmentOpacity = dataOpacity;
        }
        
        return (
          <View
            key={index}
            style={[
              styles.chargeSegment,
              {
                backgroundColor: segmentColor,
                opacity: segmentOpacity,
              },
            ]}
          />
        );
      })}
    </View>
  );
});

/** 
 * Memoized child component that holds the Swipeable ref.
 */
const SwipeableRow = memo(function SwipeableRow({
  item,
  onDisconnect,
  onRemove,
  onConnectOrView,
}: {
  item: ClaytonPowerDevice;
  onDisconnect: () => void;
  onRemove: () => void;
  onConnectOrView: () => void;
}) {
  const swipeableRef = React.useRef<Swipeable>(null);
  const { t } = useTranslation();

  const renderRightActions = useMemo(() => () => (
    <View style={styles.rightActionContainer}>
      <DisconnectButton
        title={t('deviceList.disconnect')}
        onPress={async () => {
          await onDisconnect();
          swipeableRef.current?.close();
        }}
        disabled={!item.isConnected}
      />
      <DestructiveButton
        title={t('deviceList.remove')}
        onConfirm={async () => {
          await onRemove();
          swipeableRef.current?.close();
        }}
      />
    </View>
  ), [onDisconnect, onRemove, item.isConnected]);

  const renderContent = useMemo(() => () => (
    <View style={styles.deviceCard}>
      <ChargeIndicator 
        stateOfCharge={item.stateOfCharge} 
        isVisible={item.isVisible} 
        isConnected={item.isConnected || false}
      />
      <View style={styles.deviceContent}>
        <Image
          source={require('../assets/images/LPS2-Transparent.png')}
          style={{ height: 50, width: 50 }}
          resizeMode="contain"
        />
        <View style={{ flex: 1, flexDirection: 'column', gap: 1 }}>
          <Text style={styles.deviceName} numberOfLines={1}>{item.serialNumber}</Text>
          {item.name ? (
            <Text style={styles.deviceId} numberOfLines={1}>
              BT: {item.name}
              {item.stateOfCharge && item.stateOfCharge !== '-' && item.stateOfCharge !== ''
                ? ` (${item.stateOfCharge}${item.stateOfCharge.includes('%') ? '' : '%'})`
                : ''}
            </Text>
          ) : null}
        </View>
        <PrimaryButton
          title={item.isConnected ? t('deviceList.view') : t('deviceList.connect')}
          onPress={onConnectOrView}
          disabled={!item.isVisible && !item.isConnected}
        />
      </View>
    </View>
  ), [item, onConnectOrView]);

  return (
    <Swipeable 
      ref={swipeableRef} 
      renderRightActions={renderRightActions}
      friction={1}
      rightThreshold={30}
      overshootRight={false}
      enableTrackpadTwoFingerGesture
      containerStyle={styles.swipeableContainer}
    >
      {renderContent()}
    </Swipeable>
  );
});

const generateTestDevices = (count: number): ClaytonPowerDevice[] => {
  return Array.from({ length: count }, (_, i) => ({
    deviceId: `TEST-${i + 1}`,
    serialNumber: `SN${String(i + 1).padStart(6, '0')}`,
    name: `Test Device ${i + 1}`,
    stateOfCharge: `${Math.floor(Math.random() * 100)}%`,
    isVisible: true,
    isConnected: false,
    lastSeenTimestamp: Date.now(),
  }));
};

export default function DeviceListScreen({
  navigation,
}: {
  navigation: NavigationProp<any>;
}) {
  const {
    devices,
    isScanning,
    isConnecting,
    startScan,
    connectToDevice,
    removeDevice,
    disconnectDevice,
    stopScan,
    setDevices,
    loadCachedDevices,
  } = useDeviceListContext();
  const { t } = useTranslation();
  const insets = useSafeAreaInsets();

  const [appVersion, setAppVersion] = useState<string | null>(null);

  // Get app version and build number
  useEffect(() => {
    const getAppInfo = async () => {
      try {
        const versionString = getFullVersionString();
        setAppVersion(versionString); // Will be null if no real version available
      } catch (error) {
        console.error('Error getting app info:', error);
        setAppVersion(null); // Hide version display on error
      }
    };
    
    getAppInfo();
  }, []);

  const handleAddTestDevices = useCallback(() => {
    const testDevices = generateTestDevices(15);
    setDevices(prev => [...prev, ...testDevices]);
  }, [setDevices]);

  useFocusEffect(
    useCallback(() => {
      loadCachedDevices(); // Reload cached devices when screen comes into focus
      startScan();
      return () => {
        stopScan();
      };
    }, [loadCachedDevices, startScan, stopScan])
  );

  const handleConnectOrView = useCallback((device: ClaytonPowerDevice) => {
    if (!device.isVisible && !device.isConnected) {
      console.warn("Device is not currently visible. Cannot connect.");
      return;
    }
    if (device.isConnected) {
      navigation.navigate("DeviceInfo", {
        deviceId: device.deviceId,
        deviceSerial: device.serialNumber,
      });
    } else {
      connectToDevice(device)
        .then(() => {
          if (device.isConnected) {
            navigation.navigate("DeviceInfo", {
              deviceId: device.deviceId,
              deviceSerial: device.serialNumber,
            });
          }
        })
        .catch(() => {
          console.warn(t('deviceList.connectionFailed'));
        });
    }
  }, [navigation, connectToDevice]);

  const handleDisconnect = useCallback((deviceId: string) => {
    disconnectDevice(deviceId);
  }, [disconnectDevice]);

  const handleRemove = useCallback((deviceId: string) => {
    removeDevice(deviceId);
  }, [removeDevice]);

  const renderItem = useCallback(({ item }: { item: ClaytonPowerDevice }) => {
    return (
      <SwipeableRow
        item={item}
        onDisconnect={() => handleDisconnect(item.deviceId)}
        onRemove={() => handleRemove(item.deviceId)}
        onConnectOrView={() => handleConnectOrView(item)}
      />
    );
  }, [handleDisconnect, handleRemove, handleConnectOrView]);

  const keyExtractor = useCallback((item: ClaytonPowerDevice) => item.deviceId, []);

  const getItemLayout = useCallback((data: any, index: number) => ({
    length: 70, // Approximate height of each item
    offset: 70 * index,
    index,
  }), []);

  // Filter out devices with 'Unknown' serialNumber
  const filteredDevices = useMemo(() => {
    return devices.filter(device => 
      device.serialNumber && 
      device.serialNumber !== 'Unknown' && 
      device.serialNumber !== '' && 
      device.serialNumber !== '-'
    );
  }, [devices]);

  return (
    <View style={styles.safeArea}>
      <StatusBar barStyle="light-content" backgroundColor="#202020" translucent={false} />
      <View style={[styles.container, { paddingTop: insets.top }]}>
        <View style={styles.headerContainer}>
          <Text style={styles.title}>{t('deviceList.availableDevices')}</Text>
        </View>

        {filteredDevices.length === 0 && !isConnecting && (
          <View style={styles.noDevicesContainer}>
            <Text style={styles.noDevices}>{t('deviceList.noDevicesFound')}</Text>
          </View>
        )}

        {filteredDevices.length > 0 && (
          <FlatList
            data={filteredDevices}
            keyExtractor={keyExtractor}
            renderItem={renderItem}
            getItemLayout={getItemLayout}
            removeClippedSubviews={true}
            maxToRenderPerBatch={5}
            windowSize={3}
            initialNumToRender={8}
            updateCellsBatchingPeriod={100}
            maintainVisibleContentPosition={{
              minIndexForVisible: 0,
              autoscrollToTopThreshold: 10,
            }}
            scrollEventThrottle={16}
            onEndReachedThreshold={0.5}
            contentContainerStyle={styles.listContent}
          />
        )}

        {/* App Version Display */}
        {appVersion && (
          <View style={styles.versionContainer}>
            <Text style={styles.versionText}>
              {appVersion}
            </Text>
          </View>
        )}
      </View>

      <Modal transparent={true} visible={isConnecting} onRequestClose={() => {}}>
        <View style={styles.modalOverlay}>
          <View style={styles.modalContent}>
            <ActivityIndicator size="large" color="#ffffff" />
            <Text style={styles.modalText}>{t('deviceInfo.connecting')}</Text>
          </View>
        </View>
      </Modal>
    </View>
  );
}

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
    backgroundColor: '#202020',
  },
  container: {
    flex: 1,
    padding: 16,
    backgroundColor: '#202020',
  },
  title: {
    fontSize: 24,
    fontWeight: '600',
    color: '#ffffff',
    marginBottom: 20,
  },
  noDevicesContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  noDevices: {
    fontSize: 16,
    fontWeight: '600',
    color: '#515151',
    marginBottom: 20,
  },
  deviceCard: {
    paddingTop: 10,
    paddingBottom: 10,
    paddingHorizontal: 10,
    paddingLeft: 15,
    borderBottomWidth: 1,
    borderBottomColor: '#262626',
    flexDirection: 'column',
    backgroundColor: '#202020',
    position: 'relative',
  },
  deviceName: {
    fontSize: 16,
    fontWeight: '600',
    color: '#ffffff',
    marginBottom: 4,
  },
  deviceId: {
    fontSize: 10,
    color: '#999999',
  },
  rightActionContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'flex-end',
    paddingRight: 10,
    marginBottom: 10,
    gap: 10,
  },
  modalOverlay: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: 'rgba(0, 0, 0, 0.5)',
  },
  modalContent: {
    backgroundColor: '#121212',
    padding: 20,
    borderRadius: 10,
    alignItems: 'center',
  },
  modalText: {
    marginTop: 10,
    fontSize: 18,
    color: '#ffffff',
  },
  headerContainer: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 20,
  },
  swipeableContainer: {
    backgroundColor: '#202020',
  },
  listContent: {
    paddingBottom: 20,
  },
  versionContainer: {
    padding: 10,
    backgroundColor: '#202020',
    alignItems: 'center',
  },
  versionText: {
    fontSize: 12,
    color: '#999999',
  },
  chargeIndicatorContainer: {
    height: 50,
    width: 4,
    position: 'absolute',
    left: 0,
    top: 10,
    borderRadius: 2,
    justifyContent: 'flex-end',
    flexDirection: 'column',
  },
  chargeSegment: {
    height: 8,
    width: 4,
    marginBottom: 2,
    borderRadius: 1,
  },
  deviceContent: {
    flexDirection: 'row',
    gap: 10,
  },
});
