/**
 * BLE Gateway — CAN-over-BLE bridge for Clayton Power LPS/BMS.
 *
 * Exposes a GATT service with:
 *  - TX characteristic (Notify): ESP32 → Phone
 *  - RX characteristic (Write):  Phone → ESP32
 *
 * Binary protocol over BLE for raw CAN gateway traffic.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
//  BLE Protocol — Message Types
// ---------------------------------------------------------------------------

// ESP32 → Phone (notifications on TX characteristic)
#define BLE_MSG_CAN_FRAME        0x08  // Raw CAN frame notification [can_id_u32_le][dlc][data8]

// Phone → ESP32 (writes to RX characteristic)
#define BLE_CMD_SET_CAN_PASSTHROUGH 0x18  // [enabled:0/1]
#define BLE_CMD_SEND_CAN_FRAME      0x19  // [can_id_u32_le][dlc][data8]
#define BLE_CMD_SEND_CAN_FRAMES     0x1A  // [count][can_id_u32_le][dlc][data8]...

// ---------------------------------------------------------------------------
//  Public API
// ---------------------------------------------------------------------------

/**
 * Initialize BLE GATT server.  Call once from app_main after NVS init.
 * Returns the 6-digit passkey displayed on the LCD for pairing.
 */
uint32_t ble_gateway_init(void);

/**
 * Send raw CAN frame to app.
 * [type=0x08][can_id_u32_le][dlc][data(8)]
 */
void ble_gateway_send_can_frame(uint32_t can_id, const uint8_t *data, uint8_t dlc);

/**
 * Returns true when a BLE client is connected and subscribed to notifications.
 */
bool ble_gateway_is_connected(void);

/**
 * Get the current passkey for display on LCD.
 */
uint32_t ble_gateway_get_passkey(void);

// ---------------------------------------------------------------------------
//  Callback — called from BLE RX to be handled by can_hmi
// ---------------------------------------------------------------------------
typedef void (*ble_cmd_callback_t)(uint8_t cmd, const uint8_t *payload, uint16_t len);
void ble_gateway_set_cmd_callback(ble_cmd_callback_t cb);

typedef void (*ble_pairing_callback_t)(uint32_t passkey);
void ble_gateway_set_pairing_callback(ble_pairing_callback_t cb);

#ifdef __cplusplus
}
#endif
