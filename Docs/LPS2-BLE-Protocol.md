# LPS2 Internal BLE — Protocol

How a phone talks to an LPS2 through its **built-in** BLE module (no ClaytonDisplay involved).
Derived from the LPS2 display firmware (`LPS2 Display Firmware/firmware/src/UART_App.c`,
`Values.c`, `BLE_App.c`), its protocol document
(`firmware/Documentation/Display Uart Communication Protocol - Aktiv - Kopi.docx`) and the
previous Clayton Power GO app.

## Topology

```
Phone ──BLE── BLE module ──UART2 (115200)── LPS2 display MCU (PIC32) ──UART1── LPS2 control board
```

- The BLE module is a transparent pipe. Every BLE write/notify carries the same framed bytes
  that travel on UART2.
- The **display MCU** is the router: it answers some requests itself, forwards others to the
  control board, and forwards selected control-board traffic back to BLE.
- Exactly one LPS is ever reachable. No CAN, no unit discovery, **no bootloading** over BLE.

## GATT

| Item | UUID |
|---|---|
| Service (also the scan filter) | `0783b03e-8535-b5a0-7140-a304d2495cb7` |
| Notify (LPS → phone) | `0783b03e-8535-b5a0-7140-a304d2495cb8` |
| Write without response (phone → LPS) | `0783b03e-8535-b5a0-7140-a304d2495cba` |

Bonding is required (passkey shown on the LPS display, `Block 200 / ID 102`).

**Advertising — manufacturer data:** bytes 2..5 = serial number (uint32 LE, shown as
`XXXXXX-XXXX` by splitting off the last 4 digits), bytes 6..9 = SoC (Q16.16 fraction, ×100 = %).
Placeholder patterns `aabbccdd` / `10203040` mean "not known yet". The display keeps feeding
serial (`0:220`) and SoC (`0:119`) to the module even when no phone is connected, so the
scan list can show SoC before connecting.

## Framing

```
<SOH 0x01> <payload…> <CRCL> <CRCH> <EOT 0x04>
```

- Any payload or CRC byte equal to `0x01`, `0x04` or `0x10` is preceded by `DLE 0x10`.
- CRC-16-CCITT (XMODEM: poly `0x1021`, init `0x0000`) over the unescaped payload, sent low byte first.
- A notification may contain a partial frame or several frames — parse as a byte stream.
- The display drops frames longer than 20 bytes.

## Values

Every value is addressed by `block` + `id` (the same numbering as CAN_Extra) and is a signed
32-bit Q16.16 (`raw / 65536`). States are Q16 enums:

| Kind | Values |
|---|---|
| Operating state (`0:200..203`) | `0x00000` off, `0x10000` wakeup, `0x20000` ready, `0x30000` starting, `0x40000` stopping, `0x50000` on, `0xFFFF0000` disabled |
| Failure level (`0:204..207`, `0:209`) | `0x00000` ok, `0x10000` warning, `0x20000` simple failure, `0x30000` empty, `0x40000` critical |
| Battery status (`0:210`) | `0x10000` idle, `0x20000` discharge, `0x30000` charge, `0x40000` full, `0x50000` balancing, `0xFFFF0000` empty |

Dashboard values (all `Access_BLE`):

| Block:ID | Meaning | Block:ID | Meaning |
|---|---|---|---|
| 0:119 | Battery SoC (fraction) | 0:104 | AC in W |
| 0:127 | Battery W | 0:110 | DC in W |
| 0:100 / 0:101 | Battery V / A | 0:79 | Solar W |
| 0:120 | Remaining time (hours) | 0:107 | AC out W |
| 0:210 | Battery status | 0:113 | DC out W |
| 0:220 | Serial | 0:200..0:209 | Op/fail states per function |

## Commands (phone → LPS)

| Payload | Meaning | Handled by |
|---|---|---|
| `20 bb ii` | Read value | Forwarded to control board; reply `20 bb ii v0 v1 v2 v3` comes back on notify |
| `22 bb ii` | Read min/max | Answered by the display from its cache, or fetched from the control board (display fix 2) |
| `24 bb ii` | Read default | Forwarded; reply `24 bb ii v32` |
| `50 bb ii v0..v3` | Write setting (SET_VAL) | Forwarded to the control board as SET_VAL (display fix 1) |
| `80 00 CB` | Toggle DC out (12 V) | Display decides on/off from `0:203` and sends the switch command |
| `80 00 C9` | Toggle AC out (230 V) | Display decides on/off from `0:201` |
| `60` | Clear latched errors | Forwarded |
| `41` | Ask for the error buffer | Forwarded |

The 4 value bytes after `80 00 CB/C9` are ignored — it is always a **toggle**.

## Messages (LPS → phone)

| Payload | Meaning |
|---|---|
| `20 bb ii v32` | A value (answer to a read, or forwarded from the display's own polling) |
| `22 bb ii min32 max32` | Min/max |
| `41 e1..e8` | Error buffer: 8 error codes, 0 = empty slot. Sent every second while errors exist. Codes are the same table as `ClaytonPowerApp/src/utils/errorCodes.js` |

**Do not rely on unsolicited data.** The display only polls what its own screen shows
(operating/failure states always; watts only on the main view). The app must poll the
dashboard values itself with `20 bb ii`.

## Display firmware requirement

Writing settings (`0x50`) and reading min/max (`0x22`) need the LPS2 display firmware with
both fixes from `LPS2-Display-Fix-BLE-Settings.md` (verified on the bench 2026-10-02). Older
display firmware keeps written values in its own RAM only and never answers `0x22`.

## Open points

1. **Battery sign.** Confirm the sign of `0:127` / `0:101` while charging. The app takes the
   direction from `0:210`, so this only matters for raw-value displays.
