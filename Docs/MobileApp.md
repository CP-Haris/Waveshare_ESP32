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
│   ├── screens/
│   │   ├── ConnectScreen.js      # BLE scan & connection UI
│   │   ├── DashboardScreen.js    # Live telemetry display
│   │   └── SettingsScreen.js     # Units, errors, diagnostics
│   ├── components/
│   │   ├── DataCard.js           # Reusable metric tile
│   │   ├── SocArc.js             # Circular SOC ring
│   │   └── StatusBadge.js        # Component status indicator
│   ├── services/
│   │   └── bleService.js         # BLE singleton (scan, connect, notify)
│   └── utils/
│       ├── protocol.js           # Binary encode/decode
│       └── theme.js              # Dark theme colors & spacing
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
| `react-native-svg` | — | SocArc circular chart |

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

Bottom Tab Navigator with 3 tabs:

| Tab | Icon | Screen | Initial Route |
|-----|------|--------|---------------|
| Dashboard | 📊 | `DashboardScreen` | — |
| Settings | ⚙️ | `SettingsScreen` | — |
| Connect | 📡 | `ConnectScreen` | ✓ (initial) |

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

**Data source**: Ensures CAN passthrough is enabled, decodes live CAN broadcasts in `canGatewayService`, and refreshes the visible snapshot every **2 seconds**. It does not request an ESP32-generated dashboard packet.

**Layout (scrollable, top to bottom):**

| Section | Content |
|---------|---------|
| Header | Unit type (LPS/BMS) + green/red connection dot |
| SOC Ring | Large circular arc (0–100%), time-to-full/empty below |
| Battery | Voltage, current (green=charging / orange=discharging), DOD Ah |
| Cells | 4× cell voltages |
| DC Power | DC In voltage+current, DC Out voltage+current |
| AC Power | AC In watts, AC Out watts (hidden if zero/unavailable) |
| Solar | Solar current (hidden if unavailable, gold color) |
| Temperature | Up to 3 internal sensors + cell average |
| System Status | 5 `StatusBadge` components: Inverter, Charger, DC-In, DC-Out, Solar |
| Quick Controls | ⚡ Inverter toggle, 🔌 DC Output toggle |
| Errors | Active error codes list (red, hidden when no errors) |

**State machine:**
- State names: Error (−1), Off (0), On (1), Standby (2), Charge (3), Float (4)

---

### Screen: Settings (`src/screens/SettingsScreen.js`)

Device management and diagnostics.

**Sections:**

| Section | Content |
|---------|---------|
| Discovered Units | All cascade units found from CAN frames, tap to select locally in the app |
| Error Codes | Active error list, green card when clear |
| About | Protocol version, service UUID, app version |

**Manual refresh buttons:**
- "Refresh Units" sends CAN identification requests through the gateway.
- "Refresh Errors" reads the latest failure-code broadcast data already decoded by the app.

Each unit shows: type (LPS/BMS), part number, serial number, index, CAN address (hex).

---

## 6. BLE Service

### Configuration (`src/services/bleService.js`)

Library: `react-native-ble-plx`

| Item | Value |
|------|-------|
| Service UUID | `00001000-0000-1000-8000-00805f9b34fb` |
| TX Char (Notify) | `00001001-0000-1000-8000-00805f9b34fb` |
| RX Char (Write) | `00001002-0000-1000-8000-00805f9b34fb` |
| MTU | 256 bytes |
| Write type | WriteWithoutResponse |

> **Note on naming**: "TX" from the firmware perspective notifies the phone. "RX" from the firmware perspective receives writes from the phone. The bleService naming follows the app's perspective (TX = data to app, RX = commands from app).

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
`decodeNotification()` only decodes raw CAN frame notifications. App-level Dashboard and Settings events are emitted by `canGatewayService` after it parses those CAN frames.

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

### Command Encoding

```javascript
export function encodeSetCanPassthrough(enabled) { ... }
export function encodeSendCanFrame(canId, dataBytes) { ... }
export function encodeSendCanFrames(frames) { ... }
```

---

## 8. CAN Gateway Parsing

Dashboard and Settings are decoded from CAN frames in `src/services/canGatewayService.js`.

- Broadcast telemetry uses Clayton/J1939 `0x18FF`, `0x19FF`, and `0x14FF` frames.
- Unit discovery sends `0x18EAFFFE` requests and decodes identification responses.
- Settings use CAN_Extra frames on `0x19EF[target][FE]`.
- Firmware update uses the same raw gateway transport and batches CAN frames with command `0x1A`.

---

## 9. Components

### `DataCard` (`src/components/DataCard.js`)

Reusable metric tile.

| Prop | Type | Description |
|------|------|-------------|
| `title` | string | Label (displayed uppercase, gray) |
| `value` | string/number | Main value (large, colored) |
| `unit` | string | Unit suffix (small, gray) |
| `color` | string | Value text color (from theme) |
| `small` | bool | Compact variant for denser layouts |

---

### `SocArc` (`src/components/SocArc.js`)

Circular arc progress ring for SOC display.

| Prop | Type | Description |
|------|------|-------------|
| `percent` | number | 0–100 |
| `size` | number | Diameter in pixels |
| `strokeWidth` | number | Arc thickness |
| `color` | string | Arc color (green/orange/red by level) |

Rendered with `react-native-svg` arcs.

---

### `StatusBadge` (`src/components/StatusBadge.js`)

Subsystem status indicator (inverter, charger, DC-in, DC-out, solar).

| Prop | Type | Description |
|------|------|-------------|
| `label` | string | Subsystem name |
| `state` | int8 | −1=Error, 0=Off, 1=On, 2=Standby, 3=Charge, 4=Float |
| `fail` | uint8 | Non-zero = fail code present |

**Rendering logic:**
- `fail > 0` → red dot + red "Error" text
- `state === 1` → green dot + "On"
- `state === 0` → gray dot + "Off"
- `state === 2` → blue dot + "Standby"
- `state === 3` → yellow dot + "Charge"
- `state === 4` → teal dot + "Float"

---

## 10. Theme

Dark theme defined in `src/utils/theme.js` (GitHub-inspired dark colors):

| Token | Value | Use |
|-------|-------|-----|
| `background` | `#0d1117` | Screen background |
| `surface` | `#161b22` | Card/tile background |
| `border` | `#30363d` | Card borders |
| `text` | `#c9d1d9` | Primary text |
| `textMuted` | `#8b949e` | Secondary / label text |
| `green` | `#3fb950` | On, charging, OK |
| `orange` | `#d29922` | Warning, discharging |
| `red` | `#f85149` | Error, critical |
| `blue` | `#58a6ff` | Accent, standby |
| `gold` | `#e3b341` | Solar |
| `teal` | `#39d353` | Float state |
