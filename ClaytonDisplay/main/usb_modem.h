/**
 * USB CAN Modem — virtual COM port CAN gateway (TinyUSB CDC-ACM).
 *
 * Reimplements the protocol of the PIC32 "CAN MODEM V03.02" dongle so the
 * existing Clayton Power PC tools work unchanged against the display:
 *
 *   CDC0 = modem protocol   CDC1 = console/log output
 *
 * Protocol (ASCII, CR-terminated):
 *   CAN->PC : 'M' + 8 hex (CAN id) + 16 hex (8 data bytes) + 4 hex CRC16 + CR
 *   PC->CAN : same 'M' frame, CRC16-validated before transmit
 *   'S''W' + 8 hex speed : set CAN speed (only 125000 accepted — shared bus)
 *   'S''R'               : read setting (replies 'S' frame with CRC)
 *
 * The DEBUG/console port (CDC1) additionally accepts the text command 'BT':
 * it arms a reboot into ROM download mode, executed when the port closes.
 * Used by flash.ps1; deliberately NOT on the modem port, so it can never
 * collide with the CAN tool's protocol traffic.
 *
 * Mode is selected by the baudrate the PC sets on the COM port (like the
 * original dongle): >= 64000 = CAN mode, anything below = off. The
 * original's Single-Wire UART mode (baud 10..63999) is not supported —
 * the display has no single-wire hardware.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Install TinyUSB (dual CDC) and start the modem. Call once from
 *        app_main. Takes over the USB-C port — USB-Serial-JTAG stops working;
 *        console output is redirected to CDC1.
 */
esp_err_t usb_modem_init(void);

/**
 * @brief True while the modem should forward CAN traffic: a host has the
 *        COM port open with DTR asserted and a CAN-range baudrate selected.
 *        DTR is REQUIRED — PC programs must set DtrEnable=true. Read live
 *        from the TinyUSB stack (nothing cached, nothing to get stuck).
 */
bool usb_modem_active(void);

/**
 * @brief True while a USB host is attached and the bus is not suspended.
 *        Used to block display standby so the port stays responsive.
 */
bool usb_modem_host_present(void);

/**
 * @brief True while the console/debug COM port (CDC1) is open on the host
 *        (DTR). Only used for the dashboard USB status icon.
 */
bool usb_modem_console_open(void);

/**
 * @brief Standby: uninstall the USB stack completely (true) or bring it back
 *        (false). Light sleep corrupts an active USB-OTG session, so standby
 *        must detach cleanly — the port disappears from the PC and
 *        re-enumerates on wake.
 */
void usb_modem_standby(bool standby);

/**
 * @brief Encode a received CAN frame as an 'M' record and queue it on CDC0.
 *        Non-blocking; drops the frame if the USB TX buffer is full.
 */
void usb_modem_send_can_frame(uint32_t can_id, const uint8_t *data, uint8_t dlc);

/**
 * @brief Callback invoked (from the TinyUSB task) for each CRC-valid 'M'
 *        frame received from the PC. The callee transmits it on the CAN bus.
 */
typedef void (*usb_modem_can_tx_cb_t)(uint32_t can_id, const uint8_t *data, uint8_t dlc);
void usb_modem_set_can_tx_callback(usb_modem_can_tx_cb_t cb);

#ifdef __cplusplus
}
#endif
