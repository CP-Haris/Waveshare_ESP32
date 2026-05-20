import React, { useState, useEffect } from 'react';
import { StatusBar } from 'expo-status-bar';
import { NavigationContainer, DefaultTheme } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { View, StyleSheet, Platform, AppState } from 'react-native';
import { MaterialIcons } from '@expo/vector-icons';

import DashboardScreen from './src/screens/DashboardScreen';
import SettingsScreen from './src/screens/SettingsScreen';
import ConnectScreen from './src/screens/ConnectScreen';
import FirmwareUpdateScreen from './src/screens/FirmwareUpdateScreen';
import { colors, radius } from './src/utils/theme';
import bleService from './src/services/bleService';

const Tab = createBottomTabNavigator();

const navTheme = {
  ...DefaultTheme,
  dark: true,
  colors: {
    ...DefaultTheme.colors,
    primary: colors.accent,
    background: colors.bg,
    card: colors.bgElevated,
    text: colors.text,
    border: colors.border,
  },
};

function TabIcon({ label, focused }) {
  const iconMap = {
    Dashboard: 'dashboard',
    Settings: 'settings',
    Connect: 'bluetooth-connected',
    Update: 'system-update',
  };

  return (
    <View style={styles.tabIcon}>
      <MaterialIcons
        name={iconMap[label] || 'circle'}
        size={22}
        color={focused ? colors.accent : colors.textMuted}
      />
    </View>
  );
}

export default function App() {
  const [connected, setConnected] = useState(false);

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
    const appStateSub = AppState.addEventListener('change', (state) => {
      if (state === 'active') applyImmersiveMode();
    });

    const bleUnsub = bleService.onConnectionChange(setConnected);

    return () => {
      appStateSub.remove();
      bleUnsub();
    };
  }, []);

  return (
    <>
      <StatusBar style="light" hidden={true} />
      <NavigationContainer theme={navTheme}>
        <Tab.Navigator
          screenOptions={({ route }) => ({
            headerShown: false,
            tabBarStyle: styles.tabBar,
            tabBarShowLabel: false,
            tabBarIcon: ({ focused }) => (
              <TabIcon label={route.name} focused={focused} />
            ),
          })}
          initialRouteName="Connect"
        >
          <Tab.Screen name="Dashboard" component={DashboardScreen} />
          <Tab.Screen name="Settings" component={SettingsScreen} />
          <Tab.Screen name="Update" component={FirmwareUpdateScreen} />
          <Tab.Screen
            name="Connect"
            component={ConnectScreen}
            options={{
              tabBarBadge: connected ? undefined : '!',
              tabBarBadgeStyle: styles.badge,
            }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </>
  );
}

const styles = StyleSheet.create({
  tabBar: {
    backgroundColor: colors.bgElevated,
    borderTopColor: colors.borderSubtle,
    borderTopWidth: 1,
    height: 70,
    paddingBottom: 8,
    paddingTop: 8,
  },
  tabIcon: { alignItems: 'center', justifyContent: 'center' },
  badge: {
    backgroundColor: colors.red,
    fontSize: 10,
    minWidth: 16,
    height: 16,
    lineHeight: 16,
    borderRadius: radius.full,
  },
});
