import React from "react";
import { NavigationContainer } from "@react-navigation/native";
import {
  DrawerContentScrollView,
  DrawerItem,
  createDrawerNavigator,
} from "@react-navigation/drawer";
import DeviceListScreen from "./screens/DeviceListScreen";
import SettingsScreen from "./screens/SettingsScreen";
import LanguageSelectionScreen from "./screens/LanguageSelectionScreen";
import ErrorCodesListScreen from "./screens/ErrorCodesListScreen";
import DeviceInfoScreen from "./screens/DeviceInfoScreen";
import ErrorListScreen from "./screens/ErrorListScreen";
import { Text, View, Image, Platform, TouchableOpacity } from "react-native";
import {
  DeviceListProvider,
  useDeviceListContext,
} from "./context/DeviceListContext";
import { DeviceConnectionProvider } from "./context/DeviceConnectionContext";
import { ErrorProviderWrapper } from "./context/ErrorContext";
import { ClaytonPowerDevice } from "./models/ClaytonPowerDevice";
import { useTranslation } from 'react-i18next';
import { ToastProvider } from 'react-native-toast-notifications';
import './i18n/config';
import { registerLowBatteryBackgroundTask, initializeNotifee } from './services/LowBatteryBackgroundService';
import { Logger } from './components/Logger';

const WEB_FONT_STACK =
  'system-ui, "Segoe UI", Roboto, Helvetica, Arial, sans-serif, "Apple Color Emoji", "Segoe UI Emoji", "Segoe UI Symbol"';

const CPTheme = {
  dark: true,
  colors: {
    primary: "rgb(0, 117, 193)",
    background: "rgb(32, 32, 32)",
    card: "rgb(35, 35, 35)",
    text: "rgb(255, 255, 255)",
    border: "rgb(35, 35, 35)",
    notification: "rgb(255, 69, 58)",
  },
  fonts: Platform.select({
    web: {
      regular: {
        fontFamily: WEB_FONT_STACK,
        fontWeight: "400",
      },
      medium: {
        fontFamily: WEB_FONT_STACK,
        fontWeight: "500",
      },
      bold: {
        fontFamily: WEB_FONT_STACK,
        fontWeight: "600",
      },
      heavy: {
        fontFamily: WEB_FONT_STACK,
        fontWeight: "700",
      },
    },
    ios: {
      regular: {
        fontFamily: "System",
        fontWeight: "400",
      },
      medium: {
        fontFamily: "System",
        fontWeight: "500",
      },
      bold: {
        fontFamily: "System",
        fontWeight: "600",
      },
      heavy: {
        fontFamily: "System",
        fontWeight: "700",
      },
    },
    default: {
      regular: {
        fontFamily: "sans-serif",
        fontWeight: "normal",
      },
      medium: {
        fontFamily: "sans-serif-medium",
        fontWeight: "normal",
      },
      bold: {
        fontFamily: "sans-serif",
        fontWeight: "600",
      },
      heavy: {
        fontFamily: "sans-serif",
        fontWeight: "700",
      },
    },
  }),
};

const Drawer = createDrawerNavigator();

