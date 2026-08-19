/**
 * USB CAN Modem — see usb_modem.h for the protocol description.
 *
 * Ported from "CAN MODEM V03.02" (PIC32MX795). Deliberate fixes vs. the
 * original: correctly sized TX frame buffer (the PIC code overflowed a
 * 25-byte array by 5 bytes), the settings reply reports the actual CAN
 * speed, and there is no blocking delay in the TX path — the TWAI driver's
 * 64-deep queue provides the pacing instead.
 */

#include "usb_modem.h"

#include <stdio.h>
#include <string.h>
#include "rtc_pcf85063.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_cntl_struct.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tinyusb_cdc_acm.h"
#include "tinyusb_console.h"

static const char *TAG = "usb_modem";

// The shared LPS/BMS bus always runs 125 kbps; 'S''W' requests for anything
// else are refused (the original dongle had the bus to itself).
#define MODEM_CAN_SPEED       125000UL

#define MODEM_FRAME_LEN       30      // 'M' + 8 id + 16 data + 4 crc + CR
#define MODEM_CRC_SPAN        25      // CRC covers 'M' + id + data
#define MODEM_RX_BUF_LEN      256     // protocol reassembly buffer

// Baudrate-selected mode (mirrors the PIC32 mySetLineCodingHandler):
// >= 64000 selects CAN mode, anything below is off (the 10..63999
// Single-Wire range of the original dongle has no hardware here).
#define BAUD_CAN_MIN          64000

// No cached port state: DTR and baudrate are read LIVE from the TinyUSB
// stack (tud_cdc_n_connected / tud_cdc_n_get_line_coding) whenever needed.
// Cached event-driven flags proved unreliable — a single missed line-state
// event left the connection status (and the dashboard icon) stuck.
static volatile bool s_boot_armed = false;   // 'BT' seen — reboot on port close
static bool s_installed = false;             // TinyUSB stack installed
static usb_modem_can_tx_cb_t s_can_tx_cb = NULL;
static uint32_t s_tx_dropped = 0;

// Protocol reassembly (only touched from the TinyUSB task)
static uint8_t s_rx_buf[MODEM_RX_BUF_LEN];
static size_t  s_rx_len = 0;

// The modem port carries CAN traffic when the host has selected a baudrate
// in the CAN range (115200 = classic "CP mode"; the bus itself is fixed at
// 125 kbps). Windows sets the line coding as part of every port open.
static bool cdc0_can_baud(void)
{
    cdc_line_coding_t lc;
    tud_cdc_n_get_line_coding(TINYUSB_CDC_ACM_0, &lc);
    return lc.bit_rate >= BAUD_CAN_MIN;
}

// ---------------------------------------------------------------------------
//  CRC16-CCITT (poly 0x1021, init 0) — same table as the PIC32 original
// ---------------------------------------------------------------------------
static const uint16_t crc16tab[256] = {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
    0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
    0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
    0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
    0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
    0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
    0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
    0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
    0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
    0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
    0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
    0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
    0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

static uint16_t crc16(const uint8_t *buf, size_t len)
{
    uint16_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc = (uint16_t)((crc << 8) ^ crc16tab[((crc >> 8) ^ buf[i]) & 0xFF]);
    }
    return crc;
}

// ---------------------------------------------------------------------------
//  Hex helpers
// ---------------------------------------------------------------------------
static const char hex_chars[] = "0123456789ABCDEF";

static void byte_to_hex(uint8_t v, uint8_t *out)
{
    out[0] = (uint8_t)hex_chars[v >> 4];
    out[1] = (uint8_t)hex_chars[v & 0x0F];
}

// Returns -1 on a non-hex character (the original indexed a table out of
// bounds instead)
static int hex_to_byte(const uint8_t *in)
{
    int v = 0;
    for (int i = 0; i < 2; i++) {
        uint8_t c = in[i];
        v <<= 4;
        if (c >= '0' && c <= '9')      v |= c - '0';
        else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
        else return -1;
    }
    return v;
}

