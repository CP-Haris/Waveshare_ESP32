import i18n from 'i18next';
import { initReactI18next } from 'react-i18next';
import { getStoredLanguage, saveLanguage } from './languageStorage';

// Import translation files
import en from './locales/en.json';
import fr from './locales/fr.json';
import de from './locales/de.json';
import dk from './locales/dk.json';
import sv from './locales/sv.json';
import nl from './locales/nl.json';
import es from './locales/es.json';

const resources = {
  en: {
    translation: en,
  },
  fr: {
    translation: fr,
  },
  de: {
    translation: de,
  },
  dk: {
    translation: dk,
  },
  sv: {
    translation: sv,
  },
  nl: {
    translation: nl,
  },
  es: {
    translation: es,
  },
};

// Initialize i18n synchronously first with default language
i18n
  .use(initReactI18next)
  .init({
    resources,
    lng: 'en', // Start with English, will be updated if stored language exists
    fallbackLng: 'en',
    debug: false,

    interpolation: {
      escapeValue: false, // not needed for react native as it escapes by default
    },
  });

// Then asynchronously load stored language
const loadStoredLanguage = async () => {
  try {
    const storedLanguage = await getStoredLanguage();
    if (storedLanguage && storedLanguage !== i18n.language) {
      await i18n.changeLanguage(storedLanguage);
    }
  } catch (error) {
    console.warn('Failed to load stored language:', error);
  }
};

// Load stored language
loadStoredLanguage();

// Save language changes to storage
i18n.on('languageChanged', (lng) => {
  saveLanguage(lng);
});

export default i18n; 