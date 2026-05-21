# Waveshare ESP32 Clayton Power HMI

Workspace for the Clayton Power ESP32-S3 CAN HMI firmware and the companion React Native mobile app.

## Active Components

| Component | Path | Purpose |
| --- | --- | --- |
| ESP32 CAN HMI | `ESP32-S3-Touch-LCD-5-Demo/ESP-IDF/09_CAN_HMI/` | ESP-IDF firmware for the Waveshare ESP32-S3 5 inch touch display, local LVGL UI, CAN decoding, and raw CAN-over-BLE gateway. |
| ClaytonPowerApp | `ClaytonPowerApp/` | Expo/React Native app for BLE connection, dashboard, settings, error overview, and CAN bootloader firmware update. |
| Docs | `Docs/` and `ClaytonPowerApp/docs/` | System overview, firmware notes, mobile app architecture, CAN gateway, and bootloader details. |

## Quick Commands

### ESP32 Firmware

```powershell
. "C:\esp\v6.0\esp-idf\export.ps1"
cd "ESP32-S3-Touch-LCD-5-Demo\ESP-IDF\09_CAN_HMI"
idf.py build
idf.py -p COM11 flash monitor
```

### Mobile App

```powershell
cd ClaytonPowerApp
npm install
npm run start
```

## Architecture Notes

- The ESP32 acts as a raw CAN-over-BLE gateway for the mobile app.
- The mobile app owns CAN parsing for dashboard, settings, unit discovery, errors, and firmware update planning.
- Active unit selection is global app-side state in the app header; selecting a unit does not send a BLE command to the ESP32.
- Firmware update uses the same raw CAN gateway and an exclusive BLE command lock.

See `Docs/README.md` for the full documentation index.