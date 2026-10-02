import AsyncStorage from '@react-native-async-storage/async-storage';

const LANGUAGE_KEY = '@user_language';

export const saveLanguage = async (language: string): Promise<void> => {
  try {
    await AsyncStorage.setItem(LANGUAGE_KEY, language);
  } catch (error) {
    console.error('Error saving language:', error);
  }
};

export const getStoredLanguage = async (): Promise<string | null> => {
  try {
    const language = await AsyncStorage.getItem(LANGUAGE_KEY);
    return language;
  } catch (error) {
    console.error('Error getting stored language:', error);
    return null;
  }
};

export const clearStoredLanguage = async (): Promise<void> => {
  try {
    await AsyncStorage.removeItem(LANGUAGE_KEY);
  } catch (error) {
    console.error('Error clearing stored language:', error);
  }
}; 