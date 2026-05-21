# ESP32 CAN HMI Firmware

ESP-IDF firmware for the Clayton Power LPS touchscreen dashboard.  
**Path**: `ESP32-S3-Touch-LCD-5-Demo/ESP-IDF/09_CAN_HMI/`

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [File Structure](#2-file-structure)
3. [Build & Flash](#3-build--flash)
4. [CAN Bus Protocol](#4-can-bus-protocol)
5. [Data Model](#5-data-model)
6. [Settings System](#6-settings-system)
7. [LVGL UI Pages](#7-lvgl-ui-pages)
8. [BLE Protocol](#8-ble-protocol)
9. [Power Management](#9-power-management)
10. [Multi-Device Support](#10-multi-device-support)
11. [Error Handling](#11-error-handling)
12. [Display & Touch Initialization](#12-display--touch-initialization)

---

## 1. Project Overview

The HMI firmware runs as an ESP-IDF application on the ESP32-S3.  
It connects to Clayton Power LPS / BMS units via CAN bus, displays live telemetry on a 5" touchscreen, allows the user to read and change settings locally, and acts as a raw CAN-over-BLE gateway for the companion mobile app.

**Key responsibilities:**
- CAN bus message decoding (J1939 broadcast + CAN_Extra query/response)
- LVGL-based touchscreen UI (dashboard, settings, errors)
- BLE GATT server for raw CAN passthrough to the mobile app
- Power management (sleep on idle, wake on touch or CAN activity)
- Multi-device cascade support (up to 8 LPS/BMS units)

---

## 2. File Structure

```
09_CAN_HMI/
├── CMakeLists.txt            # Root build file (requires ESP-IDF component path)
├── sdkconfig.defaults        # Pre-configured build settings (flash, PSRAM, BLE, etc.)
├── partitions.csv            # Custom partition table (if present)
└── main/
    ├── CMakeLists.txt        # Sources list
    ├── Kconfig.projbuild     # Menuconfig definitions (bounce buffer height, etc.)
    ├── can_hmi.c             # Main application (~3500 lines)
    ├── ble_gateway.c         # NimBLE GATT server and raw CAN notification transport
    ├── ble_gateway.h         # BLE API and raw gateway message/command IDs
    ├── waveshare_rgb_lcd_port.c  # LCD panel init, I2C CH422G helpers
    ├── waveshare_rgb_lcd_port.h  # Pin definitions, timing constants
    ├── lvgl_port.c           # LVGL display + touch driver integration
    └── lvgl_port.h           # LVGL configuration constants
```

### Key sdkconfig.defaults Settings

| Setting | Value | Purpose |
|---------|-------|---------|
| `CONFIG_ESPTOOLPY_FLASHFREQ_120M` | `y` | 120 MHz Flash |
| `CONFIG_SPI_FLASH_HPM_ENA` | `y` | High-Performance Mode for Flash |
| `CONFIG_SPIRAM_SPEED_120M` | `y` | 120 MHz PSRAM (Octal) |
| `CONFIG_COMPILER_OPTIMIZATION_PERF` | `y` | -O2 optimization |
| `CONFIG_EXAMPLE_LVGL_PORT_AVOID_TEAR_ENABLE` | `y` | Tear-free double-buffer |
| `CONFIG_EXAMPLE_LVGL_PORT_TASK_CORE` | `1` | LVGL on core 1 |
| `CONFIG_BT_ENABLED` / `CONFIG_BT_NIMBLE_ENABLED` | `y` | NimBLE BLE stack |
| `CONFIG_PM_ENABLE` / `CONFIG_FREERTOS_USE_TICKLESS_IDLE` | `y` | Light sleep power management |

---

## 3. Build & Flash

### Prerequisites

- ESP-IDF v6.0 installed at `C:\esp\v6.0\esp-idf`

### Build Steps

```powershell
# 1. Load ESP-IDF environment
. "C:\esp\v6.0\esp-idf\export.ps1"

# 2. Navigate to project
cd "ESP32-S3-Touch-LCD-5-Demo\ESP-IDF\09_CAN_HMI"

# 3. Build
idf.py build

# 4. Flash (device on COM11)
idf.py -p COM11 flash

# 5. Monitor serial output
idf.py -p COM11 monitor

# Or combined:
idf.py -p COM11 flash monitor
```

> **Note**: First build will take several minutes to compile all components.  
> Subsequent builds only recompile changed files.

### Menuconfig

```powershell
idf.py menuconfig
```

Notable options:
- `Example Configuration → LCD RGB Bounce Buffer Height` — default 20 (higher = less tearing)

---

## 4. CAN Bus Protocol

### Physical Layer

| Parameter | Value |
|-----------|-------|
| Standard | J1939 (29-bit extended CAN ID) |
| Speed | 250 kbps |
| TX GPIO | 15 |
| RX GPIO | 16 |
| Peripheral | ESP32-S3 TWAI |

### CAN ID Structure (J1939 / CAN_Extra)

| CAN ID Pattern | Direction | Description |
|----------------|-----------|-------------|
| `0x18FF` / `0x14FF` / `0x19FF` | LPS → HMI | Broadcast telemetry |
| `0x19EF[target][source]` / `0x15EF[target][source]` | HMI ↔ LPS | CAN_Extra query/response |
| `0x18EA` | HMI → LPS | J1939 Request PGN (identification) |

### Broadcast PGN Messages (LPS → HMI)

Each broadcast message has an 8-byte payload. The first byte is the PGN (sub-index), the rest carry the data.

| PGN | Fields |
|-----|--------|
| `0x00` | SOC %, battery current, time remaining, DOD |
| `0x01` | DC input voltage & current, DC output voltage & current |
| `0x03` | Operating state, failure level, cell count, sensor count |
| `0x04` | Inverter state/fail, charger state/fail, DC-in state/fail, DC-out state/fail |
| `0x05` | 8 error codes (bulk) |
| `0x06` | 3 internal temperatures + cell average temperature |
| `0x09` | AC input voltage & current, AC output voltage & current |
| `0x10` | 4 cell voltages |
| `0x20` | Solar current, solar state, solar failure |
| `0x22` | AC input power, AC output power |

### CAN_Extra Commands (HMI → LPS, `0x19EF`)

| Command ID | Name | Payload | Description |
|------------|------|---------|-------------|
| `0x40` | GET_VAL | `[block, id]` | Query current value of a setting |
| `0x41` | SET_VAL | `[block, id, value_q1616_le]` | Set setting value |
| `0x42` | GET_DEF | `[block, id]` | Query default value |
| `0x43` | GET_MIN | `[block, id]` | Query minimum allowed value |
| `0x44` | GET_MAX | `[block, id]` | Query maximum allowed value |

### Data Scaling

Settings use Q16.16 fixed-point format (32-bit, lower 16 bits = fractional part):

| Type | Conversion |
|------|-----------|
| Voltage | `raw_i32 / 65536.0` → volts |
| Current | `raw_i32 / 65536.0` → amps |
| Percentage | `(raw_i32 / 65536.0 / 655.35) * 100` |
| Time | `raw_i32 / 65536.0 * 3600` → seconds |
| Enum | Upper 16 bits contain enum index |

---

## 5. Data Model

### `lps_data_t` — Live Telemetry

Primary data structure populated from CAN broadcast messages:

```c
typedef struct {
    float    soc_percent;          // State of charge 0–100%
    float    battery_voltage_v;    // Battery bank voltage
    float    battery_current_a;    // Current (+charging, -discharging)
    uint16_t battery_dod_ah;       // Depth of discharge in Ah
    int16_t  soc_time_min;         // Minutes to full/empty (-1 = unknown)

    float    dc_in_voltage;        // DC input (alternator/charger input)
    float    dc_in_current;
    float    dc_out_voltage;       // DC output (load)
    float    dc_out_current;

    float    ac_in_voltage;        // Shore power / generator
    float    ac_in_current;
    uint16_t ac_in_power;          // Watts
    float    ac_out_voltage;       // Inverter output
    float    ac_out_current;
    uint16_t ac_out_power;

    float    solar_current;        // Solar/PV charging current

    float    cell_voltage[4];      // Individual cell voltages
    float    temp[3];              // Internal temperature sensors
    float    temp_cell_avg;        // Average battery cell temperature

    int8_t   operating_state;      // -1=error, 0=off, 1=on, ...
    uint8_t  failure_level;        // 0=OK, 1=warning, 2=fault, 4=critical
    int8_t   inverter_state;       // -1=error, 0=off, 1=on, 2=standby, 3=charge, 4=float
    uint8_t  inverter_fail;
    int8_t   charger_state;
    uint8_t  charger_fail;
    int8_t   dc_in_state;
    uint8_t  dc_in_fail;
    int8_t   dc_out_state;
    uint8_t  dc_out_fail;
    int8_t   solar_state;
    uint8_t  solar_fail;

    uint8_t  error_count;
    uint8_t  error_codes[8];       // Active error codes
} lps_data_t;
```

---

## 6. Settings System

### Architecture

Settings are accessed via CAN_Extra commands. The HMI sends GET_VAL/SET_VAL with a `block:id` address. Responses are matched asynchronously.

A **staggered request queue** prevents CAN bus flooding: 25 ms spacing between GET_VAL requests when loading a settings page.

### Settings Menu Structure

```
LPS Settings
├── AC Output  (Block 50)
│   ├── 50:0  Inverter Cutoff Voltage
│   ├── 50:1  Auto Off Delay
│   └── 50:2  Auto Off Load Current
├── AC Input  (Block 60)
│   └── 60:2  Max Charge Current
├── DC Output  (Block 40)
│   ├── 40:0  Shutdown Delay
│   ├── 40:1  Saver Time
│   └── 40:2  Saver Current
├── DC Input  (Block 30)
│   ├── 30:1  Operating Voltage  [ENUM]
│   ├── 30:7  Charge Current
│   ├── 30:12 Start Voltage
│   └── 30:13 Stop Voltage
├── Starter Battery  (Block 31)
│   ├── 31:0  Enable
│   ├── 31:1  Charge Current
│   ├── 31:2  Charge Voltage
│   ├── 31:3  Cut-Off Current
│   ├── 31:4  Maintenance Voltage
│   └── 31:10 Cut-Off Timer
├── Solar  (Block 70)
│   └── 70:0  Operation  [ENUM: Off / Auto / On]
└── General
    ├── 1:1   Jumpstart Timer
    └── 7:0   Config Select  [ENUM: None / Extension]

BMS Settings
├── Battery  (Block 10)
│   ├── 10:0  Battery Capacity (Ah)
│   └── 10:1  DOD Capacity (Ah)
├── Status    (read-only telemetry)
└── Temperature  (read-only sensor data)
```

### `setting_t` Structure

Each setting entry defines:
- Block and ID (CAN_Extra address)
- Display name and unit string
- Min/max value range
- Data type (float, enum, bool)
- Format function (how to display the raw Q16.16 value)

---

## 7. LVGL UI Pages

### Page 1: Dashboard

Main screen shown after startup.

**Elements:**
- **SOC Arc**: Large circular arc gauge (0–100%), color coded (green/orange/red by level)
- **Battery Info**: Voltage, current (colored: green=charging, orange=discharging), DOD, time remaining
- **Device Selector**: Tap to switch between discovered units (LPS/BMS)
- **Error Badge**: Red indicator with count of active errors
- **Failure Level Indicator**: Color overlay for warning/fault/critical states

### Page 2: Settings Grid

Shown when user taps the settings button.

**Elements:**
- Category tiles in a grid: icon + category name
- Categories change based on selected unit type (LPS categories vs BMS categories)
- Tap a tile → navigate to Settings Detail for that category

### Page 3: Settings Detail

Shows info rows and editable settings for a category.

**Elements:**
- Read-only info rows (e.g., current operating state)
- Editable setting rows (current value + +/- controls or enum picker)
- Settings are fetched on page open (staggered GET_VAL queue)
- Saving sends SET_VAL via CAN_Extra, followed by GET_VAL confirmation

### Page 4: Errors

Full error code list.

**Elements:**
- Scrollable list of active error codes
- Each entry: error code number, description text, severity color
- Error descriptions from static lookup table (128 entries)

### Page 5: Error Popup

Modal overlay for critical/important errors.

**Behavior:**
- `POP_AUTO`: Dismissed automatically when error clears
- `POP_KEEP`: Requires user to manually dismiss
- `POP_HIDE`: Never auto-popup (silently logged only)

### BLE PIN Screen

Displays the 6-digit pairing PIN when a phone initiates BLE bonding.  
PIN is randomly generated at startup and shown until bonding completes.

---

## 8. BLE Protocol

See [MobileApp.md](MobileApp.md) for the app side. The firmware side is in `ble_gateway.c`.

### GATT Service

| UUID | Description |
|------|-------------|
| `00001000-0000-1000-8000-00805f9b34fb` | Clayton Power Service |
| `00001001-0000-1000-8000-00805f9b34fb` | TX Characteristic (Notify — ESP32 → Phone) |
| `00001002-0000-1000-8000-00805f9b34fb` | RX Characteristic (Write — Phone → ESP32) |

**Device advertised name**: `Clayton Power`

**Security**: Display-only IO capability, bonding + MITM + Secure Connections, 6-digit passkey displayed on LCD.

**MTU**: Negotiated to 256 bytes.

### Messages ESP32 -> Phone (TX, Notifications)

| ID | Name | Description |
|----|------|-------------|
| `0x08` | `BLE_MSG_CAN_FRAME` | Raw 29-bit CAN frame: `[can_id_u32_le][dlc][data8]` |

### Commands Phone -> ESP32 (RX, Write)

| ID | Name | Payload | Description |
|----|------|---------|-------------|
| `0x18` | `BLE_CMD_SET_CAN_PASSTHROUGH` | `[enabled]` | Enable or disable CAN forwarding |
| `0x19` | `BLE_CMD_SEND_CAN_FRAME` | `[can_id_u32_le][dlc][data8]` | Send one CAN frame |
| `0x1A` | `BLE_CMD_SEND_CAN_FRAMES` | `[count][can_id_u32_le][dlc][data8]...` | Send a batch of CAN frames |

When passthrough is enabled, every received CAN frame is forwarded to the app as `BLE_MSG_CAN_FRAME`. The firmware still decodes CAN frames for the local LCD UI, but it no longer emits ESP32-generated dashboard/settings/unit/error BLE packets.

---

## 9. Power Management

The firmware implements a complete sleep/wake cycle to reduce power consumption when idle.

### State Machine

```
PWR_ACTIVE ─────────────────────────────────────┐
  │ 5 s idle (no touch, no significant CAN change) │ Touch / CAN activity
  ▼                                               │
PWR_SLEEP_SPLASH  (2 s "Going to sleep" splash)  │
  │                                               │
  ▼                                               │
PWR_SLEEPING  ◄─────────────────────────────────►┘
  (display off, CPU 80 MHz, TWAI stopped)
  │ Touch or CAN activity
  ▼
PWR_WAKE_SPLASH  (2 s "Waking up" splash)
  │
  ▼
PWR_DASHBOARD  ──(30 s no touch)──► PWR_RESLEEP_SPLASH ──► PWR_SLEEPING
```

### Sleep Sequence (in order)

1. Turn off backlight via I2C CH422G
2. Suspend LVGL task (stops DMA transfer and rendering)
3. Wait 120 ms for ST7262 charge pump discharge
4. Float all 16 RGB data GPIO pins
5. Assert LCD_RST (CH422G IO3 LOW)
6. Set CH422G outputs to safe state (keeps CTP_RST HIGH for GT911)
7. Stop TWAI (releases power management lock)
8. Drop CPU frequency: 240 MHz → 80 MHz (BLE requires APB clock ≥ 80 MHz)

### Wake Sequence (in order)

1. Restore CPU to 240 MHz
2. Start TWAI
3. Re-initialize CH422G
4. Release LCD_RST
5. Restore RGB GPIO pins as output
6. Restart LCD DMA
7. Resume LVGL task
8. Turn on backlight

### Touch Detection During Sleep

In sleep mode, LVGL is suspended so the normal touch polling is inactive.  
The main task directly polls the GT911 touch controller via I2C (register `0x814E`) to detect a touch event that triggers wake.

---

## 10. Multi-Device Support

The HMI supports up to **8 CAN bus units** simultaneously (LPS and/or BMS units in a cascade configuration).

### `unit_entry_t` Structure

Each discovered unit tracks:
- CAN address (1-byte J1939 source address)
- Unit type (LPS or BMS)
- Part number and serial number (from identification PGN)
- Live `lps_data_t` data
- Online status

### Device Lifecycle

| Event | Timeout | Action |
|-------|---------|--------|
| Broadcast received | — | Mark unit ONLINE, update data |
| No broadcast | 5 seconds | Mark unit OFFLINE (grayed out in UI) |
| No broadcast | 60 seconds | FREE slot (remove from table) |

### Device Identification

On first appearance, the HMI sends a J1939 Request PGN (`0x18EA`) to the unit.  
The unit responds with part number, serial, firmware version.  
If no response within timeout, the HMI retries automatically.

### Active Unit Selection

The currently selected unit is the source for:
- Dashboard display data
- Settings menu (LPS settings vs BMS settings based on type)

The user can switch units from the device selector widget on the ESP32 dashboard. The mobile app keeps its own selected unit from raw CAN frames and does not change the ESP32's local selection over BLE.

---

## 11. Error Handling

### Error Code Table

128 static error definitions, each with:
- Error number (used as the identifier)
- Description text
- Severity level

| Range | Category |
|-------|----------|
| E001–E048 | EEPROM, voltage, sensor, calibration issues |
| E050–E062 | Battery (discharge, low, cell balancing, temperature) |
| E070–E075 | Solar voltage/current |
| E088–E097 | DC input/output failures |
| E101–E127 | AC offset, system, CAN bus, extension failures |
| E130–E142 | Power supply issues |
| E150–E157 | AC overload, inverter cutoff |
| E200–E247 | AC input, charger, locked states |

### Failure Levels

| Value | Name | Color | Description |
|-------|------|-------|-------------|
| 0 | `FL_OK` | Green | No fault |
| 1 | `FL_WARNING` | Orange | Warning, system continues |
| 2 | `FL_SIMPLE_FAILURE` | Red | Fault, degraded operation |
| 4 | `FL_CRITICAL_FAILURE` | Red | Critical, system may stop |

### Popup Behavior

| Mode | Behavior |
|------|---------|
| `POP_AUTO` | Popup shown automatically; dismissed when error clears |
| `POP_KEEP` | Popup shown; requires manual user dismissal |
| `POP_HIDE` | Not shown as popup; only visible in error list page |

---

## 12. Display & Touch Initialization

### LCD (ST7262, RGB interface)

The ST7262 is a **pure RGB timing controller** — it has no SPI/I2C configuration interface.  
All display quality is determined by the RGB timing parameters set in the ESP32 LCD driver.

**Official timing values for Waveshare 5B (1024×600):**

```c
.hsync_pulse_width = 24,
.hsync_back_porch  = 160,
.hsync_front_porch = 160,
.vsync_pulse_width = 2,
.vsync_back_porch  = 23,
.vsync_front_porch = 12,
.pclk_hz           = 21000000,   // 21 MHz pixel clock
.pclk_active_neg   = 1,
```

### I2C IO Expander (CH422G)

The CH422G expander manages LCD reset, backlight, and CTP reset signals.

| I2C Address | Register | Function |
|-------------|----------|---------|
| `0x24` | config | Mode configuration |
| `0x38` | data | Output pin states |

**Common write values:**

| Value | Effect |
|-------|--------|
| `0x1E` (to 0x38) | Backlight ON |
| `0x1A` (to 0x38) | Backlight OFF |
| `0x12` (to 0x38) | LCD_RST asserted (LOW) |
| `0x1A` (to 0x38) | LCD_RST released (HIGH) |

**I2C bus**: GPIO 8 (SDA), GPIO 9 (SCL)

### Touch Controller (GT911)

- I2C address: `0x5D`
- Sleep detection register: `0x814E`
- Managed by LVGL GT911 driver in normal operation
- Polled directly (bypassing LVGL) during sleep mode for wake detection
