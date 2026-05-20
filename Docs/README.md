# Clayton Power LPS HMI – Project Overview

Embedded display system for Clayton Power LPS (Lithium Power System) units.  
Consists of two components: an ESP32-S3 touchscreen HMI running on the CAN bus, and a companion BLE mobile app.

---

## Components

| Component | Location | Description |
|-----------|----------|-------------|
| **ESP32 CAN HMI Firmware** | `ESP32-S3-Touch-LCD-5-Demo/ESP-IDF/09_CAN_HMI/` | ESP-IDF firmware for the 5" touchscreen dashboard |
| **Clayton Power App** | `ClaytonPowerApp/` | React Native Expo app for Android/iOS |

---

## System Architecture

```
[LPS / BMS Unit]
      │
      │  CAN Bus (250 kbps, J1939 + CAN_Extra)
      │  TX: GPIO15  RX: GPIO16
      ▼
[ESP32-S3 HMI] ──── RGB LCD 1024×600 ──── [Touchscreen Dashboard]
      │
      │  BLE (NimBLE, GATT)
      │  Service: 0x1000
      ▼
[ClaytonPowerApp]  (Android / iOS phone)
```

---

## Documentation

| File | Contents |
|------|----------|
| [ESP32-Firmware.md](ESP32-Firmware.md) | Firmware architecture, CAN protocol, BLE protocol, LVGL UI, power management, build instructions |
| [MobileApp.md](MobileApp.md) | App architecture, screens, BLE connection flow, binary protocol parsing, build instructions |
| [Hardware.md](Hardware.md) | Board pinout, LCD wiring, CAN bus wiring, I2C peripherals |

---

## Quick Start

### Flash ESP32 Firmware

```powershell
# Load ESP-IDF environment
. "C:\Espressif\tools\Microsoft.v6.0.PowerShell_profile.ps1"

# Navigate to project
cd "ESP32-S3-Touch-LCD-5-Demo\ESP-IDF\09_CAN_HMI"

# Build + flash (COM11)
idf.py -p COM11 flash monitor
```

### Build Mobile App (EAS)

```bash
cd ClaytonPowerApp
eas build --platform android --profile preview
```

---

## Target Hardware

- **Board**: Waveshare ESP32-S3-Touch-LCD-5B (SKU 28292)
- **SoC**: ESP32-S3, 240 MHz dual-core, 16 MB Flash, 8 MB PSRAM (Octal, 120 MHz)
- **Display**: 5" 1024×600 RGB LCD, driver IC ST7262
- **Touch**: GT911 capacitive, I2C
- **IO Expander**: CH422G (I2C 0x24/0x38) — controls LCD reset, backlight, CTP reset
- **CAN Transceiver**: TWAI peripheral at GPIO 15/16
