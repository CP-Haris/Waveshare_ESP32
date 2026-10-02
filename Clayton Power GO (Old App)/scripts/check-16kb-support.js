#!/usr/bin/env node

/**
 * Script to check 16 KB page size support for Clayton Power app
 * Run this script to validate your app's 16 KB compatibility
 */

const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

console.log('🔍 Checking 16 KB page size support for Clayton Power...\n');

// Check if device is connected
try {
  const devices = execSync('adb devices', { encoding: 'utf8' });
  if (!devices.includes('device')) {
    console.log('❌ No Android device connected. Please connect a device or start an emulator.');
    process.exit(1);
  }
  console.log('✅ Android device detected');
} catch (error) {
  console.log('❌ ADB not found. Please install Android SDK tools.');
  process.exit(1);
}

// Check device page size
try {
  const pageSize = execSync('adb shell getconf PAGE_SIZE', { encoding: 'utf8' }).trim();
  console.log(`📱 Device page size: ${pageSize} bytes`);
  
  if (pageSize === '16384') {
    console.log('✅ Device is running in 16 KB mode');
  } else if (pageSize === '4096') {
    console.log('⚠️  Device is running in 4 KB mode (legacy)');
    console.log('   For full testing, enable 16 KB mode in Developer Options');
  } else {
    console.log(`⚠️  Unexpected page size: ${pageSize}`);
  }
} catch (error) {
  console.log('❌ Could not check device page size');
}

// Check if APK exists and validate alignment
const apkPath = path.join(__dirname, '../android/app/build/outputs/apk/release/app-release.apk');
if (fs.existsSync(apkPath)) {
  console.log('\n📦 Checking APK alignment...');
  try {
    execSync(`zipalign -c -P 16 -v 4 "${apkPath}"`, { encoding: 'utf8' });
    console.log('✅ APK is properly aligned for 16 KB page sizes');
  } catch (error) {
    console.log('❌ APK alignment check failed');
    console.log('   Build a release APK first: npm run android -- --mode release');
  }
} else {
  console.log('\n📦 No release APK found');
  console.log('   Build a release APK to check alignment: npm run android -- --mode release');
}

// Check gradle properties
console.log('\n⚙️  Checking configuration...');
const buildGradlePath = path.join(__dirname, '../android/build.gradle');
if (fs.existsSync(buildGradlePath)) {
  const buildGradle = fs.readFileSync(buildGradlePath, 'utf8');
  
  if (buildGradle.includes('targetSdkVersion = Integer.parseInt(findProperty(\'android.targetSdkVersion\') ?: \'35\')')) {
    console.log('✅ Target SDK configured for Android 15 (API 35)');
  } else {
    console.log('⚠️  Target SDK should be set to 35 for Android 15');
  }
}

// Check manifest for target SDK
const manifestPath = path.join(__dirname, '../android/app/src/main/AndroidManifest.xml');
if (fs.existsSync(manifestPath)) {
  console.log('✅ AndroidManifest.xml found');
}

console.log('\n🎯 Next steps:');
console.log('1. Build and test your app on a 16 KB device/emulator');
console.log('2. Run: adb shell getconf PAGE_SIZE (should return 16384)');
console.log('3. Test all app functionality thoroughly');
console.log('4. Build release APK and verify alignment');
console.log('\n📚 For more info: https://developer.android.com/guide/practices/page-sizes');