function DrawerNavigator() {
  const { devices, connectToDevice } = useDeviceListContext();
  const [activeDeviceId, setActiveDeviceId] = React.useState<string | null>(null);
  const { t } = useTranslation();

  // Create memoized callbacks to ensure consistent hook order
  const handleDevicePress = React.useCallback(async (device: ClaytonPowerDevice, navigation: any) => {
    try {
      if (!device.isConnected) {
        await connectToDevice(device);
      }
      if (device.isConnected) {
        setActiveDeviceId(device.deviceId);
        navigation.navigate("DeviceInfo", {
          deviceId: device.deviceId,
          deviceSerial: device.serialNumber,
        });
      }
    } catch {
      console.warn("Connection failed; not navigating.");
    }
  }, [connectToDevice]);

  return (
    <DeviceConnectionProvider deviceId={activeDeviceId || ''}>
      <ErrorProviderWrapper deviceId={activeDeviceId || ''}>
        <Drawer.Navigator
          initialRouteName="Device List"
          screenOptions={{
            headerTintColor: "#ffffff",
            drawerActiveTintColor: "white",
            headerStyle: {
              backgroundColor: "#202020",
            },
          }}
          drawerContent={(props) => (
            <DrawerContentScrollView {...props}>
              <View
                style={{
                  alignItems: "flex-start",
                  marginBottom: 20,
                  marginTop: 20,
                  marginLeft: 10,
                }}
              >
                <Image
                  source={require("./assets/images/logo.png")}
                  style={{ height: 30, width: 150 }}
                />
              </View>

              <DrawerItem
                label={t('navigation.deviceList')}
                onPress={() => {
                  props.navigation.navigate("Device List");
                }}
              />
              
              <DrawerItem
                label={t('navigation.settings')}
                onPress={() => {
                  props.navigation.navigate("Settings");
                }}
              />

              {devices.length > 0 && (
                <Text
                  style={{
                    marginBottom: 10,
                    marginTop: 40,
                    marginLeft: 20,
                    fontSize: 10,
                    fontWeight: "bold",
                    color: "#606060",
                  }}
                >
                  {t('deviceList.availableDevices')}
                </Text>
              )}

              {devices.map((device: ClaytonPowerDevice) => (
                <TouchableOpacity
                  key={device.deviceId}
                  onPress={() => handleDevicePress(device, props.navigation)}              
                  style={{
                    flexDirection: "row",
                    gap: 5,
                    paddingTop: 10,
                    paddingBottom: 10,
                    paddingLeft: 20,
                    paddingRight: 20,
                    flex: 1,
                    width: 200,
                  }}
                >
                  <Image
                    source={require("./assets/images/LPS2-Transparent.png")}
                    style={{ height: 50, width: 50 }}
                  />
                  <View style={{ flexDirection: "column", gap: 5 }}>
                    <Text style={{ color: "#ffffff", fontSize: 14 }}>
                      {device.serialNumber}
                    </Text>
                    <Text style={{ color: "#606060", fontSize: 10 }}>
                      BT: {device.name}
                    </Text>
                  </View>
                </TouchableOpacity>
              ))}
            </DrawerContentScrollView>
          )}
        >
          <Drawer.Screen
            name="DeviceInfo"
            component={DeviceInfoScreen}
            options={{ title: t('navigation.deviceDetails') }}
          />
          <Drawer.Screen
            name="ErrorList"
            component={ErrorListScreen}
            options={{ title: t('navigation.ignoredErrors') }}
          />
          <Drawer.Screen
            name="Device List"
            component={DeviceListScreen}
            options={{ title: t('navigation.deviceList') }}
          />
          <Drawer.Screen
            name="Settings"
            component={SettingsScreen}
            options={{ title: t('navigation.settings') }}
          />
          <Drawer.Screen
            name="LanguageSelection"
            component={LanguageSelectionScreen}
            options={{ 
              title: t('settings.selectLanguage', 'Select Language'),
              drawerItemStyle: { display: 'none' }, // Hide from drawer menu
              unmountOnBlur: true, // Ensure clean unmount
            }}
          />
          <Drawer.Screen
            name="ErrorCodesList"
            component={ErrorCodesListScreen}
            options={{ 
              title: 'Error Codes',
              drawerItemStyle: { display: 'none' }, // Hide from drawer menu
              unmountOnBlur: true, // Ensure clean unmount
            }}
          />
        </Drawer.Navigator>
      </ErrorProviderWrapper>
    </DeviceConnectionProvider>
  );
}

export default function App() {
  // Track if initialization has been done
  const initializedRef = React.useRef(false);

  // Defer heavy initialization to avoid blocking app startup
  React.useEffect(() => {
    const initTimer = setTimeout(() => {
      initializeBackgroundMonitoring();
    }, 100); // Small delay to let UI render first

    return () => clearTimeout(initTimer);
  }, []);

  // Initialize background battery monitoring (deferred)
  const initializeBackgroundMonitoring = React.useCallback(async () => {
    if (initializedRef.current) return;

    try {
      // Initialize Notifee (notification permissions and channels)
      await initializeNotifee();
      
      // Register background task for periodic battery checks
      const taskRegistered = await registerLowBatteryBackgroundTask();
      
      if (taskRegistered) {
        Logger.info('[App] ✅ Background battery monitoring initialized');
      } else {
        Logger.warn('[App] ⚠️ Background task not scheduled (may need native rebuild)');
      }
      
      initializedRef.current = true;
    } catch (error) {
      Logger.error('[App] Failed to initialize background monitoring:', error);
    }
  }, []);

  return (
    <DeviceListProvider>
      <ToastProvider>
        <NavigationContainer theme={CPTheme}>
          <DrawerNavigator />
        </NavigationContainer>
      </ToastProvider>
    </DeviceListProvider>
  );
}
