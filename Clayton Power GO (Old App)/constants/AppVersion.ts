import { Platform } from 'react-native';
import DeviceInfo from 'react-native-device-info';

// These should be updated to match the native project versions
// iOS version is in ios/ClaytonPower/Info.plist (CFBundleShortVersionString)
// Android version is in android/app/build.gradle (versionName)
export const APP_VERSIONS = {
  ios: '1.2.0',        // From Info.plist CFBundleShortVersionString
  android: '1.2.0',    // From build.gradle versionName
};

// Build numbers
// iOS build is in ios/ClaytonPower.xcodeproj/project.pbxproj (CURRENT_PROJECT_VERSION)
// Android build is in android/app/build.gradle (versionCode)
export const BUILD_NUMBERS = {
  ios: '4',           // From CURRENT_PROJECT_VERSION
  android: '7',       // From versionCode
};

// Get real app version from the native app bundle
export const getAppVersion = (): string | null => {
  try {
    return DeviceInfo.getVersion();
  } catch (error) {
    console.log('Could not retrieve app version:', error);
    return null;
  }
};

// Get real build number from the native app bundle
export const getBuildNumber = (): string | null => {
  try {
    return DeviceInfo.getBuildNumber();
  } catch (error) {
    console.log('Could not retrieve build number:', error);
    return null;
  }
};

// Get formatted version string with platform info - only if real data is available
export const getFullVersionString = (): string | null => {
  const version = getAppVersion();
  const build = getBuildNumber();
  
  // Only return version string if we have real version data
  if (!version) {
    return null;
  }
  
  const platform = Platform.OS === 'ios' ? 'iOS' : 'Android';
  const buildString = build ? ` (${build})` : '';
  
  return `Version ${version}${buildString} - ${platform}`;
};

// This can be automated in your build process to read from package.json
// or native project files during build time 