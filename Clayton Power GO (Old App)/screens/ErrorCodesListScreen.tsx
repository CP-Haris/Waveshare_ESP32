import React, { useState, useMemo } from "react";
import { View, Text, StyleSheet, SafeAreaView, TouchableOpacity, ScrollView, StatusBar, TextInput, KeyboardAvoidingView, Platform } from "react-native";
import { useSafeAreaInsets } from 'react-native-safe-area-context';
import { useTranslation } from 'react-i18next';
import { useNavigation } from '@react-navigation/native';

export default function ErrorCodesListScreen() {
  const { t, i18n } = useTranslation();
  const insets = useSafeAreaInsets();
  const navigation = useNavigation();
  const [searchQuery, setSearchQuery] = useState('');

  // Get all error codes from translations
  const errorCodes = useMemo(() => {
    const deviceErrors = t('deviceErrors', { returnObjects: true }) as Record<string, { name: string; description: string }>;
    
    if (!deviceErrors || typeof deviceErrors !== 'object') {
      return [];
    }

    return Object.entries(deviceErrors)
      .map(([code, error]) => ({
        code,
        name: error.name || 'Unknown',
        description: error.description || 'No description available'
      }))
      .sort((a, b) => {
        // Sort ErrorUnknown first, then numeric codes
        if (a.code === 'ErrorUnknown') return -1;
        if (b.code === 'ErrorUnknown') return 1;
        
        const aNum = parseInt(a.code);
        const bNum = parseInt(b.code);
        
        if (isNaN(aNum) && isNaN(bNum)) return a.code.localeCompare(b.code);
        if (isNaN(aNum)) return 1;
        if (isNaN(bNum)) return -1;
        
        return aNum - bNum;
      });
  }, [t, i18n.language]);

  // Filter error codes based on search query
  const filteredErrorCodes = useMemo(() => {
    if (!searchQuery.trim()) return errorCodes;
    
    const query = searchQuery.toLowerCase();
    return errorCodes.filter(error => 
      error.code.toLowerCase().includes(query) ||
      error.name.toLowerCase().includes(query) ||
      error.description.toLowerCase().includes(query)
    );
  }, [errorCodes, searchQuery]);

  const getCurrentLanguageName = () => {
    const languages: Record<string, string> = {
      'en': 'English',
      'fr': 'Français',
      'de': 'Deutsch',
      'dk': 'Dansk',
      'sv': 'Svenska',
      'nl': 'Nederlands',
      'es': 'Español'
    };
    return languages[i18n.language] || i18n.language.toUpperCase();
  };

  return (
    <KeyboardAvoidingView 
      style={styles.safeArea} 
      behavior={Platform.OS === 'ios' ? 'padding' : 'height'}
      keyboardVerticalOffset={Platform.OS === 'ios' ? 0 : 20}
    >
      <StatusBar barStyle="light-content" backgroundColor="#202020" translucent={false} />
      <View style={[styles.container, { paddingTop: insets.top }]}>
        <View style={styles.header}>
          <TouchableOpacity 
            style={styles.backButton} 
            onPress={() => {
              // Navigate back to Settings safely
              navigation.navigate('Settings' as never);
            }}
          >
            <Text style={styles.backButtonText}>←</Text>
          </TouchableOpacity>
          <View style={styles.titleContainer}>
            <Text style={styles.title}>{t('errorCodesList.title')}</Text>
            <Text style={styles.subtitle}>{getCurrentLanguageName()}</Text>
          </View>
        </View>
        
        <View style={styles.searchContainer}>
          <TextInput
            style={styles.searchInput}
            placeholder={t('errorCodesList.searchPlaceholder')}
            placeholderTextColor="#999999"
            value={searchQuery}
            onChangeText={setSearchQuery}
            selectionColor="#0075c1"
            autoCorrect={false}
            autoCapitalize="none"
            returnKeyType="search"
            clearButtonMode="while-editing"
          />
        </View>

        
        <ScrollView 
          style={styles.scrollView} 
          showsVerticalScrollIndicator={false}
          keyboardShouldPersistTaps="handled"
          contentContainerStyle={styles.scrollViewContent}
        >
          {filteredErrorCodes.map((error, index) => (
            <View key={error.code} style={styles.errorItem}>
              <View style={styles.errorHeader}>
                <Text style={styles.errorCode}>
                  {error.code === 'ErrorUnknown' ? 'Unknown' : `${t('errorCodesList.errorPrefix')} ${error.code}`}
                </Text>
                <Text style={styles.errorName}>{error.name}</Text>
              </View>
              <Text style={styles.errorDescription}>{error.description}</Text>
            </View>
          ))}
          
          {filteredErrorCodes.length === 0 && (
            <View style={styles.noResultsContainer}>
              <Text style={styles.noResultsText}>{t('errorCodesList.noResultsTitle')}</Text>
              <Text style={styles.noResultsSubtext}>
                {t('errorCodesList.noResultsSubtext')}
              </Text>
            </View>
          )}
        </ScrollView>
      </View>
    </KeyboardAvoidingView>
  );
}

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
    backgroundColor: '#202020',
  },
  container: {
    flex: 1,
    padding: 20,
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 20,
  },
  backButton: {
    width: 40,
    height: 40,
    justifyContent: 'center',
    alignItems: 'center',
    marginRight: 10,
  },
  backButtonText: {
    fontSize: 24,
    color: '#ffffff',
    fontWeight: 'bold',
  },
  titleContainer: {
    flex: 1,
    alignItems: 'center',
    marginRight: 50, // Offset for back button
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#ffffff',
  },
  subtitle: {
    fontSize: 14,
    color: '#cccccc',
    marginTop: 2,
  },
  searchContainer: {
    marginBottom: 16,
  },
  searchInput: {
    backgroundColor: '#2a2a2a',
    borderRadius: 8,
    padding: 12,
    fontSize: 16,
    color: '#ffffff',
    borderWidth: 1,
    borderColor: '#404040',
  },
  scrollView: {
    flex: 1,
  },
  scrollViewContent: {
    flexGrow: 1,
    paddingBottom: 20,
  },
  errorItem: {
    backgroundColor: '#2a2a2a',
    borderRadius: 8,
    padding: 16,
    marginBottom: 12,
    borderLeftWidth: 4,
    borderLeftColor: '#0075c1',
  },
  errorHeader: {
    marginBottom: 8,
  },
  errorCode: {
    fontSize: 12,
    color: '#0075c1',
    fontWeight: '600',
    textTransform: 'uppercase',
    letterSpacing: 0.5,
  },
  errorName: {
    fontSize: 18,
    fontWeight: '600',
    color: '#ffffff',
    marginTop: 4,
  },
  errorDescription: {
    fontSize: 14,
    color: '#cccccc',
    lineHeight: 20,
  },
  noResultsContainer: {
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 40,
  },
  noResultsText: {
    fontSize: 18,
    color: '#ffffff',
    fontWeight: '500',
    marginBottom: 8,
  },
  noResultsSubtext: {
    fontSize: 14,
    color: '#cccccc',
  },
});