// ---------------------------------------------------------------------------
//  CDC0 TX — CAN frame / settings reply encoding
// ---------------------------------------------------------------------------
void usb_modem_send_can_frame(uint32_t can_id, const uint8_t *data, uint8_t dlc)
{
    if (!usb_modem_active()) return;

    uint8_t frame[MODEM_FRAME_LEN];
    uint8_t bytes[8] = {0};
    if (data && dlc > 0) memcpy(bytes, data, dlc > 8 ? 8 : dlc);

    frame[0] = 'M';
    byte_to_hex((uint8_t)(can_id >> 24), &frame[1]);
    byte_to_hex((uint8_t)(can_id >> 16), &frame[3]);
    byte_to_hex((uint8_t)(can_id >> 8),  &frame[5]);
    byte_to_hex((uint8_t)(can_id),       &frame[7]);
    for (int i = 0; i < 8; i++) {
        byte_to_hex(bytes[i], &frame[9 + i * 2]);
    }
    uint16_t crc = crc16(frame, MODEM_CRC_SPAN);
    byte_to_hex((uint8_t)(crc >> 8), &frame[25]);
    byte_to_hex((uint8_t)(crc),      &frame[27]);
    frame[29] = 0x0D;

    size_t queued = tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, frame, sizeof(frame));
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
    if (queued != sizeof(frame)) {
        if ((++s_tx_dropped % 1024) == 1) {
            ESP_LOGW(TAG, "USB TX buffer full, dropped=%lu", (unsigned long)s_tx_dropped);
        }
    }
}

// Settings reply: 'S' + 8 hex speed + 4 hex (status 0x02, 0x00) + 4 hex CRC + CR
static void send_setting_reply(void)
{
    uint8_t frame[18];
    frame[0] = 'S';
    byte_to_hex((uint8_t)(MODEM_CAN_SPEED >> 24), &frame[1]);
    byte_to_hex((uint8_t)(MODEM_CAN_SPEED >> 16), &frame[3]);
    byte_to_hex((uint8_t)(MODEM_CAN_SPEED >> 8),  &frame[5]);
    byte_to_hex((uint8_t)(MODEM_CAN_SPEED),       &frame[7]);
    byte_to_hex(2, &frame[9]);
    byte_to_hex(0, &frame[11]);
    uint16_t crc = crc16(frame, 13);
    byte_to_hex((uint8_t)(crc >> 8), &frame[13]);
    byte_to_hex((uint8_t)(crc),      &frame[15]);
    frame[17] = 0x0D;

    tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, frame, sizeof(frame));
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
}

// ---------------------------------------------------------------------------
//  CDC0 RX — protocol parser (runs in the TinyUSB task)
// ---------------------------------------------------------------------------

// Decode and forward one CRC-valid 'M' frame. Returns false on bad CRC/hex.
static bool parse_m_frame(const uint8_t *f)
{
    int crc_hi = hex_to_byte(&f[25]);
    int crc_lo = hex_to_byte(&f[27]);
    if (crc_hi < 0 || crc_lo < 0) return false;
    if (((crc_hi << 8) | crc_lo) != crc16(f, MODEM_CRC_SPAN)) return false;

    uint32_t can_id = 0;
    uint8_t data[8];
    for (int i = 0; i < 4; i++) {
        int b = hex_to_byte(&f[1 + i * 2]);
        if (b < 0) return false;
        can_id = (can_id << 8) | (uint32_t)b;
    }
    for (int i = 0; i < 8; i++) {
        int b = hex_to_byte(&f[9 + i * 2]);
        if (b < 0) return false;
        data[i] = (uint8_t)b;
    }

    if (s_can_tx_cb) s_can_tx_cb(can_id, data, 8);
    return true;
}

// Handle an 'S' settings record at buf[0]; returns consumed byte count,
// or 0 if more data is needed.
static size_t parse_s_frame(const uint8_t *buf, size_t avail)
{
    if (avail < 2) return 0;

    if (buf[1] == 'R') {
        send_setting_reply();
        return 2;
    }
    if (buf[1] == 'W') {
        if (avail < 10) return 0;
        uint32_t speed = 0;
        for (int i = 0; i < 4; i++) {
            int b = hex_to_byte(&buf[2 + i * 2]);
            if (b < 0) return 1;   // malformed — skip the 'S' and resync
            speed = (speed << 8) | (uint32_t)b;
        }
        if (speed != MODEM_CAN_SPEED) {
            ESP_LOGW(TAG, "SW request for %lu bps ignored — shared bus runs %lu",
                     (unsigned long)speed, (unsigned long)MODEM_CAN_SPEED);
        }
        return 10;
    }
    return 1;   // unknown subcommand — skip the 'S' and resync
}

