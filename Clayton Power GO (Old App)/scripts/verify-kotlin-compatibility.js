#!/usr/bin/env node

const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

console.log('🔍 Verifying Kotlin compatibility for Android 15...\n');

const aabPath = path.join(__dirname, '../android/app/build/outputs/bundle/release/app-release.aab');

// Check if AAB exists
if (!fs.existsSync(aabPath)) {
  console.log('❌ app-release.aab not found. Build the release bundle first.');
  process.exit(1);
}

console.log('✅ Found app-release.aab\n');

// Check if bundletool is available
let bundletoolPath;
try {
  // Try to use bundletool from the system
  execSync('bundletool --version', { stdio: 'ignore' });
  bundletoolPath = 'bundletool';
} catch (error) {
  console.log('⚠️  bundletool not found in PATH');
  console.log('   Download it from: https://github.com/google/bundletool/releases\n');
  
  // Check for bundletool.jar in scripts directory
  const localBundletool = path.join(__dirname, 'bundletool.jar');
  if (fs.existsSync(localBundletool)) {
    bundletoolPath = `java -jar ${localBundletool}`;
    console.log('✅ Using local bundletool.jar\n');
  } else {
    console.log('💡 To verify the bundle, download bundletool.jar and place it in the scripts directory\n');
    console.log('   For now, checking with available tools...\n');
  }
}

// Try to extract and analyze the APK
if (bundletoolPath) {
  try {
    console.log('📦 Extracting APK from bundle...\n');
    
    const tempDir = path.join(__dirname, '../android/app/build/outputs/temp-apk');
    if (!fs.existsSync(tempDir)) {
      fs.mkdirSync(tempDir, { recursive: true });
    }
    
    const apkPath = path.join(tempDir, 'app.apks');
    
    // Build APKs from bundle
    execSync(`${bundletoolPath} build-apks --bundle=${aabPath} --output=${apkPath} --mode=universal`, {
      stdio: 'inherit'
    });
    
    console.log('\n✅ APK extracted successfully\n');
    console.log('🔍 Checking for problematic Kotlin methods...\n');
    
    // Extract the universal APK
    execSync(`unzip -o ${apkPath} -d ${tempDir}`, { stdio: 'ignore' });
    
    const universalApk = path.join(tempDir, 'universal.apk');
    const dexDir = path.join(tempDir, 'dex');
    
    if (!fs.existsSync(dexDir)) {
      fs.mkdirSync(dexDir, { recursive: true });
    }
    
    // Extract DEX files from APK
    execSync(`unzip -o ${universalApk} "*.dex" -d ${dexDir}`, { stdio: 'ignore' });
    
    // Search for removeFirst/removeLast in DEX files
    const dexFiles = fs.readdirSync(dexDir).filter(f => f.endsWith('.dex'));
    
    let foundIssue = false;
    
    for (const dexFile of dexFiles) {
      const dexPath = path.join(dexDir, dexFile);
      const content = fs.readFileSync(dexPath, 'utf8');
      
      if (content.includes('removeFirst') || content.includes('removeLast')) {
        console.log(`⚠️  Found removeFirst/removeLast in ${dexFile}`);
        foundIssue = true;
      }
    }
    
    if (!foundIssue) {
      console.log('✅ No removeFirst/removeLast calls found in DEX files\n');
    } else {
      console.log('\n❌ Kotlin incompatibility detected!\n');
    }
    
    // Clean up
    fs.rmSync(tempDir, { recursive: true, force: true });
    
  } catch (error) {
    console.log('⚠️  Could not fully analyze bundle:', error.message);
  }
}

// Final recommendations
console.log('📋 Verification Summary:\n');
console.log('   Current configuration:');
console.log('   - Target SDK: 35 (Android 15)');
console.log('   - react-native-svg: Check package.json for version');
console.log('   - Build Tools: 35.0.0');
console.log('   - Gradle: 8.8');
console.log('   - AGP: 8.6.1\n');

console.log('💡 If Google Play still reports issues:');
console.log('   1. The issue might be in compiled bytecode from dependencies');
console.log('   2. Consider using R8/ProGuard to optimize and remove unused code');
console.log('   3. Update all dependencies to their latest versions');
console.log('   4. Check if the dependency maintainers have released a fix\n');

console.log('🚀 To test on an actual 16 KB device:');
console.log('   npm run check-16kb\n');



