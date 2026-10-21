# Clayton Power HMI workspace

This repository holds several related but separate projects. Read this map before touching
anything, and check which project a file belongs to.

## Projects

| Folder | Status | What it is | Language / tools |
|---|---|---|---|
| `ClaytonDisplay/` | **Active** | ESP32-S3 5" touch display (HMI) on the CAN bus. Shows the Carbon Blue dashboard and acts as a CAN-over-BLE gateway for the app. | C, ESP-IDF 6, LVGL 8, NimBLE |
| `ClaytonPowerApp/` | **Active** | The phone app (Android first, iOS). Talks to LPS/BMS units over BLE. | React Native 0.81, Expo 54, react-native-ble-plx, react-native-svg |
| `Docs/` | **Active** | System documentation (index: `Docs/README.md`). | Markdown, PDFs |
| `LPS2 Display Firmware/` | **Reference only** | Firmware of the display MCU *inside* an LPS2 (PIC32, MPLAB X). Source of truth for the LPS2 BLE protocol. Do not modify. | C, MPLAB X / Harmony |
| `Email And Conversations/` | Notes | Correspondence; currently empty. | — |

"LPS2 display" (the small built-in screen of an LPS2) and "ClaytonDisplay" (our ESP32
product) are different devices. Never mix them up.

## How a phone reaches a unit — two paths

```
Path A: via ClaytonDisplay (many units, CAN)
  Phone ─BLE─ ClaytonDisplay (ESP32) ─CAN 125 kbps─ LPS / BMS units (one or many)

Path B: LPS2 built-in BLE (exactly one unit, no CAN)
  Phone ─BLE─ BLE module (DA14531) ─UART─ LPS2 display MCU ─UART─ LPS2 control board
```

| | Path A — ClaytonDisplay | Path B — LPS2 internal BLE |
|---|---|---|
| GATT service | `00001000-0000-1000-8000-00805f9b34fb` | `0783b03e-8535-b5a0-7140-a304d2495cb7` |
| Payload | Raw CAN frames (J1939 broadcasts + CAN_Extra get/set) | SOH/DLE/EOT frames with CRC16, `[cmd, block, id, value32]` |
| Units visible | Every LPS/BMS on the bus | Only the LPS2 itself |
| Settings | Yes (CAN_Extra) | Yes, via the LPS2 display MCU (needs display firmware with both fixes in `Docs/LPS2-Display-Fix-BLE-Settings.md`) |
| Firmware update | Yes (CAN bootloader) | **No** |
| Protocol doc | `ClaytonPowerApp/docs/CAN-Gateway.md`, `Docs/ESP32-Firmware.md` | `Docs/LPS2-BLE-Protocol.md` |

Both paths address values the same way (`block` + `id`, Q16.16 values, same error codes), so
the app shares its data model, settings definitions and error table between them.

## ClaytonPowerApp layout

```
src/ble/bleLink.js            BLE transport: scan both chip types, connect, PIN pairing, notify in / write out.
src/ble/gattProfiles.js       GATT UUIDs per chip, chip detection from the scan, LPS2 advert parsing.
src/devices/deviceSession.js  The only object screens use. Picks the driver for the connected chip,
                              forwards calls and messages, exposes `capabilities`.
src/devices/dashboardModel.js Snapshot shape every driver emits (normalized functions, FAULT).
src/devices/display/          Path A driver (CAN via ClaytonDisplay) + CAN bootloader (firmwareUpdate*.js).
src/devices/lps2/             Path B driver (lps2Device.js) + framing/CRC (lps2Frame.js).
src/screens/, src/components/ Carbon Blue UI. Never import a driver or bleLink directly from here.
src/utils/                    theme, error table, unit classification.
src/services/backgroundService.js  Remembers the last unit, auto-reconnects (Android autoConnect),
                              runs the foreground service when "Background notifications" is on.
src/services/alertMonitor.js  Dashboard snapshots -> notifications (errors, low battery, full).
src/services/notifier.js, prefs.js  expo-notifications wrapper; AsyncStorage preferences.
modules/ble-foreground/       Local Expo module (Android/Kotlin): connectedDevice foreground service
                              that keeps the JS BLE link alive in the background. Native change =
                              new EAS dev build.
```

Capabilities: `multiUnit` (unit picker), `settings`, `firmwareUpdate` (Update tab). A screen
that needs to differ between chips checks a capability, never `kind`. A new chip = a new
driver implementing the interface documented at the top of `deviceSession.js`.

## Design

Carbon Blue is the visual language for both ClaytonDisplay and the app. Specs:
`ClaytonDisplay/docs/Carbon Blue Designspec Display.pdf` and
`ClaytonPowerApp/docs/Carbon Blue App Designspecs.pdf`. Tokens live in
`ClaytonDisplay/main/dashboard_carbon.c` (`CB_*`) and `ClaytonPowerApp/src/utils/theme.js`
and must stay identical.

## Conventions

- Unit names: **LPS 3000**, **Battery (G4)**. Never write "Zeliox" (a competitor's product).
- LPS battery current is positive while charging, negative while discharging.
- The app UI is in English; conversation with Haris is in Danish.
- Commit on a feature branch, not directly on `main`.
- Files in `LPS2 Display Firmware/` are reference only — read, never modify.
- The old Clayton Power GO app has been removed on purpose. Do not use it (or its background
  service) as a model; everything needed from it is already in the app and in `Docs/`.

## Commands

```powershell
# ClaytonDisplay
. "C:\esp\v6.0\esp-idf\export.ps1"; cd ClaytonDisplay; idf.py build
# flashing: see ClaytonDisplay/flash.ps1

# ClaytonPowerApp
cd ClaytonPowerApp; npx expo start --dev-client       # Metro for the dev build
npx eas-cli build --profile development --platform android   # needed after native dependency changes
```