static void parser_feed(const uint8_t *data, size_t len)
{
    // Append to the reassembly buffer (drop oldest data on overflow —
    // only happens if the host sends garbage)
    if (len > sizeof(s_rx_buf)) { data += len - sizeof(s_rx_buf); len = sizeof(s_rx_buf); }
    if (s_rx_len + len > sizeof(s_rx_buf)) {
        size_t keep = sizeof(s_rx_buf) - len;
        memmove(s_rx_buf, &s_rx_buf[s_rx_len - keep], keep);
        s_rx_len = keep;
    }
    memcpy(&s_rx_buf[s_rx_len], data, len);
    s_rx_len += len;

    // Scan for records
    size_t pos = 0;
    while (pos < s_rx_len) {
        uint8_t c = s_rx_buf[pos];
        if (c == 'M') {
            if (s_rx_len - pos < MODEM_FRAME_LEN) break;   // incomplete — wait
            if (parse_m_frame(&s_rx_buf[pos])) {
                pos += MODEM_FRAME_LEN;
            } else {
                pos++;   // bad CRC — resync on next byte
            }
        } else if (c == 'S') {
            size_t consumed = parse_s_frame(&s_rx_buf[pos], s_rx_len - pos);
            if (consumed == 0) break;                      // incomplete — wait
            pos += consumed;
        } else {
            pos++;   // noise (CR/LF between frames etc.)
        }
    }

    // Keep the unconsumed tail
    if (pos > 0) {
        memmove(s_rx_buf, &s_rx_buf[pos], s_rx_len - pos);
        s_rx_len -= pos;
    }
}

static void cdc_rx_cb(int itf, cdcacm_event_t *event)
{
    (void)event;
    uint8_t buf[256];
    size_t rx_size = 0;
    while (tinyusb_cdcacm_read(itf, buf, sizeof(buf), &rx_size) == ESP_OK && rx_size > 0) {
        if (usb_modem_active()) {
            parser_feed(buf, rx_size);
        }
        rx_size = 0;
    }
}

// ---------------------------------------------------------------------------
//  Mode selection via baudrate + port state
// ---------------------------------------------------------------------------

// Bootloader touch: 'BT' on the DEBUG port (console_rx_cb) ARMS a reboot
// into ROM download mode; it fires when that port closes again, with a 3 s
// fallback in case the host never closes it. Rebooting while the host still
// has the port open leaves Windows with a dead, stale enumeration — hence
// the arm/close dance.
static void download_mode_reboot(void *arg)
{
    (void)arg;
    // Detach cleanly so the host drops the CDC ports before we vanish.
    tud_disconnect();
    esp_rom_delay_us(100 * 1000);

    // Route the internal USB PHY back to USB-Serial-JTAG. The app switched
    // it to the OTG controller via RTC-domain bits that SURVIVE esp_restart
    // (only a power-on reset clears them — which is why the BOOT+RESET
    // buttons always worked while a plain software reboot left the ROM's
    // download mode with no usable USB). Clearing the bits restores the
    // power-on default, so the ROM enumerates exactly like after BOOT+RESET.
    RTCCNTL.usb_conf.sw_usb_phy_sel = 0;
    RTCCNTL.usb_conf.sw_hw_usb_phy_sel = 0;

    REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
    esp_restart();
}

static void schedule_download_reboot(uint32_t delay_ms)
{
    static esp_timer_handle_t reboot_timer = NULL;
    if (!reboot_timer) {
        const esp_timer_create_args_t args = {
            .callback = download_mode_reboot,
            .name = "dl_reboot",
        };
        esp_timer_create(&args, &reboot_timer);
    }
    if (reboot_timer) {
        esp_timer_stop(reboot_timer);
        esp_timer_start_once(reboot_timer, (uint64_t)delay_ms * 1000);
    }
}

// Line-state callbacks are used only for EDGES (parser reset on close, the
// BT reboot trigger) and logging — connection status is always read live
// from the stack, never cached here.
static void cdc_line_state_cb(int itf, cdcacm_event_t *event)
{
    (void)itf;
    bool dtr = event->line_state_changed_data.dtr;
    if (!dtr) s_rx_len = 0;
    ESP_LOGI(TAG, "Modem port %s", dtr ? "opened (DTR)" : "closed");
}

static void console_line_state_cb(int itf, cdcacm_event_t *event)
{
    (void)itf;
    if (!event->line_state_changed_data.dtr && s_boot_armed) {
        ESP_LOGW(TAG, "BT command — rebooting into download mode");
        s_boot_armed = false;
        schedule_download_reboot(100);
    }
}

// Debug-port text commands (CR/LF-terminated lines). They live on the
// console port — not the modem port — so they can never collide with the
// CAN tool's protocol traffic:
//   BT                  arm a reboot into ROM download mode; fires when the
//                       port closes (3 s fallback). Used by flash.ps1.
//   TS YYYYMMDDHHMMSS   set the RTC + system clock (space optional).
static void console_handle_line(const char *line, size_t len)
{
    if (len >= 2 && line[0] == 'B' && line[1] == 'T') {
        ESP_LOGW(TAG, "BT command armed — download mode on port close");
        s_boot_armed = true;
        schedule_download_reboot(3000);
        return;
    }
    if (len >= 2 && line[0] == 'T' && line[1] == 'S') {
        const char *p = line + 2;
        while (*p == ' ') p++;
        int y, mo, d, h, mi, s;
        if (sscanf(p, "%4d%2d%2d%2d%2d%2d", &y, &mo, &d, &h, &mi, &s) == 6 &&
            rtc_pcf85063_set_datetime(y, mo, d, h, mi, s) == ESP_OK) {
            ESP_LOGI(TAG, "TS ok");
        } else {
            ESP_LOGW(TAG, "TS failed — expected TS YYYYMMDDHHMMSS");
        }
        return;
    }
}

