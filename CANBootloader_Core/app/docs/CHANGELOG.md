# Changelog

All notable changes to the CAN Bootloader Console application will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed
- Documentation refresh for app and backend operation guides.
- Remote Mac debugging docs now use placeholders for host/user instead of embedded credentials.
- Tasks documentation aligned with current `../../.vscode/tasks.json`.

### Added
- Backend documentation index and deployment/API guide under `backend/docs/`.

## [2.3.0] - 2026-03-03

### Changed
- **Repository moved** to new organization: `IOT-Freund/Bootloader_WinMacConsoleCanBootloader` (previously `CP-Haris/CANBootloader_Core`)
- **BootloaderProtocol**: Refactored from synchronous blocking protocol to async timer-based communication
  - Timer-based communication timeout with automatic retry (up to 5 retries)
  - Platform-specific timeouts (Windows 2000ms, macOS/Linux 3000ms)
  - Thread-safe state machine with lock object
  - Removed synchronous `SendCommandWithRetry` pattern
- **CANPort**: Simplified by removing synchronous read infrastructure
  - Removed `PauseAsyncReceive`/`ResumeAsyncReceive` methods
  - Removed `CANMessage` class for blocking reads
- **FirmwareUploader**: Refactored upload flow
  - Added `CancellationTokenSource` support
  - Removed `BootloaderSessionState` class (batch session management)
  - Simplified device detection (count-based instead of HashSet tracking)
  - Added Debug-mode logging for connection and discovery steps
  - Adjusted communication timing delays for improved reliability

### Removed
- Synchronous blocking read/write helpers in CANPort
- `BootloaderSessionState` class and `partNumber` parameter from FirmwareUploader

---

## [2.2.0] - 2026-01-29

### Added
- **Conditional Advanced Mode Build**: Compiler directive `ENABLE_ADVANCED_MODE` to control M menu visibility
  - Simple builds: No Advanced/Debug mode switching (for production operators)
  - Advanced builds: Full mode switching capability (for developers/support)
  - Build command: `-p:DefineConstants="ENABLE_ADVANCED_MODE"` or `-p:DefineConstants="SIMPLE_MODE_ONLY"`
- **Application Icon**: Added ProgramIcon.ico for Windows executable branding
- **Bootloader Serial Number Copy**: Automatically copies application serial to bootloader when bootloader has no serial

### Fixed
- Output formatting consistency between Option 1 (Update Firmware) and Option 2 (Reload Current Firmware)
- Bootloader update failure when device serial number only exists in application area
- Bridge 3 (Display) firmware update reliability on specific devices

### Changed
- Removed debug CAN logging from CANPort.cs (cleaner production code)

## [2.2.0.0] - Previous Release

### Features
- Interactive menu system with three operator modes (Simple, Advanced, Debug)
- Cross-platform support (Windows x64, macOS ARM64/Intel)
- Centralized firmware source integration for firmware management
- Encrypted local firmware cache for offline operation
- Automatic device discovery and firmware version detection
- Reliable firmware upload with retry logic
- Multi-device and bridge board support (up to 5 boards)
- Serial port auto-detection with VID/PID matching
- Platform-specific serial port optimizations

### Platform Support
- Windows: Full support with EV code signing
- macOS: ARM64 and Intel builds with optimized serial communication
- Self-contained single-file deployment

### Security
- EV Code Signing certificate for Windows executables (Clayton Power A/S)
- Encrypted firmware cache using AES-256
- Secure credential handling
