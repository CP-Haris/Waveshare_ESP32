import React, { useState, useEffect } from 'react';
import { StatusBar } from 'expo-status-bar';
import { NavigationContainer, DefaultTheme } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { View, StyleSheet, Platform, AppState, Pressable } from 'react-native';
import { useFonts } from 'expo-font';

import DashboardScreen from './src/screens/DashboardScreen';
import SettingsScreen from './src/screens/SettingsScreen';
import ConnectScreen from './src/screens/ConnectScreen';
import FirmwareUpdateScreen from './src/screens/FirmwareUpdateScreen';
import CarbonIcon from './src/components/CarbonIcon';
import ErrorCenter from './src/components/ErrorCenter';
import { colors, fontAssets } from './src/utils/theme';
import deviceSession from './src/devices/deviceSession';
import { backgroundService } from './src/services/backgroundService';

const Tab = createBottomTabNavigator();

const DASHBOARD_POLL_MS = 2000;

const navTheme = {
  ...DefaultTheme,
  dark: true,
  colors: {
    ...DefaultTheme.colors,
    primary: colors.blue,
    background: colors.bg,
    card: colors.bg,
    text: colors.ink,
    border: colors.line,
  },
};

const TAB_ICONS = {
  Dashboard: 'dash',
  Settings: 'gear',
  Update: 'update',
  Connect: 'bt',
};

// Tab bar (spec §5): icons only. The selected tab is drawn like the selected
// row in the display's menu: the whole cell becomes a grey plate with a blue
// edge (here along the top) and a blue icon. The button is replaced outright
// because the default one squeezes the icon into a ~31 dp box.
function TabButton({ routeName, alert, onPress, onLongPress, 'aria-selected': selected }) {
  return (
    <Pressable
      onPress={onPress}
      onLongPress={onLongPress}
      accessibilityRole="tab"
      accessibilityState={{ selected: !!selected }}
      accessibilityLabel={routeName}
      style={[styles.tabCell, selected && styles.tabCellOn]}
    >
      {selected && <View style={styles.tabEdge} />}
      <View>
        <CarbonIcon name={TAB_ICONS[routeName] || 'dash'} size={26} color={selected ? colors.blue : colors.dim} />
        {alert && <View style={styles.tabDot} />}
      </View>
    </Pressable>
  );
}

export default function App() {
  const [connected, setConnected] = useState(false);
  // The Update tab only exists for chips with a bootloader path (ClaytonDisplay).
  const [canUpdate, setCanUpdate] = useState(true);
  const [fontsLoaded] = useFonts(fontAssets);

  useEffect(() => {
    const applyImmersiveMode = async () => {
      if (Platform.OS !== 'android') return;
      try {
        let NavigationBar = null;
        try {
          NavigationBar = require('expo-navigation-bar');
        } catch (moduleError) {
          NavigationBar = null;
        }
        if (NavigationBar?.setVisibilityAsync) {
          await NavigationBar.setVisibilityAsync('hidden');
        }
      } catch (e) {
        // expo-navigation-bar may not be available in all environments
      }
    };

    applyImmersiveMode();
    // Reconnect to the last unit and, if enabled, keep it in the background.
    backgroundService.start();
    const appStateSub = AppState.addEventListener('change', (state) => {
      if (state === 'active') applyImmersiveMode();
    });

    // Dashboard snapshots feed every screen (status bar, error popups,
    // forecast history), so polling lives here rather than in one screen.
    let pollTimer = null;
    const bleUnsub = deviceSession.onConnectionChange((c) => {
      setConnected(c);
      setCanUpdate(!c || deviceSession.capabilities.firmwareUpdate);
      clearInterval(pollTimer);
      pollTimer = null;
      if (c) {
        deviceSession.requestDashboard();
        pollTimer = setInterval(() => {
          if (deviceSession.isConnected) deviceSession.requestDashboard();
        }, DASHBOARD_POLL_MS);
      }
    });

    return () => {
      appStateSub.remove();
      bleUnsub();
      clearInterval(pollTimer);
    };
  }, []);

  if (!fontsLoaded) return <View style={styles.boot} />;

  return (
    <SafeAreaProvider>
      <StatusBar style="light" hidden={true} />
      <NavigationContainer theme={navTheme}>
        <Tab.Navigator
          screenOptions={({ route }) => ({
            headerShown: false,
            tabBarStyle: styles.tabBar,
            tabBarShowLabel: false,
            tabBarButton: (props) => (
              <TabButton {...props} routeName={route.name} alert={route.name === 'Connect' && !connected} />
            ),
          })}
          initialRouteName="Connect"
        >
          <Tab.Screen name="Dashboard" component={DashboardScreen} />
          <Tab.Screen name="Settings" component={SettingsScreen} />
          {canUpdate && <Tab.Screen name="Update" component={FirmwareUpdateScreen} />}
          <Tab.Screen name="Connect" component={ConnectScreen} />
        </Tab.Navigator>
      </NavigationContainer>
      <ErrorCenter />
    </SafeAreaProvider>
  );
}

const styles = StyleSheet.create({
  boot: { flex: 1, backgroundColor: colors.bg },
  tabBar: {
    backgroundColor: colors.bg,
    borderTopColor: colors.line,
    borderTopWidth: 1,
    height: 64,
    paddingTop: 0,
    paddingBottom: 0,
    elevation: 0,
  },
  tabCell: { flex: 1, alignItems: 'center', justifyContent: 'center' },
  tabCellOn: { backgroundColor: colors.panel },
  tabEdge: { position: 'absolute', top: 0, left: 0, right: 0, height: 3, backgroundColor: colors.blue },
  tabDot: { position: 'absolute', top: -3, right: -5, width: 8, height: 8, borderRadius: 4, backgroundColor: colors.red },
});
