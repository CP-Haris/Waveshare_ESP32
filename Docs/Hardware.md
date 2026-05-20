# Hardware Reference

Physical hardware configuration for the Clayton Power ESP32-S3 HMI system.

---

## Table of Contents

1. [Board Identification](#1-board-identification)
2. [ESP32-S3 GPIO Map](#2-esp32-s3-gpio-map)
3. [LCD Timing Configuration](#3-lcd-timing-configuration)
4. [I2C Peripherals](#4-i2c-peripherals)
5. [CAN Bus Wiring](#5-can-bus-wiring)
6. [Power Supply](#6-power-supply)
7. [Known Hardware Issues](#7-known-hardware-issues)

---

## 1. Board Identification

| Item | Value |
|------|-------|
| Manufacturer | Waveshare |
| Model | ESP32-S3-Touch-LCD-5B |
| SKU | 28292 |
| Display size | 5 inch |
| Resolution | 1024 × 600 |
| Display interface | Parallel RGB (16-bit) |
| Display driver IC | ST7262 |
| Touch controller | GT911 (capacitive, I2C) |
| IO Expander | CH422G (I2C) |
| SoC | ESP32-S3 (dual-core Xtensa LX7, 240 MHz) |
| Flash | 16 MB (120 MHz, Octal HPM) |
| PSRAM | 8 MB Octal (120 MHz) |
| USB | USB-C (USB-JTAG + USB-Serial) |

---

## 2. ESP32-S3 GPIO Map

### RGB LCD (16-bit parallel, ST7262)

| Signal | GPIO |
|--------|------|
| R0 | 1 |
| R1 | 2 |
| R2 | 3 |
| R3 | 4 |
| R4 | 5 |
| G0 | 6 |
| G1 | 7 |
| G2 | 10 |
| G3 | 11 |
| G4 | 12 |
| G5 | 13 |
| B0 | 14 |
| B1 | 17 |
| B2 | 18 |
| B3 | 19 |
| B4 | 20 |
| PCLK | 21 |
| HSYNC | 46 |
| VSYNC | 47 |
| DE | 48 |

> **Note**: All LCD GPIOs are floated during sleep to prevent current leakage through the ST7262 charge pump.

### I2C Bus

| Signal | GPIO |
|--------|------|
| SDA | 8 |
| SCL | 9 |

Shared by: CH422G IO expander (0x24/0x38) and GT911 touch controller (0x5D)

### CAN Bus (TWAI)

| Signal | GPIO |
|--------|------|
| TX | 15 |
| RX | 16 |

### Other

| Signal | GPIO | Note |
|--------|------|------|
| LCD BACKLIGHT | Controlled via CH422G | Not a direct GPIO |
| LCD RESET | Controlled via CH422G | Not a direct GPIO |
| CTP RESET | Controlled via CH422G | Touch controller reset |

---

## 3. LCD Timing Configuration

The ST7262 is a **pure RGB timing controller** — it has no SPI/I2C configuration registers.  
All display parameters are configured in the ESP32 RGB panel driver at init time.

### Official Timing for Waveshare 5B (1024×600)

```c
.h_res            = 1024,
.v_res            = 600,
.pclk_hz          = 21 * 1000 * 1000,   // 21 MHz pixel clock
.hsync_pulse_width = 24,
.hsync_back_porch  = 160,
.hsync_front_porch = 160,
.vsync_pulse_width = 2,
.vsync_back_porch  = 23,
.vsync_front_porch = 12,
.pclk_active_neg   = 1,                  // PCLK active on falling edge
.data_width        = 16,                 // 16-bit parallel RGB565
```

### Framebuffer Configuration

| Parameter | Value |
|-----------|-------|
| Framebuffers | 2–3 (in PSRAM) |
| Bounce buffer height | 20 lines (configurable via menuconfig) |
| Tear prevention | Double-buffer + LVGL direct mode (mode 3) |
| LVGL task core | Core 1 |

---

## 4. I2C Peripherals

### CH422G IO Expander

| Address | Function |
|---------|---------|
| `0x24` | Write mode configuration |
| `0x38` | Write output pin states |

**Output pin assignments:**

| Bit | Signal | Sleep state |
|-----|--------|-------------|
| IO0 | LCD_RST (active LOW) | LOW (asserted during sleep) |
| IO1 | CTP_RST (active LOW) | HIGH (GT911 kept running) |
| IO2 | LCD_BL | LOW (backlight off) |
| IO3 | (additional control) | — |

**Common write values to address 0x38:**

| Hex | Effect |
|-----|--------|
| `0x1E` | Backlight ON, LCD_RST released |
| `0x1A` | Backlight OFF, LCD_RST released |
| `0x12` | Backlight OFF, LCD_RST asserted |
| `0x00` | All outputs low (hard reset state) |

### GT911 Capacitive Touch Controller

| Property | Value |
|----------|-------|
| I2C address | `0x5D` |
| Max touch points | 5 |
| Interface | I2C |
| Resolution | Tracks LCD resolution (1024×600) |
| Sleep detection | Poll register `0x814E` directly during ESP32 sleep |

---

## 5. CAN Bus Wiring

### Physical Layer

| Parameter | Value |
|-----------|-------|
| Protocol | J1939 (ISO 11898, 29-bit extended IDs) |
| Baud rate | 250 kbps |
| Termination | 120 Ω at each end of the bus |
| Connector | Typically DB9 or bare wire to LPS unit |

### Pinout (DB9 CAN connector, standard J1939)

| DB9 Pin | Signal |
|---------|--------|
| 2 | CAN_L |
| 7 | CAN_H |
| 3 / 6 | GND |

### ESP32 TWAI Peripheral

The ESP32-S3 TWAI peripheral does **not** include a physical CAN transceiver.  
An external transceiver (e.g., SN65HVD230 or MCP2551) is required between GPIO 15/16 and the CAN bus differential pair.

| ESP32 GPIO | Transceiver Pin | CAN Bus |
|------------|----------------|---------|
| GPIO 15 (TX) | TXD | — |
| GPIO 16 (RX) | RXD | — |
| — | CAN_H | CAN_H line |
| — | CAN_L | CAN_L line |

---

## 6. Power Supply

| Rail | Nominal | Source |
|------|---------|--------|
| Board input | 5 V (USB-C) or external 5 V | USB or JST connector |
| ESP32-S3 core | 3.3 V (onboard LDO) | Regulated from 5 V |
| LCD panel | 3.3 V / backlight from onboard boost | Via CH422G BL control |
| PSRAM / Flash | 3.3 V | Onboard |

---

## 7. Known Hardware Issues

### LCD Washed-Out / Pale Border Display

**Status**: Unresolved (as of April 2025)

**Symptoms**: The LCD image appears washed out, with a pale/faded border region — as if VCOM is set incorrectly or there is a brightness gradient from edge to center.

**Investigation performed:**
- Timing parameters updated to official Waveshare 5B values ✅
- Bounce buffer increased to 20 lines ✅
- PSRAM/Flash upgraded to 120 MHz ✅
- `LV_COLOR_SCREEN_TRANSP` removed ✅
- ST7262 confirmed: no software VCOM/contrast control (pure RGB, no register interface)
- Pre-built Waveshare demo comparison: inconclusive (black screen — ESP-IDF bootloader version mismatch)

**Likely cause**: Hardware-level VCOM calibration on the LCD panel (factory trim), or a power supply variation affecting the panel's backlight/TFT voltage.

**Next steps to investigate:**
- Compare side-by-side with a different Waveshare 5B board (if available)
- Try varying pixel clock (18–25 MHz range)
- Check if reducing `hsync_back_porch` / `hsync_front_porch` changes the appearance
- Build official Waveshare demo using ESP-IDF 5.x (matching their build environment) for a clean comparison

### Waveshare Demo Black Screen (ESP-IDF 6.0 Bootloader)

Pre-built Waveshare demo binary (`08_lvgl_Porting`) shows black screen when flashed alongside the ESP-IDF v6.0 bootloader.  
**Root cause**: The demo was compiled with ESP-IDF 5.2; the bootloader format changed between 5.x and 6.0.  
**Workaround**: Would need to build the 08_lvgl_Porting project from source with ESP-IDF 5.2.
