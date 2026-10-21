# Clayton Power Mobile App

React Native Expo companion app for monitoring and controlling LPS units via BLE.  
**Path**: `ClaytonPowerApp/`

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [File Structure](#2-file-structure)
3. [Dependencies](#3-dependencies)
4. [Build & Deploy](#4-build--deploy)
5. [Navigation & Screens](#5-navigation--screens)
6. [BLE Service](#6-ble-service)
7. [Binary Protocol](#7-binary-protocol)
8. [CAN Gateway Parsing](#8-can-gateway-parsing)
9. [Components](#9-components)
10. [Theme](#10-theme)

---

## 1. Project Overview

The Clayton Power App connects to the ESP32 HMI via Bluetooth Low Energy.  
It displays live power system telemetry, system status, and active error codes.  
The app communicates with the ESP32 as a CAN-over-BLE gateway. Dashboard, settings, unit discovery, errors, and firmware updates are implemented in the app by sending and decoding raw CAN frames.

**Target platforms**: Android (primary), iOS  
**App ID**: `com.claytonpower.hmi`  
**EAS Account**: `cp_haris` (hh@claytonpower.com)  
**Advertised device name**: `Clayton Power`

---

## 2. File Structure

```
ClaytonPowerApp/
├── App.js                        # Navigation root (Bottom Tabs)
├── index.js                      # Expo app registration
├── package.json                  # Dependencies
├── app.json                      # Expo config (name, slug, permissions)
├── eas.json                      # EAS build profiles
├── src/
│   ├── ble/
│   │   ├── bleLink.js            # BLE transport for both chips (scan, connect, pairing, I/O)
│   │   └── gattProfiles.js       # GATT UUIDs, chip detection, LPS2 advert parsing
│   ├── devices/
│   │   ├── deviceSession.js      # Facade used by all screens; picks the driver, exposes capabilities
│   │   ├── dashboardModel.js     # Shared dashboard snapshot (normalized functions)
│   │   ├── display/              # Path A: ClaytonDisplay CAN gateway
│   │   │   ├── displayDevice.js
│   │   │   ├── displayProtocol.js
│   │   │   ├── firmwareUpdateService.js
│   │   │   └── firmwareUpdateHelpers.js
│   │   └── lps2/                 # Path B: LPS2 built-in BLE
│   │       ├── lps2Device.js
│   │       └── lps2Frame.js
│   ├── screens/                  # Dashboard, Settings, Update, Connect
│   ├── components/               # Carbon Blue UI components
│   ├── services/socHistory.js    # SoC ring buffer for the forecast chart
│   └── utils/                    # theme, errorCodes, units
└── assets/                       # Icons & splash screens
```

---

## 3. Dependencies

| Package | Version | Purpose |
|---------|---------|---------|
| `react-native` | — | Core framework |
| `expo` | — | Managed workflow |
| `react-native-ble-plx` | 3.5.1 | BLE scanning, connection, GATT |
| `@react-navigation/native` | v7 | Navigation container |
| `@react-navigation/bottom-tabs` | v7 | Bottom tab navigator |
| `react-native-svg` | 15.12 | Gauges, chart, pictograms (native — needs a dev build that includes it) |
| `expo-font` | 14 | Loads Barlow Semi Condensed (`assets/fonts/`) |

---

## 4. Build & Deploy

### Prerequisites

- Node.js + npm/yarn
- EAS CLI: `npm install -g eas-cli`
- EAS account: `cp_haris` (hh@claytonpower.com)

### Development Build

```bash
cd ClaytonPowerApp

# Install dependencies
npm install

# Start Expo dev server
npx expo start
```

### EAS Build (Android APK)

```bash
# Login to EAS
eas login

# Build Android APK (preview profile = APK, not AAB)
eas build --platform android --profile preview

# Download APK from EAS dashboard and install on device
```

### EAS Build Profiles (`eas.json`)

| Profile | Output | Use case |
|---------|--------|---------|
| `preview` | APK | Direct install for testing |
| `production` | AAB | Google Play submission |

### Android Permissions (auto-configured in `app.json`)

- `BLUETOOTH_SCAN`
- `BLUETOOTH_CONNECT`
- `ACCESS_FINE_LOCATION` (required for BLE scan on Android ≤12)

---

## 5. Navigation & Screens

### Navigation Structure (`App.js`)

Bottom Tab Navigator with 4 tabs:

| Tab | Icon | Screen | Initial Route |
|-----|------|--------|---------------|
| Dashboard | dashboard | `DashboardScreen` | — |
| Settings | settings | `SettingsScreen` | — |
| Update | system-update | `FirmwareUpdateScreen` | — |
| Connect | bluetooth-connected | `ConnectScreen` | yes |

**Connection badge**: Red `!` badge on the Connect tab when BLE is disconnected.

---

### Screen: Connect (`src/screens/ConnectScreen.js`)

Entry point for BLE device pairing.

**Flow:**
1. Request Android runtime permissions (BLUETOOTH_SCAN, BLUETOOTH_CONNECT, ACCESS_FINE_LOCATION)
2. Tap "Scan" → 5-second scan filtering for Clayton Power service UUID
3. List shows found devices: name, MAC address, RSSI signal strength
4. Tap a device to connect (spinner shown during connection)
5. After connection: "Connected ✅" status + disconnect button

**Pairing note**: The ESP32 uses a 6-digit passkey displayed on the LCD. The user must enter it when prompted by Android's pairing dialog.

---

### Screen: Dashboard (`src/screens/DashboardScreen.js`)

Real-time monitoring of the selected LPS/BMS unit.

**Data source**: Ensures CAN passthrough is enabled, decodes live CAN broadcasts in `src/devices/display/displayDevice.js`, and refreshes the visible snapshot every **2 seconds**. It does not request an ESP32-generated dashboard packet.

**Layout (scrollable, top to bottom):**

| Section | Content |
|---------|---------|
| Header | Shared Clayton Power header with global unit switcher |
| SOC Ring | Large circular arc (0–100%), time-to-full/empty below |
| Product identity | Part number, serial number, unit family |
| System power | Product-specific system cards. CL/LPS shows DC Output, DC Input, Inverter, Charger, Solar. CB/Battery shows Battery. |
| Quick Controls | Inverter and DC Output toggles for LPS units only |
| Errors | Error overview modal with decoded error meanings and clear-errors command |

**State machine:**
- State names: Error (−1), Off (0), On (1), Standby (2), Charge (3), Float (4)

---

### Screen: Settings (`src/screens/SettingsScreen.js`)

CAN_Extra configuration editor for the globally selected unit.

**Sections:**

| Section | Content |
|---------|---------|
| Diagnostics | Active error count for the selected unit |
| Configuration Profiles | LPS/BMS-specific settings categories |
| Detail editor | Reads ranges and values over CAN_Extra, then writes updated values back to the selected unit |

Unit switching is not owned by Settings. The shared header `UnitSwitcher` controls the app-wide active unit; Settings clears pending reads/writes and editor state when that active unit changes.

---

### Screen: Firmware Update (`src/screens/FirmwareUpdateScreen.js`)

Bootloader update flow over the same raw CAN-over-BLE gateway.

**Flow:**
1. Uses the header-selected unit as the preferred CAN target.
2. Discovers part number, serial number, and bridge firmware versions from CAN frames.
3. Builds a product-specific update plan: CB/Battery exposes bridge 1; CL/LPS exposes bridges 1-4.
4. Shows non-responding CL/LPS bridges as `Cannot update` instead of hiding them.
5. Runs updates only for bridges with a valid current version and newer compatible released firmware.

---

## 6. BLE Service

### Configuration (`src/ble/gattProfiles.js`, `src/ble/bleLink.js`)

Library: `react-native-ble-plx`

| Item | Value |
|------|-------|
| Service UUID | `00001000-0000-1000-8000-00805f9b34fb` |
| TX Char (Notify) | `00001001-0000-1000-8000-00805f9b34fb` |
| RX Char (Write) | `00001002-0000-1000-8000-00805f9b34fb` |
| MTU | 256 bytes |
| Write type | WriteWithoutResponse |

> **Note on naming**: "TX" from the firmware perspective notifies the phone. "RX" from the firmware perspective receives writes from the phone. The app naming follows the app's perspective (TX = data to app, RX = commands from app).

### Key Methods

| Method | Description |
|--------|-------------|
| `scan(timeoutMs=5000)` | Scan for devices advertising the service UUID. Returns `[{id, name, rssi}]` |
| `connect(deviceId)` | MTU negotiation → service/characteristic discovery → subscribe to TX notifications |
| `disconnect()` | Cancel connection, clean up listeners |
| `writeCommand(base64Data)` | Write base64-encoded bytes to RX characteristic |
| `onNotification(fn)` | Register callback for decoded incoming messages |
| `onConnectionChange(fn)` | Register callback for connect/disconnect events |
| `isConnected` | Boolean property — current connection state |

### Incoming Message Dispatch

All BLE notifications arrive on the TX characteristic.  
`decodeNotification()` only decodes raw CAN frame notifications. App-level Dashboard and Settings events are emitted by `displayDevice` after it parses those CAN frames.

```javascript
{ type: 'canFrame', data: { canId, dlc, data } }
```

---

## 7. Binary Protocol

All BLE payloads use a 1-byte type/command prefix followed by a type-specific payload. Multi-byte values are little-endian. Commands are base64-encoded before writing to BLE.

### Message Types (ESP32 → Phone)

| ID | Constant | Description |
|----|----------|-------------|
| `0x08` | `MSG.CAN_FRAME` | Raw 29-bit CAN frame: `[can_id_u32_le][dlc][data8]` |

### Command Types (Phone → ESP32)

| ID | Constant | Payload | Description |
|----|----------|---------|-------------|
| `0x18` | `CMD.SET_CAN_PASSTHROUGH` | `[enabled]` | Enable or disable CAN forwarding |
| `0x19` | `CMD.SEND_CAN_FRAME` | `[can_id_u32_le][dlc][data8]` | Send one CAN frame |
| `0x1A` | `CMD.SEND_CAN_FRAMES` | `[count][can_id_u32_le][dlc][data8]...` | Send a batch of CAN frames |
| `0x1B` | `CMD.SYNC_TIME` | `[year_u16_le][month][day][hour][min][sec][force]` | Sent on every connect to a ClaytonDisplay; the display only uses it while its clock is unset |

### Command Encoding

```javascript
export function encodeSetCanPassthrough(enabled) { ... }
export function encodeSendCanFrame(canId, dataBytes) { ... }
export function encodeSendCanFrames(frames) { ... }
```

---

## 8. CAN Gateway Parsing

Dashboard and Settings are decoded from CAN frames in `src/devices/display/displayDevice.js`.

- Broadcast telemetry uses Clayton/J1939 `0x18FF`, `0x19FF`, and `0x14FF` frames.
- Unit discovery sends `0x18EAFFFE` requests and decodes identification responses.
- Settings use CAN_Extra frames on `0x19EF[target][FE]`.
- Firmware update uses the same raw gateway transport and batches CAN frames with command `0x1A`.

---

## 9. Device Layer

The app talks to two BLE chips: a ClaytonDisplay (CAN gateway, many units, firmware update) and the LPS2 built-in module (one unit, no CAN, no firmware update; see [LPS2-BLE-Protocol.md](LPS2-BLE-Protocol.md)). Screens only use `deviceSession`, which forwards to `displayDevice` or `lps2Device` and exposes `capabilities` (`multiUnit`, `settings`, `firmwareUpdate`). Both drivers emit the same `dashboard` snapshot defined in `dashboardModel.js`.

---

## 10. Background Connection and Notifications

Settings › APP › *Background notifications* (off by default, Android first):

- The last connected unit is remembered (`prefs.js`). `backgroundService.js` reconnects to it with Android `autoConnect`, which waits at low power until the unit is in range — no scanning. Reconnecting runs while the app is open, or always when the setting is on. Pressing DISCONNECT stops it until the user connects again.
- With the setting on, `modules/ble-foreground` runs an Android foreground service of type `connectedDevice`. It keeps the app process (and the JS BLE link) alive and shows the ongoing notification Android requires. It is started while the app is in the foreground; afterwards only its text is updated.
- `alertMonitor.js` reads the normal dashboard snapshots and posts notifications only while the app is in the background: every new error code (all levels), SoC below 20 % (re-armed above 25 %), and 100 % while charging (re-armed below 95 %). State is kept per unit, so returning to the background never repeats what was already shown.
- Permissions: `POST_NOTIFICATIONS` (asked when the setting is turned on), `FOREGROUND_SERVICE`, `FOREGROUND_SERVICE_CONNECTED_DEVICE` (declared by the module).
- Limits: phone makers' battery savers can still kill the service; a phone restart stops it until the app is opened again. iOS is not covered yet.

---

## 11. Components

The UI follows the Carbon Blue language shared with the display. The full
visual spec is in [Carbon Blue App-spec.md](Carbon%20Blue%20App-spec.md);
the display spec it derives from is [Carbon Blue Designspec.md](Carbon%20Blue%20Designspec.md).

| Component | Role |
|-----------|------|
| `StatusBar` | 48 dp status line on every tab: error badge (opens the error list), BLE state, unit chip, clock |
| `UnitSwitcher` | Unit chip (`LPS`/`BMS` + last four serial digits) and picker sheet |
| `ErrorCenter` | Mounted once in `App.js`; error list sheet and automatic popup for new FAILURE/CRITICAL codes |
| `Dial` | 270° gauge; icon colour is the function state (`dialState()` ports the firmware's `set_dial()`); with `onPress` it sits on a round button plate |
| `ForecastChart` | Time-symmetric SoC chart (history left of NOW, time-true projection right), port of the firmware's `update_chart()` |
| `Chevrons` | Marching chevrons showing flow direction |
| `Carbon` | `ScreenTitle`, `Section`, `Row`, `Button`, `Sheet`, `IconButton`, `Notice` |

Dashboard polling (`requestDashboard()` every 2 s) runs in `App.js` while BLE is connected, so the status bar, error popups and forecast history update on every tab.

---

## 12. Theme

Carbon Blue tokens in `src/utils/theme.js` (identical values to the display):

| Token | Value | Use |
|-------|-------|-----|
| `bg` | `#0B0C0E` | Screen background (near-black) |
| `panel` | `#26292E` | Grey plates: buttons, dial plates, chip, popups |
| `sheet` | `#1A1C20` | Bottom sheets |
| `line` | `#26282C` | Hairline dividers |
| `track` | `#1F2125` | Empty arcs/tracks |
| `ink` | `#EFEDE8` | Numbers, labels, arcs |
| `dim` | `#82868C` | Secondary text, off state |
| `blue` | `#4E9EEB` | Battery/forecast, on, selected, primary action |
| `yellow` | `#E6C84A` | Overload / warning |
| `red` | `#E25454` | Blocked / fault |

Font: Barlow Semi Condensed SemiBold/Bold via `fontAssets`; use the `type` presets rather than raw font sizes.
