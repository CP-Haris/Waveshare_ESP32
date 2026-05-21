# CAN Gateway Architecture

The mobile app treats the ESP32 as a CAN-over-BLE gateway. The ESP32 owns the BLE radio and CAN controller, but it should not be the application protocol endpoint for dashboard or settings data.

## BLE transport

The app uses the existing BLE service and only relies on the gateway commands:

- `0x18 SET_CAN_PASSTHROUGH` enables CAN forwarding.
- `0x19 SEND_CAN_FRAME` sends one 29-bit CAN frame from the app to the bus.
- `0x1A SEND_CAN_FRAMES` sends a batch of CAN frames, mainly used by the bootloader.
- `0x08 CAN_FRAME` notifications carry raw CAN frames from the bus back to the app.

Legacy high-level BLE helpers such as `GET_DASHBOARD`, `GET_SETTING`, `SET_SETTING`, `GET_UNITS`, and `GET_ERRORS` have been removed from the mobile app protocol layer. Dashboard, settings, unit discovery, and errors are handled by the app-side CAN parser.

## Dashboard and errors

Dashboard data is decoded from normal Clayton/J1939 CAN broadcasts:

- LPS frames use `0x18FF/0x19FF/0x14FF` with the message byte in bits 8-15.
- BMS frames use the same broadcast pattern with a BMS-specific field layout.
- Failure codes are read from broadcast message `0x05` and exposed to both Dashboard and Settings.
- The Dashboard maps active failure codes through the same error table used by the ESP32 display.
- Clearing errors uses CAN_Extra `SET_VAL` on block `1`, id `252`, value `1234`.

The app keeps the active unit as global app-side state exposed by the header unit switcher. Selecting a unit only changes which locally parsed unit snapshot Dashboard and Settings use; it does not send a BLE command to the ESP32.

## Unit discovery

Units are discovered from source addresses seen on CAN frames. The app requests identification over CAN with `0x18EAFFFE`, then decodes `0x19FF00`, `0x19FF03`, and `0x19FF04` responses for serial number and part number.

Part numbers classify units:

- `CL...` is treated as LPS.
- `CB...` is treated as BMS.

## Settings

Settings use CAN_Extra over peer-to-peer CAN IDs:

- CAN ID: `0x19EF[target][FE]`
- Payload: `[0x00, command, block, id, value_i32_le]`

Commands:

- `0x40 GET_VAL`
- `0x41 SET_VAL`
- `0x43 GET_MIN`
- `0x44 GET_MAX`

Responses are decoded from `0x19EF` or `0x15EF` frames whose source address matches the selected unit. Range responses are combined in the app from the matching min and max replies.

Settings follows the global active unit. When the active unit changes, pending setting reads/writes and local editor state are cleared before the new unit's values are requested.

The inverter and DC output switches are ordinary setting writes:

- Inverter: block `0`, id `158`
- DC output: block `0`, id `159`
- Values are Q16.16, so off is `0` and on is `65536`.

## ESP32 behavior

When passthrough is enabled, the ESP32 forwards all received CAN frames to BLE. It may still decode frames for its local display, but it no longer emits ESP32-generated dashboard/settings/unit/error BLE messages for the mobile app.