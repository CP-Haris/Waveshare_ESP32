#!/usr/bin/env node

/**
 * Clean and rebuild script for Clayton Power app after Android upgrades
 */

const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

console.log('🧹 Starting clean rebuild process...\n');

// Function to run command and handle errors
function runCommand(command, description) {
  console.log(`⚡ ${description}...`);
  try {
    execSync(command, { stdio: 'inherit', cwd: process.cwd() });
    console.log(`✅ ${description} completed\n`);
  } catch (error) {
    console.log(`❌ ${description} failed`);
    console.log(`Command: ${command}`);
    console.log(`Error: ${error.message}\n`);
    return false;
  }
  return true;
}

// Function to remove directory if it exists
function removeDir(dirPath, description) {
  if (fs.existsSync(dirPath)) {
    console.log(`🗑️  Removing ${description}...`);
    fs.rmSync(dirPath, { recursive: true, force: true });
    console.log(`✅ ${description} removed\n`);
  }
}

console.log('Step 1: Cleaning build artifacts');
removeDir('node_modules', 'node_modules');
removeDir('android/build', 'Android build directory');
removeDir('android/app/build', 'Android app build directory');
removeDir('android/.gradle', 'Android .gradle directory');

console.log('Step 2: Reinstalling dependencies');
if (!runCommand('npm install', 'Installing npm dependencies')) {
  process.exit(1);
}

console.log('Step 3: Cleaning Android project');
process.chdir('android');
if (!runCommand('./gradlew clean', 'Cleaning Android project')) {
  process.exit(1);
}
process.chdir('..');

console.log('Step 4: Clearing Metro cache');
runCommand('npx react-native start --reset-cache', 'Clearing Metro cache (will exit after cache clear)');

console.log('🎉 Clean rebuild process completed!');
console.log('\n📱 Next steps:');
console.log('1. Run: npx react-native start');
console.log('2. In another terminal or press "a" to run on Android');
console.log('3. Test your app thoroughly on Android 15 / 16 KB devices');




