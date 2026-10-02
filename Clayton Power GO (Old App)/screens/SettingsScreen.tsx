import React from "react";
import { View, Text, StyleSheet, SafeAreaView, TouchableOpacity, ScrollView, StatusBar } from "react-native";
import { useSafeAreaInsets } from 'react-native-safe-area-context';
import { useTranslation } from 'react-i18next';
import { useNavigation } from '@react-navigation/native';

const languages: Record<string, string> = {
  'en': 'English',
  'fr': 'Français', 
  'de': 'Deutsch',
  'dk': 'Dansk',
  'sv': 'Svenska',
  'nl': 'Nederlands',
  'es': 'Español'
};

export default function SettingsScreen() {
  const { t, i18n } = useTranslation();
  const insets = useSafeAreaInsets();
  const navigation = useNavigation();

  const getCurrentLanguageName = () => {
    return languages[i18n.language] || i18n.language.toUpperCase();
  };

  return (
    <View style={styles.safeArea}>
      <StatusBar barStyle="light-content" backgroundColor="#202020" translucent={false} />
      <ScrollView style={styles.scrollView}>
        <View style={[styles.container, { paddingTop: insets.top }]}>
          <Text style={styles.title}>{t('settings.title')}</Text>
          
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>{t('settings.preferences')}</Text>
            
            {/* Language Selection */}
            <TouchableOpacity
              style={styles.settingItem}
              onPress={() => navigation.navigate('LanguageSelection' as never)}
            >
              <View style={styles.settingContent}>
                <Text style={styles.settingLabel}>{t('settings.language')}</Text>
                <Text style={styles.settingValue}>{getCurrentLanguageName()}</Text>
              </View>
              <Text style={styles.chevron}>›</Text>
            </TouchableOpacity>

          </View>

        </View>
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
    backgroundColor: '#202020',
  },
  scrollView: {
    flex: 1,
  },
  container: {
    flex: 1,
    padding: 20,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#ffffff',
    marginBottom: 30,
    textAlign: 'center',
  },
  section: {
    marginBottom: 30,
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: '600',
    color: '#ffffff',
    marginBottom: 16,
  },
  settingItem: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    padding: 16,
    backgroundColor: '#2a2a2a',
    borderRadius: 8,
    marginBottom: 8,
  },
  settingContent: {
    flex: 1,
  },
  settingLabel: {
    fontSize: 16,
    fontWeight: '500',
    color: '#ffffff',
    marginBottom: 2,
  },
  settingValue: {
    fontSize: 14,
    color: '#cccccc',
  },
  chevron: {
    fontSize: 20,
    color: '#cccccc',
    fontWeight: '300',
  },
});