static void console_rx_cb(int itf, cdcacm_event_t *event)
{
    (void)event;
    static char line[32];
    static size_t line_len = 0;
    uint8_t buf[64];
    size_t rx_size = 0;
    while (tinyusb_cdcacm_read(itf, buf, sizeof(buf), &rx_size) == ESP_OK && rx_size > 0) {
        for (size_t i = 0; i < rx_size; i++) {
            char c = (char)buf[i];
            if (c == '\r' || c == '\n') {
                if (line_len > 0) {
                    line[line_len] = '\0';
                    console_handle_line(line, line_len);
                }
                line_len = 0;
            } else if (line_len < sizeof(line) - 1) {
                line[line_len++] = c;
            }
        }
        rx_size = 0;
    }
}

// Live state, read straight from the TinyUSB stack every call:
// tud_cdc_n_connected() is the actual DTR bit, tud_cdc_n_get_line_coding()
// the actual baudrate the host last set. DTR is REQUIRED — PC programs must
// set DtrEnable=true (harmless with the old PIC32 dongle too).
bool usb_modem_active(void)
{
    return usb_modem_host_present() &&
           tud_cdc_n_connected(TINYUSB_CDC_ACM_0) &&
           cdc0_can_baud();
}

bool usb_modem_host_present(void)
{
    return s_installed && tud_mounted() && !tud_suspended();
}

bool usb_modem_console_open(void)
{
    return usb_modem_host_present() && tud_cdc_n_connected(TINYUSB_CDC_ACM_1);
}

void usb_modem_set_can_tx_callback(usb_modem_can_tx_cb_t cb)
{
    s_can_tx_cb = cb;
}

// ---------------------------------------------------------------------------
//  Init / standby
// ---------------------------------------------------------------------------
esp_err_t usb_modem_init(void)
{
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    esp_err_t err = tinyusb_driver_install(&tusb_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "tinyusb_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    // CDC0 — modem protocol
    tinyusb_config_cdcacm_t acm0 = {
        .cdc_port = TINYUSB_CDC_ACM_0,
        .callback_rx = cdc_rx_cb,
        .callback_line_state_changed = cdc_line_state_cb,
    };
    err = tinyusb_cdcacm_init(&acm0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CDC0 init failed: %s", esp_err_to_name(err));
        return err;
    }

    // CDC1 — console/logs (replaces the USB-Serial-JTAG console, which is
    // dead once the OTG controller owns the PHY)
    tinyusb_config_cdcacm_t acm1 = {
        .cdc_port = TINYUSB_CDC_ACM_1,
        .callback_rx = console_rx_cb,
        .callback_line_state_changed = console_line_state_cb,
    };
    err = tinyusb_cdcacm_init(&acm1);
    if (err == ESP_OK) {
        tinyusb_console_init(TINYUSB_CDC_ACM_1);
        // The console VFS may claim CDC1's RX callback for stdin — take it
        // back. We never read stdin, and the BT command must be seen.
        tinyusb_cdcacm_register_callback(TINYUSB_CDC_ACM_1, CDC_EVENT_RX,
                                         console_rx_cb);
        tinyusb_cdcacm_register_callback(TINYUSB_CDC_ACM_1,
                                         CDC_EVENT_LINE_STATE_CHANGED,
                                         console_line_state_cb);
    } else {
        ESP_LOGW(TAG, "CDC1 init failed (%s) — logs stay on default console",
                 esp_err_to_name(err));
    }

    s_installed = true;
    ESP_LOGI(TAG, "USB CAN modem ready (CDC0=modem, CDC1=console)");
    return ESP_OK;
}

void usb_modem_standby(bool standby)
{
    if (!s_installed) return;

    if (standby) {
        // Soft-detach only (release the D+ pull-up): the host sees a clean
        // disconnect and drops the COM ports, but the TinyUSB driver stays
        // installed. A full uninstall/reinstall cycle is NOT used — the
        // reinstall panics inside esp_tinyusb (crashed every wake, which is
        // why the display cold-booted instead of showing the wake splash).
        // With no session active, light sleep has nothing left to corrupt.
        tud_disconnect();
        s_boot_armed = false;
        s_rx_len = 0;
        ESP_LOGI(TAG, "USB detached for standby");
    } else {
        // Re-attach: the host re-enumerates the same device.
        tud_connect();
        ESP_LOGI(TAG, "USB re-attached");
    }
}
