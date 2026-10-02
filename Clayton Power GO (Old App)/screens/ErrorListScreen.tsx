import React, { useLayoutEffect, useState } from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity, Alert, ActivityIndicator, StatusBar } from 'react-native';
import { useSafeAreaInsets } from 'react-native-safe-area-context';
import { useRoute, RouteProp, useNavigation } from '@react-navigation/native';
import { useError } from '../context/ErrorContext';
import CommService from '../ble/services/CommService';
import useDeviceDisconnected from '../hooks/useDeviceDisconnected';

// Types

type RootStackParamList = {
  ErrorList: { deviceId: string };
};

type ErrorListScreenRouteProp = RouteProp<RootStackParamList, 'ErrorList'>;

export default function ErrorListScreen() {
  const route = useRoute<ErrorListScreenRouteProp>();
  const navigation = useNavigation();
  const { deviceId } = route.params;
  const { ignoredErrors, setIgnoredErrors } = useError();
  const [isClearing, setIsClearing] = useState(false);
  const handleDisconnect = useDeviceDisconnected(deviceId);
  const insets = useSafeAreaInsets();

  // Handle device disconnect (manual or automatic)
  React.useEffect(() => {
    // Only run this effect once on mount
    let didRun = false;
    if (!didRun) {
      const originalHandleDisconnect = handleDisconnect;
      const wrappedHandleDisconnect = async () => {
        setIgnoredErrors([]);
        await originalHandleDisconnect();
        navigation.goBack();
      };
      // Use wrappedHandleDisconnect as needed
    }
    // No cleanup needed
  }, [handleDisconnect, setIgnoredErrors, navigation]);

  // Add clear button to header
  useLayoutEffect(() => {
    navigation.setOptions({
      headerRight: () => (
        <TouchableOpacity
          onPress={() => {
            Alert.alert(
              'Clear All Errors',
              'Are you sure you want to clear all ignored errors?',
              [
                { text: 'Cancel', style: 'cancel' },
                {
                  text: 'Clear',
                  style: 'destructive',
                  onPress: async () => {
                    setIsClearing(true);
                    const start = Date.now();
                    try {
                      const commService = new CommService(deviceId);
                      await commService.clearErrors();
                      setIgnoredErrors([]);
                    } catch (e) {
                      // Optionally show error
                    }
                    const elapsed = Date.now() - start;
                    if (elapsed < 1000) {
                      await new Promise(resolve => setTimeout(resolve, 1000 - elapsed));
                    }
                    setIsClearing(false);
                    navigation.goBack();
                  },
                },
              ]
            );
          }}
          disabled={isClearing}
          style={styles.headerButton}
        >
          <Text style={[styles.headerButtonText, isClearing && { opacity: 0.5 }]}>Clear</Text>
        </TouchableOpacity>
      ),
      headerTitle: 'Ignored Errors',
    });
  }, [navigation, deviceId, isClearing, setIgnoredErrors]);

  return (
    <View style={styles.safeArea}>
      <StatusBar barStyle="light-content" backgroundColor="#202020" translucent={false} />
      <View style={[styles.container, { paddingTop: insets.top }]}>
      <FlatList
        data={ignoredErrors}
        keyExtractor={(item) => item.code.toString()}
        renderItem={({ item }) => (
          <View style={styles.errorItem}>
            <Text style={styles.errorName}>{item.name}</Text>
            <Text style={styles.errorDescription}>{item.description}</Text>
          </View>
        )}
        ListEmptyComponent={
          <View style={styles.emptyContainer}>
            <Text style={styles.emptyText}>No ignored errors</Text>
          </View>
        }
        contentContainerStyle={{ flexGrow: 1 }}
        ListFooterComponent={
          <TouchableOpacity
            style={styles.backButton}
            onPress={() => navigation.goBack()}
            disabled={isClearing}
          >
            <Text style={styles.backButtonText}>Back</Text>
          </TouchableOpacity>
        }
      />
      {isClearing && (
        <View style={styles.loadingOverlay}>
          <ActivityIndicator size="large" color="#fff" />
        </View>
      )}
      </View>
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
    backgroundColor: '#202020',
    padding: 16,
  },
  errorItem: {
    backgroundColor: '#303030',
    padding: 16,
    marginBottom: 8,
    borderRadius: 8,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.2,
    shadowRadius: 4,
    elevation: 2,
  },
  errorName: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  errorDescription: {
    color: '#cccccc',
    fontSize: 14,
  },
  emptyContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 32,
  },
  emptyText: {
    color: '#666666',
    fontSize: 16,
  },
  headerButton: {
    marginRight: 16,
    padding: 8,
  },
  headerButtonText: {
    color: '#ff4d4f', // reddish
    fontSize: 16,
    fontWeight: 'bold',
  },
  backButton: {
    backgroundColor: '#303030',
    padding: 16,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 16,
    marginBottom: 16,
  },
  backButtonText: {
    color: '#ffffff',
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
}); 