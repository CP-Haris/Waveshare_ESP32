/**
 * Clayton Power LPS - CAN HMI Dashboard + Settings
 * ESP32-S3-Touch-LCD-5 (1024x600)
 *
 * Modern graphical dashboard with touch settings menu.
 * Communicates via CAN bus using CAN_Extra (0x19EF) protocol for GET/SET.
 *
 * Ported from RP2350-Touch-LCD-4 (480x480) to ESP32-S3 with TWAI CAN driver.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#define CONFIG_TWAI_SUPPRESS_DEPRECATE_WARN 1
#include "driver/twai.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "can_hmi.h"
#include "waveshare_rgb_lcd_port.h"
#include "ble_gateway.h"

static const char *TAG = "can_hmi";

#define BOOT_TX_VERBOSE_PACKET_LIMIT 3
#define BOOT_TX_PROGRESS_INTERVAL 100
static bool ble_can_passthrough_enabled = false;
static volatile bool pwr_force_wake_request = false;
static uint32_t boot_tx_packet_no = 0;
static uint32_t boot_tx_chunk_count = 0;
static uint16_t boot_tx_seen_bytes = 0;
static uint16_t boot_tx_expected_bytes = 0;
static uint8_t boot_tx_cmd = 0;
static uint32_t boot_tx_started_at_ms = 0;

static bool is_boot_diag_can_id(uint32_t can_id)
{
    uint32_t top24 = (can_id >> 8) & 0xFFFFFF;
    uint16_t top16 = (can_id >> 16) & 0xFFFF;

    return (top24 == 0x19FF20) || // bootloader heartbeat/control
           (top24 == 0x19FF21) || // bootloader data
           (top24 == 0x19FF22) || // bootloader response
           (top24 == 0x19FF00) || // serial high
           (top24 == 0x19FF03) || // part number high
           (top24 == 0x19FF04) || // part number low
           (top24 == 0x18FF07) || // serial alt
           (top24 == 0x18EAFF) || // broadcast request
           (top16 == 0x19EF);     // enter-bootloader + CAN_Extra traffic
}

static bool should_log_boot_can_frame(uint32_t can_id)
{
    uint32_t top24 = (can_id >> 8) & 0xFFFFFF;
    return is_boot_diag_can_id(can_id) && top24 != 0x19FF21;
}

static void log_boot_diag_frame(const char *dir, uint32_t can_id, const uint8_t *data, uint8_t len)
{
    uint8_t b0 = (data && len > 0) ? data[0] : 0;
    uint8_t b1 = (data && len > 1) ? data[1] : 0;
    uint8_t b2 = (data && len > 2) ? data[2] : 0;
    uint8_t b3 = (data && len > 3) ? data[3] : 0;
    uint8_t b4 = (data && len > 4) ? data[4] : 0;
    uint8_t b5 = (data && len > 5) ? data[5] : 0;
    uint8_t b6 = (data && len > 6) ? data[6] : 0;
    uint8_t b7 = (data && len > 7) ? data[7] : 0;

    ESP_LOGI(TAG,
             "[BLE-CAN][%s] id=0x%08lX len=%u data=%02X %02X %02X %02X %02X %02X %02X %02X",
             dir,
             (unsigned long)can_id,
             (unsigned)len,
             b0, b1, b2, b3, b4, b5, b6, b7);
}

// ---------------------------------------------------------------------------
//  Platform helpers (ESP32 replacements for Pico SDK)
// ---------------------------------------------------------------------------
static inline uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static void log_twai_status_on_tx_error(uint32_t can_id, esp_err_t err)
{
    twai_status_info_t status = {0};
    esp_err_t status_err = twai_get_status_info(&status);

    if (status_err == ESP_OK) {
        ESP_LOGW(TAG,
                 "[BOOT-TX] TWAI tx failed id=0x%08lX err=%s state=%lu msgs_to_tx=%lu tx_err=%lu tx_failed=%lu bus_err=%lu",
                 (unsigned long)can_id,
                 esp_err_to_name(err),
                 (unsigned long)status.state,
                 (unsigned long)status.msgs_to_tx,
                 (unsigned long)status.tx_error_counter,
                 (unsigned long)status.tx_failed_count,
                 (unsigned long)status.bus_error_count);
    } else {
        ESP_LOGW(TAG, "[BOOT-TX] TWAI tx failed id=0x%08lX err=%s status_err=%s",
                 (unsigned long)can_id,
                 esp_err_to_name(err),
                 esp_err_to_name(status_err));
    }
}

static void boot_tx_diag_update(uint32_t can_id, const uint8_t *data, uint8_t len, esp_err_t tx_result)
{
    uint32_t top24 = (can_id >> 8) & 0xFFFFFF;
    if (top24 != 0x19FF21 || !data || len == 0) return;

    uint8_t count = data[0] > 7 ? 7 : data[0];
    if (count > (uint8_t)(len - 1)) count = len > 0 ? (uint8_t)(len - 1) : 0;
    if (count == 0) return;

    const uint8_t *chunk = &data[1];
    bool starts_packet = chunk[0] == 0xAF;

    if (starts_packet) {
        boot_tx_packet_no++;
        boot_tx_chunk_count = 0;
        boot_tx_seen_bytes = 0;
        boot_tx_expected_bytes = 0;
        boot_tx_cmd = count > 1 ? chunk[1] : 0xFF;
        boot_tx_started_at_ms = now_ms();

        if (count >= 4) {
            uint16_t data_len = (uint16_t)chunk[2] | ((uint16_t)chunk[3] << 8);
            boot_tx_expected_bytes = (uint16_t)(16 + data_len);
        }

        bool log_packet = boot_tx_packet_no <= BOOT_TX_VERBOSE_PACKET_LIMIT ||
                          (boot_tx_packet_no % BOOT_TX_PROGRESS_INTERVAL) == 0;
        if (log_packet) {
            ESP_LOGI(TAG,
                     "[BOOT-TX] start #%lu unit=%lu cmd=0x%02X expected=%u first=%02X %02X %02X %02X %02X %02X %02X",
                     (unsigned long)boot_tx_packet_no,
                     (unsigned long)(can_id & 0xFF),
                     boot_tx_cmd,
                     (unsigned)boot_tx_expected_bytes,
                     data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
        }
    }

    boot_tx_chunk_count++;
    boot_tx_seen_bytes = (uint16_t)(boot_tx_seen_bytes + count);

    if (tx_result != ESP_OK) {
        ESP_LOGW(TAG,
                 "[BOOT-TX] fragment tx error #%lu chunk=%lu cmd=0x%02X seen=%u/%u result=%s",
                 (unsigned long)boot_tx_packet_no,
                 (unsigned long)boot_tx_chunk_count,
                 boot_tx_cmd,
                 (unsigned)boot_tx_seen_bytes,
                 (unsigned)boot_tx_expected_bytes,
                 esp_err_to_name(tx_result));
        return;
    }

    if (boot_tx_expected_bytes > 0 && boot_tx_seen_bytes >= boot_tx_expected_bytes) {
        bool log_packet = boot_tx_packet_no <= BOOT_TX_VERBOSE_PACKET_LIMIT ||
                          (boot_tx_packet_no % BOOT_TX_PROGRESS_INTERVAL) == 0;
        if (log_packet) {
            ESP_LOGI(TAG,
                     "[BOOT-TX] complete #%lu cmd=0x%02X chunks=%lu bytes=%u/%u elapsed=%lums",
                     (unsigned long)boot_tx_packet_no,
                     boot_tx_cmd,
                     (unsigned long)boot_tx_chunk_count,
                     (unsigned)boot_tx_seen_bytes,
                     (unsigned)boot_tx_expected_bytes,
                     (unsigned long)(now_ms() - boot_tx_started_at_ms));
        }
        boot_tx_expected_bytes = 0;
    }
}

// ---------------------------------------------------------------------------
//  CAN Protocol Definitions
// ---------------------------------------------------------------------------
#define CAN_LPS_ADDR      0x03   // Default LPS source address (used as hint only)
#define CAN_DISPLAY_ADDR  0xFE   // Our display address

// Active CAN target address — updated when selected unit changes
static uint8_t can_target_addr = CAN_LPS_ADDR;
static bool    can_target_valid = false;

// CAN_Extra command IDs (sent via 0x19EF)
#define CAN_CMD_GET_VAL   0x40
#define CAN_CMD_SET_VAL   0x41
#define CAN_CMD_GET_DEF   0x42
#define CAN_CMD_GET_MIN   0x43
#define CAN_CMD_GET_MAX   0x44

// Data scaling helpers
#define BYTES_TO_UINT16(h, l)  (((uint16_t)(h) << 8) | (uint16_t)(l))
#define BYTES_TO_SINT16(h, l)  ((int16_t)(((uint16_t)(h) << 8) | (uint16_t)(l)))

// Q16.16 helpers
#define Q16_TO_FLOAT(v) ((float)(v) / 65536.0f)
#define FLOAT_TO_Q16(f) ((int32_t)((f) * 65536.0f))

// ---------------------------------------------------------------------------
//  Buzzer stubs (no buzzer on ESP32-S3-Touch-LCD-5)
// ---------------------------------------------------------------------------
static void buzzer_beep(uint32_t ms)  { (void)ms; }
static void buzzer_click(void)        { }
static void buzzer_alarm(void)        { }

// ---------------------------------------------------------------------------
//  CAN Send / Receive for Settings (CAN_Extra via 0x19EF)
// ---------------------------------------------------------------------------
static void can_send_raw(uint32_t can_id, uint8_t *data, uint8_t len)
{
    twai_message_t msg = {0};
    msg.identifier = can_id;
    msg.extd = 1;              // J1939 uses 29-bit extended IDs
    msg.data_length_code = len;
    memcpy(msg.data, data, len);
    twai_transmit(&msg, pdMS_TO_TICKS(10));
}

static void can_send_raw_ext(uint32_t can_id, const uint8_t *data, uint8_t len)
{
    twai_message_t msg = {0};
    msg.identifier = can_id;
    msg.extd = 1;
    msg.data_length_code = len > 8 ? 8 : len;
    if (data && msg.data_length_code > 0) {
        memcpy(msg.data, data, msg.data_length_code);
    }

    esp_err_t err = twai_transmit(&msg, pdMS_TO_TICKS(10));
    boot_tx_diag_update(can_id, msg.data, msg.data_length_code, err);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "[BLE-CAN] TX failed id=0x%08lX dlc=%u err=%s",
                 (unsigned long)can_id,
                 (unsigned)msg.data_length_code,
                 esp_err_to_name(err));
        if (is_boot_diag_can_id(can_id)) {
            log_twai_status_on_tx_error(can_id, err);
        }
    }
}

static void can_send_command(uint8_t cmd, uint8_t block, uint8_t id, int32_t value)
{
    if (!can_target_valid) return;  // No unit selected
    // CAN ID: 0x19EF[target][source]
    uint32_t can_id = 0x19EF0000 | ((uint32_t)can_target_addr << 8) | CAN_DISPLAY_ADDR;
    uint8_t data[8] = {0};
    data[0] = 0x00;  // Command marker
    data[1] = cmd;
    data[2] = block;
    data[3] = id;
    // Value in little-endian (bytes 4-7)
    data[4] = (value)       & 0xFF;
    data[5] = (value >> 8)  & 0xFF;
    data[6] = (value >> 16) & 0xFF;
    data[7] = (value >> 24) & 0xFF;
    can_send_raw(can_id, data, 8);
}

static void can_get_value(uint8_t block, uint8_t id)
{
    can_send_command(CAN_CMD_GET_VAL, block, id, 0);
}

static void can_set_value(uint8_t block, uint8_t id, int32_t value)
{
    can_send_command(CAN_CMD_SET_VAL, block, id, value);
}

static void can_get_min(uint8_t block, uint8_t id)
{
    can_send_command(CAN_CMD_GET_MIN, block, id, 0);
}

static void can_get_max(uint8_t block, uint8_t id)
{
    can_send_command(CAN_CMD_GET_MAX, block, id, 0);
}

// ---------------------------------------------------------------------------
//  CAN Request Identification (0x18EA — J1939 Request PGN)
// ---------------------------------------------------------------------------
static void can_request_id(uint8_t target_addr)
{
    uint32_t can_id = 0x18EA0000 | ((uint32_t)target_addr << 8) | CAN_DISPLAY_ADDR;
    uint8_t data[3] = { 0x00, 0xFF, 0x01 };
    can_send_raw(can_id, data, 3);
    ESP_LOGI(TAG, "Requesting identification from 0x%02X", target_addr);
}

// ---------------------------------------------------------------------------
//  Settings definitions (Block/ID from original firmware)
// ---------------------------------------------------------------------------
typedef enum {
    PREFIX_VOLTAGE = 1,
    PREFIX_CURRENT = 2,
    PREFIX_TEMP = 3,
    PREFIX_PROCENT = 4,
    PREFIX_TIME_HHMMSS = 7,
    PREFIX_POWER = 9,
    PREFIX_ENUM = 20,
    PREFIX_BITMAP = 21,
} prefix_t;

// Enum label tables for PREFIX_ENUM settings
static const char *enum_solar_op[]  = {"Off", "Auto", "On"};
static const char *enum_op_volt[]   = {"Auto", "12V", "24V"};
static const char *enum_config[]    = {"None", "Extension"};

typedef struct {
    const char *label;
    uint8_t block;
    uint8_t id;
    uint8_t prefix;
    uint8_t decimals;
    const char *unit;
    int32_t step_slow;
    int32_t current_val;
    int32_t min_val;
    int32_t max_val;
    bool    val_received;
    bool    min_received;
    bool    max_received;
    const char **enum_labels;
    uint8_t enum_count;
} setting_t;

// Menu category info-type identifiers
enum {
    INFO_NONE = 0,
    INFO_AC_OUT, INFO_AC_IN, INFO_DC_OUT, INFO_DC_IN,
    INFO_STARTER, INFO_SOLAR,
    INFO_LPS_STATUS, INFO_LPS_TEMP,
    INFO_BMS_STATUS, INFO_BMS_TEMP,
};

typedef struct {
    const char *title;
    const char *icon;
    setting_t *settings;
    int count;
    uint8_t info_type;
} menu_category_t;

#define MAX_INFO_ROWS 10
typedef struct {
    int count;
} cat_info_t;

// --- AC Output (Block 50) ---
static setting_t settings_ac_out[] = {
    {"Inverter Cutoff",     50, 0, PREFIX_PROCENT,    0, "%",  655,    0, 0, 58982,  false,true,true, NULL,0},
    {"Auto Off Delay",      50, 1, PREFIX_TIME_HHMMSS,0, "",   1092,   0, 0, 655360, false,true,true, NULL,0},
    {"Auto Off Load",       50, 2, PREFIX_POWER,      0, "W",  65536,  0, 655360, 98304000, false,true,true, NULL,0},
};

// --- AC Input (Block 60) ---
static setting_t settings_ac_in[] = {
    {"Max Current",         60, 2, PREFIX_CURRENT,    0, "A",  65536,  0, 262144, 851968, false,true,true, NULL,0},
};

// --- DC Output (Block 40) ---
static setting_t settings_dc_out[] = {
    {"Shutdown Delay",      40, 0, PREFIX_TIME_HHMMSS,0, "",   1092,   0, 0, 655360, false,true,true, NULL,0},
    {"Saver Time",          40, 1, PREFIX_TIME_HHMMSS,0, "",   1092,   0, 0, 655360, false,true,true, NULL,0},
    {"Saver Current",       40, 2, PREFIX_CURRENT,    0, "A",  65536,  0, 0, 11796480, false,true,true, NULL,0},
};

// --- DC Input (Block 30) ---
static setting_t settings_dc_in[] = {
    {"Operating Voltage",   30, 1, PREFIX_ENUM,       0, "",   65536,  0, 0, 131072, false,true,true, enum_op_volt,3},
    {"Charge Current",      30, 7, PREFIX_CURRENT,    0, "A",  65536,  0, 655360, 2949120, false,true,true, NULL,0},
    {"Start Voltage",       30,12, PREFIX_VOLTAGE,    2, "V",  6553,   0, 655360, 983040, false,true,true, NULL,0},
    {"Stop Voltage",        30,13, PREFIX_VOLTAGE,    2, "V",  6553,   0, 655360, 983040, false,true,true, NULL,0},
};

// --- Starter Battery (Block 31) ---
static setting_t settings_starter[] = {
    {"Enable",              31, 0, PREFIX_ENUM,       0, "",   65536,  0, 0, 65536,  false,true,true, NULL,0},
    {"Charge Current",      31, 1, PREFIX_CURRENT,    0, "A",  65536,  0, 0, 2621440, false,true,true, NULL,0},
    {"Charge Voltage",      31, 2, PREFIX_VOLTAGE,    2, "V",  6553,   0, 655360, 983040, false,true,true, NULL,0},
    {"Cut-Off Current",     31, 3, PREFIX_CURRENT,    0, "A",  65536,  0, 0, 2621440, false,true,true, NULL,0},
    {"Maintenance Volt",    31, 4, PREFIX_VOLTAGE,    2, "V",  6553,   0, 655360, 983040, false,true,true, NULL,0},
    {"Cut-Off Timer",       31,10, PREFIX_TIME_HHMMSS,0, "",   65536,  0, 65536, 1572864, false,true,true, NULL,0},
};

// --- Solar (Block 70) ---
static setting_t settings_solar[] = {
    {"Operation",           70, 0, PREFIX_ENUM,       0, "",   65536,  0, 0, 131072, false,true,true, enum_solar_op,3},
};

// --- General ---
static setting_t settings_general[] = {
    {"Jumpstart Timer",      1, 1, PREFIX_TIME_HHMMSS,0, "",   5461,   0, 0, 5461,   false,true,true, NULL,0},
    {"Config Select",        7, 0, PREFIX_ENUM,       0, "",   65536,  0, 0, 65536,  false,true,true, enum_config,2},
};

static menu_category_t menu_categories_lps[] = {
    {"AC Output",       LV_SYMBOL_POWER,    settings_ac_out,    3, INFO_AC_OUT},
    {"AC Input",        LV_SYMBOL_CHARGE,   settings_ac_in,     1, INFO_AC_IN},
    {"DC Output",       LV_SYMBOL_DOWNLOAD, settings_dc_out,    3, INFO_DC_OUT},
    {"DC Input",        LV_SYMBOL_UPLOAD,   settings_dc_in,     4, INFO_DC_IN},
    {"Starter Battery", LV_SYMBOL_BATTERY_3,settings_starter,   6, INFO_STARTER},
    {"Solar",           LV_SYMBOL_IMAGE,    settings_solar,     1, INFO_SOLAR},
    {"General",         LV_SYMBOL_SETTINGS, settings_general,   2, INFO_NONE},
    {"Status",          LV_SYMBOL_EYE_OPEN, NULL,               0, INFO_LPS_STATUS},
    {"Temperature",     LV_SYMBOL_WARNING,  NULL,               0, INFO_LPS_TEMP},
};
#define NUM_CATEGORIES_LPS (sizeof(menu_categories_lps) / sizeof(menu_categories_lps[0]))

// --- BMS Settings (Block 10 = Battery) ---
static setting_t settings_bms_battery[] = {
    {"Battery Capacity",    10, 0, PREFIX_CURRENT,    0, "Ah", 65536,  0, 655360, 13107200, false,false,false, NULL,0},
    {"DOD Capacity",        10, 1, PREFIX_PROCENT,    0, "%",  655,    0, 0, 65535,  false,false,false, NULL,0},
};

static menu_category_t menu_categories_bms[] = {
    {"Battery",         LV_SYMBOL_BATTERY_FULL, settings_bms_battery,  2, INFO_NONE},
    {"Status",          LV_SYMBOL_EYE_OPEN,     NULL,                  0, INFO_BMS_STATUS},
    {"Temperature",     LV_SYMBOL_WARNING,      NULL,                  0, INFO_BMS_TEMP},
};
#define NUM_CATEGORIES_BMS (sizeof(menu_categories_bms) / sizeof(menu_categories_bms[0]))

// Active menu — points to LPS or BMS menu based on selected device type
static menu_category_t *active_menu = menu_categories_lps;
static int active_menu_count = NUM_CATEGORIES_LPS;

typedef enum {
    DEV_UNKNOWN = 0,
    DEV_LPS,
    DEV_BMS,
} device_type_t;

static void switch_active_menu(device_type_t dt)
{
    if (dt == DEV_BMS) {
        active_menu = menu_categories_bms;
        active_menu_count = NUM_CATEGORIES_BMS;
    } else {
        active_menu = menu_categories_lps;
        active_menu_count = NUM_CATEGORIES_LPS;
    }
}

// Staggered request queue for all CAN_Extra commands (BLE + local UI)
#define REQ_QUEUE_SIZE 32
#define REQ_SPACING_MS 30
#define REQ_PENDING_MAX 8
#define REQ_RESPONSE_TIMEOUT_MS 220
#define REQ_MAX_RETRIES 2

typedef struct {
    uint8_t  cmd;
    uint8_t  block;
    uint8_t  id;
    int32_t  value;
} can_req_t;

typedef struct {
    bool     active;
    uint8_t  cmd;
    uint8_t  block;
    uint8_t  id;
    int32_t  value;
    uint8_t  retries;
    uint32_t sent_time_ms;
} can_pending_req_t;

static can_req_t req_queue[REQ_QUEUE_SIZE];
static int req_queue_count = 0;
static int req_queue_head  = 0;
static uint32_t req_queue_last_ms = 0;
static can_pending_req_t req_pending[REQ_PENDING_MAX];

static void reset_request_queue(void);

static bool is_retryable_cmd(uint8_t cmd)
{
    return (cmd == CAN_CMD_GET_VAL ||
            cmd == CAN_CMD_GET_MIN ||
            cmd == CAN_CMD_GET_MAX);
}

static void pending_track_sent(uint8_t cmd, uint8_t block, uint8_t id, int32_t value)
{
    if (!is_retryable_cmd(cmd)) return;

    uint32_t now = now_ms();

    for (int i = 0; i < REQ_PENDING_MAX; i++) {
        if (req_pending[i].active &&
            req_pending[i].cmd == cmd &&
            req_pending[i].block == block &&
            req_pending[i].id == id) {
            req_pending[i].value = value;
            req_pending[i].sent_time_ms = now;
            return;
        }
    }

    for (int i = 0; i < REQ_PENDING_MAX; i++) {
        if (!req_pending[i].active) {
            req_pending[i].active = true;
            req_pending[i].cmd = cmd;
            req_pending[i].block = block;
            req_pending[i].id = id;
            req_pending[i].value = value;
            req_pending[i].retries = 0;
            req_pending[i].sent_time_ms = now;
            return;
        }
    }
}

static void pending_mark_response(uint8_t cmd, uint8_t block, uint8_t id)
{
    for (int i = 0; i < REQ_PENDING_MAX; i++) {
        if (req_pending[i].active &&
            req_pending[i].cmd == cmd &&
            req_pending[i].block == block &&
            req_pending[i].id == id) {
            req_pending[i].active = false;
            return;
        }
    }
}

static void queue_command(uint8_t cmd, uint8_t block, uint8_t id, int32_t value) {
    if (req_queue_head > 0 && req_queue_head >= req_queue_count) {
        reset_request_queue();
    } else if (req_queue_head > 0 && req_queue_count >= REQ_QUEUE_SIZE) {
        int remaining = req_queue_count - req_queue_head;
        memmove(req_queue, &req_queue[req_queue_head], remaining * sizeof(can_req_t));
        req_queue_count = remaining;
        req_queue_head = 0;
    }

    if (req_queue_count < REQ_QUEUE_SIZE) {
        req_queue[req_queue_count].cmd   = cmd;
        req_queue[req_queue_count].block = block;
        req_queue[req_queue_count].id    = id;
        req_queue[req_queue_count].value = value;
        req_queue_count++;
    }
}

static void process_pending_timeouts(void)
{
    uint32_t now = now_ms();
    for (int i = 0; i < REQ_PENDING_MAX; i++) {
        if (!req_pending[i].active) continue;

        if ((now - req_pending[i].sent_time_ms) >= REQ_RESPONSE_TIMEOUT_MS) {
            if (req_pending[i].retries < REQ_MAX_RETRIES) {
                req_pending[i].retries++;
                req_pending[i].sent_time_ms = now;
                queue_command(req_pending[i].cmd,
                              req_pending[i].block,
                              req_pending[i].id,
                              req_pending[i].value);
            } else {
                req_pending[i].active = false;
            }
        }
    }
}

static void queue_get_value(uint8_t block, uint8_t id) {
    queue_command(CAN_CMD_GET_VAL, block, id, 0);
}

static void reset_request_queue(void) {
    req_queue_count = 0;
    req_queue_head  = 0;
}

// ---------------------------------------------------------------------------
//  Decoded system data from CAN broadcasts
// ---------------------------------------------------------------------------
typedef struct {
    float    soc_percent;
    float    battery_current_a;
    int16_t  soc_time_min;
    uint16_t battery_dod_ah;

    float dc_input_voltage_v;
    float dc_input_current_a;
    float dc_output_voltage_v;
    float dc_output_current_a;

    float battery_voltage_v;

    int8_t  operating_state;
    uint8_t failure_level;
    uint8_t cell_count;
    uint8_t temp_sensor_count;

    int8_t  inverter_state;
    uint8_t inverter_failure;
    int8_t  charger_state;
    uint8_t charger_failure;
    int8_t  dc_input_state;
    uint8_t dc_input_failure;
    int8_t  dc_output_state;
    uint8_t dc_output_failure;

    float temp_internal[3];
    float temp_cell_avg;

    float ac_input_voltage_v;
    float ac_input_current_a;
    float ac_output_voltage_v;
    float ac_output_current_a;

    float cell_voltage[4];

    float solar_current_a;
    int8_t solar_state;
    uint8_t solar_failure;

    uint16_t ac_input_power_w;
    uint16_t ac_output_power_w;

    uint8_t failure_codes[8];
    uint8_t failure_code_count;

    // BMS-specific fields
    float    cell_v_min;
    float    cell_v_max;
    float    neg_term_temp;
    float    pos_term_temp;
    uint8_t  bms_output_status;
    uint16_t bms_wakeup_flags;
    uint8_t  bms_batt_count;

    uint32_t last_msg_time_ms;
    uint32_t msg_count;
    bool     connected;
} lps_data_t;

// ---------------------------------------------------------------------------
//  Unit Table — multi-device discovery and tracking
// ---------------------------------------------------------------------------
#define UNIT_TABLE_SIZE     8
#define UNIT_OFFLINE_MS     5000
#define UNIT_FREE_MS        60000

typedef enum {
    UNIT_NULL = 0,
    UNIT_ONLINE,
    UNIT_OFFLINE,
} unit_status_t;

typedef struct {
    unit_status_t status;
    uint8_t       can_addr;
    device_type_t device_type;
    char          part_number[16];
    char          serial_str[32];
    char          part_desc[32];
    uint8_t       sw_ver[3];
    uint8_t       hw_ver[2];
    uint8_t       id_msg_mask;
    bool          id_complete;
    uint32_t      id_request_time;
    lps_data_t    data;
} unit_entry_t;

static unit_entry_t unit_table[UNIT_TABLE_SIZE] = {0};
static int selected_unit = -1;

static lps_data_t lps_empty_ = {0};
static lps_data_t *lps_ptr_ = &lps_empty_;
#define lps (*lps_ptr_)

static void select_unit(int idx)
{
    if (idx >= 0 && idx < UNIT_TABLE_SIZE && unit_table[idx].status != UNIT_NULL) {
        selected_unit = idx;
        lps_ptr_ = &unit_table[idx].data;
        can_target_addr = unit_table[idx].can_addr;
        can_target_valid = true;
    } else {
        selected_unit = -1;
        lps_ptr_ = &lps_empty_;
        can_target_valid = false;
    }
}

static int unit_find_by_addr(uint8_t addr)
{
    for (int i = 0; i < UNIT_TABLE_SIZE; i++) {
        if (unit_table[i].status != UNIT_NULL && unit_table[i].can_addr == addr)
            return i;
    }
    return -1;
}

static int unit_add(uint8_t addr)
{
    int idx = unit_find_by_addr(addr);
    if (idx >= 0) {
        unit_table[idx].status = UNIT_ONLINE;
        return idx;
    }
    for (int i = 0; i < UNIT_TABLE_SIZE; i++) {
        if (unit_table[i].status == UNIT_NULL) {
            memset(&unit_table[i], 0, sizeof(unit_entry_t));
            unit_table[i].status = UNIT_ONLINE;
            unit_table[i].can_addr = addr;
            unit_table[i].id_request_time = now_ms();
            ESP_LOGI(TAG, "Discovered unit #%d at addr 0x%02X", i, addr);
            can_request_id(addr);
            if (selected_unit < 0)
                select_unit(i);
            return i;
        }
    }
    return -1;
}

static int unit_online_count(void)
{
    int n = 0;
    for (int i = 0; i < UNIT_TABLE_SIZE; i++)
        if (unit_table[i].status != UNIT_NULL) n++;
    return n;
}

static void unit_offline_tick(void)
{
    uint32_t now = now_ms();
    for (int i = 0; i < UNIT_TABLE_SIZE; i++) {
        if (unit_table[i].status == UNIT_NULL) continue;
        uint32_t elapsed = now - unit_table[i].data.last_msg_time_ms;
        if (unit_table[i].status == UNIT_ONLINE && elapsed >= UNIT_OFFLINE_MS) {
            unit_table[i].status = UNIT_OFFLINE;
            unit_table[i].data.connected = false;
            ESP_LOGW(TAG, "Unit #%d (addr 0x%02X) OFFLINE", i, unit_table[i].can_addr);
        }
        if (unit_table[i].status == UNIT_OFFLINE && elapsed >= UNIT_FREE_MS) {
            ESP_LOGI(TAG, "Unit #%d (addr 0x%02X) FREED", i, unit_table[i].can_addr);
            unit_table[i].status = UNIT_NULL;
            if (selected_unit == i) {
                int next = -1;
                for (int j = 0; j < UNIT_TABLE_SIZE; j++) {
                    if (unit_table[j].status != UNIT_NULL) { next = j; break; }
                }
                select_unit(next);
            }
        }
    }
}

static void select_prev_unit(void)
{
    if (unit_online_count() <= 1) return;
    int start = (selected_unit < 0) ? 0 : selected_unit;
    for (int i = 1; i < UNIT_TABLE_SIZE; i++) {
        int idx = (start - i + UNIT_TABLE_SIZE) % UNIT_TABLE_SIZE;
        if (unit_table[idx].status != UNIT_NULL) { select_unit(idx); return; }
    }
}

static void select_next_unit(void)
{
    if (unit_online_count() <= 1) return;
    int start = (selected_unit < 0) ? 0 : selected_unit;
    for (int i = 1; i < UNIT_TABLE_SIZE; i++) {
        int idx = (start + i) % UNIT_TABLE_SIZE;
        if (unit_table[idx].status != UNIT_NULL) { select_unit(idx); return; }
    }
}

// ---------------------------------------------------------------------------
//  Error Code Lookup Table
// ---------------------------------------------------------------------------
typedef struct {
    uint8_t     code;
    uint8_t     level;
    uint8_t     pop_level;
    const char *title;
    const char *desc;
} error_def_t;

#define FL_OK               0
#define FL_WARNING          1
#define FL_SIMPLE_FAILURE   2
#define FL_CRITICAL_FAILURE 4

#define POP_AUTO  0
#define POP_KEEP  1
#define POP_HIDE  2

typedef struct {
    uint8_t active    : 1;
    uint8_t minimized : 1;
} error_flags_t;
static error_flags_t error_flags[256] = {0};
static uint8_t  active_error_code = 0;
static bool     error_popup_visible = false;

static const error_def_t error_table[] = {
    {  1, FL_CRITICAL_FAILURE, POP_AUTO, "E001 EEPROM",             "Contact your retailer for support."},
    {  2, FL_CRITICAL_FAILURE, POP_AUTO, "E002 EEPROM",             "Contact your retailer for support."},
    {  3, FL_CRITICAL_FAILURE, POP_AUTO, "E003 High Voltage",       "Contact your retailer for support."},
    {  4, FL_WARNING,          POP_AUTO, "E004 High Temp Warning",  "Unit is getting too hot. Allow it to cool down."},
    {  5, FL_SIMPLE_FAILURE,   POP_KEEP, "E005 High Temp Failure",  "Unit is too hot. Allow it to cool down."},
    {  6, FL_WARNING,          POP_AUTO, "E006 Low Temp Warning",   "Unit is getting too cold. Allow it to warm up."},
    {  7, FL_SIMPLE_FAILURE,   POP_KEEP, "E007 Low Temp Failure",   "Unit is too cold. Allow it to warm up."},
    {  8, FL_CRITICAL_FAILURE, POP_KEEP, "E008 Broken Sensor",      "Contact your retailer for support."},
    {  9, FL_CRITICAL_FAILURE, POP_KEEP, "E009 Broken Sensor",      "Contact your retailer for support."},
    { 10, FL_CRITICAL_FAILURE, POP_AUTO, "E010 Efficiency",         "Contact your retailer for support."},
    { 11, FL_SIMPLE_FAILURE,   POP_AUTO, "E011 IO Overload",        "12 VDC aux overloaded. Remove load to avoid shutdown."},
    { 12, FL_SIMPLE_FAILURE,   POP_AUTO, "E012 IO Overload",        "12 VDC aux overloaded. Contact retailer."},
    { 13, FL_SIMPLE_FAILURE,   POP_AUTO, "E013 IO Overload",        "12 VDC aux overloaded. Contact retailer."},
    { 14, FL_SIMPLE_FAILURE,   POP_AUTO, "E014 IO Overload",        "12 VDC aux overloaded. Contact retailer."},
    { 20, FL_SIMPLE_FAILURE,   POP_AUTO, "E020 230 VAC Overload",   "230 VAC overloaded. Remove load to avoid shutdown."},
    { 22, FL_SIMPLE_FAILURE,   POP_AUTO, "E022 Charger",            "Contact your retailer for support."},
    { 23, FL_CRITICAL_FAILURE, POP_AUTO, "E023 Charger",            "Contact your retailer for support."},
    { 30, FL_CRITICAL_FAILURE, POP_AUTO, "E030 Calibration",        "Contact your retailer for support."},
    { 31, FL_CRITICAL_FAILURE, POP_AUTO, "E031 Calibration",        "Contact your retailer for support."},
    { 32, FL_CRITICAL_FAILURE, POP_AUTO, "E032 Calibration",        "Contact your retailer for support."},
    { 33, FL_CRITICAL_FAILURE, POP_AUTO, "E033 Calibration",        "Contact your retailer for support."},
    { 34, FL_CRITICAL_FAILURE, POP_AUTO, "E034 Calibration",        "Contact your retailer for support."},
    { 35, FL_CRITICAL_FAILURE, POP_AUTO, "E035 Calibration",        "Contact your retailer for support."},
    { 36, FL_CRITICAL_FAILURE, POP_AUTO, "E036 Calibration",        "Contact your retailer for support."},
    { 37, FL_CRITICAL_FAILURE, POP_AUTO, "E037 Calibration",        "Contact your retailer for support."},
    { 38, FL_CRITICAL_FAILURE, POP_AUTO, "E038 Calibration",        "Contact your retailer for support."},
    { 39, FL_CRITICAL_FAILURE, POP_AUTO, "E039 Calibration",        "Contact your retailer for support."},
    { 40, FL_CRITICAL_FAILURE, POP_AUTO, "E040 Calibration",        "Contact your retailer for support."},
    { 41, FL_CRITICAL_FAILURE, POP_AUTO, "E041 Calibration",        "Contact your retailer for support."},
    { 42, FL_CRITICAL_FAILURE, POP_AUTO, "E042 Calibration",        "Contact your retailer for support."},
    { 43, FL_CRITICAL_FAILURE, POP_AUTO, "E043 Calibration",        "Contact your retailer for support."},
    { 44, FL_CRITICAL_FAILURE, POP_AUTO, "E044 Calibration",        "Contact your retailer for support."},
    { 45, FL_CRITICAL_FAILURE, POP_AUTO, "E045 Calibration",        "Contact your retailer for support."},
    { 46, FL_CRITICAL_FAILURE, POP_AUTO, "E046 Calibration",        "Contact your retailer for support."},
    { 47, FL_CRITICAL_FAILURE, POP_AUTO, "E047 Calibration",        "Contact your retailer for support."},
    { 48, FL_CRITICAL_FAILURE, POP_AUTO, "E048 Calibration",        "Contact your retailer for support."},
    { 50, FL_CRITICAL_FAILURE, POP_KEEP, "E050 Deep Discharge",     "Battery is deep-discharged. Contact retailer."},
    { 51, FL_WARNING,          POP_KEEP, "E051 Battery Empty",      "Battery is empty. Charge the unit."},
    { 52, FL_WARNING,          POP_AUTO, "E052 Battery Low",        "Battery is low. Charge the unit."},
    { 53, FL_WARNING,          POP_KEEP, "E053 Battery Empty",      "Battery is empty. Charge the unit."},
    { 54, FL_WARNING,          POP_AUTO, "E054 Cell Balancing",     "Cell balancing in progress. If warning remains after 24 hrs, contact retailer."},
    { 55, FL_WARNING,          POP_KEEP, "E055 Cell Balancing",     "Cell balancing in progress. If failure remains after 24 hrs, contact retailer."},
    { 56, FL_WARNING,          POP_AUTO, "E056 Low Temp Warning",   "Unit is getting too cold. Place in higher ambient temp."},
    { 57, FL_SIMPLE_FAILURE,   POP_KEEP, "E057 Low Temp Failure",   "Unit is too cold. Place in higher ambient temperature."},
    { 58, FL_WARNING,          POP_AUTO, "E058 High Temp Warning",  "Unit is getting too hot. Place in lower ambient temp."},
    { 59, FL_SIMPLE_FAILURE,   POP_KEEP, "E059 High Temp Failure",  "Unit is too hot. Place in lower ambient temperature."},
    { 60, FL_CRITICAL_FAILURE, POP_AUTO, "E060 Battery Empty",      "Battery is empty. Charge the unit."},
    { 61, FL_CRITICAL_FAILURE, POP_AUTO, "E061 Battery Current",    "Discharge current too high. Remove 230 VAC and 12 VDC load."},
    { 62, FL_CRITICAL_FAILURE, POP_AUTO, "E062 Battery Current",    "Charge current too high. Disconnect all charge sources."},
    { 70, FL_CRITICAL_FAILURE, POP_AUTO, "E070 Solar Voltage",      "Check solar installation. Max 50 V from solar panel."},
    { 71, FL_CRITICAL_FAILURE, POP_AUTO, "E071 Solar Current",      "Check solar installation and max current from panel."},
    { 72, FL_CRITICAL_FAILURE, POP_AUTO, "E072 Solar",              "Contact your retailer for support."},
    { 73, FL_CRITICAL_FAILURE, POP_AUTO, "E073 Solar",              "Contact your retailer for support."},
    { 74, FL_CRITICAL_FAILURE, POP_AUTO, "E074 Solar",              "Contact your retailer for support."},
    { 75, FL_CRITICAL_FAILURE, POP_AUTO, "E075 Solar",              "Contact your retailer for support."},
    { 88, FL_WARNING,          POP_HIDE, "E088 12 VDC Overload",    "12 VDC Output overloaded. Remove load to avoid shutdown."},
    { 90, FL_SIMPLE_FAILURE,   POP_KEEP, "E090 DC Input Low",       "DC Input voltage too low. Check vehicle battery."},
    { 91, FL_SIMPLE_FAILURE,   POP_KEEP, "E091 DC Input High",      "DC Input voltage too high. Check vehicle battery."},
    { 92, FL_SIMPLE_FAILURE,   POP_AUTO, "E092 DC Input Overload",  "DC Input overloaded. Contact retailer."},
    { 93, FL_SIMPLE_FAILURE,   POP_AUTO, "E093 DC Input Failure",   "Unable to charge from vehicle. Check installation."},
    { 94, FL_CRITICAL_FAILURE, POP_AUTO, "E094 DC Input",           "Contact your retailer for support."},
    { 95, FL_CRITICAL_FAILURE, POP_AUTO, "E095 DC Input",           "Contact your retailer for support."},
    { 96, FL_SIMPLE_FAILURE,   POP_AUTO, "E096 DC Out Current",     "12 VDC output charge current too high. Check installation."},
    { 97, FL_SIMPLE_FAILURE,   POP_AUTO, "E097 DC Out Failure",     "Remove 12 VDC load. Restart unit."},
    {101, FL_CRITICAL_FAILURE, POP_AUTO, "E101 AC Offset",          "Contact your retailer for support."},
    {102, FL_CRITICAL_FAILURE, POP_AUTO, "E102 DC Offset",          "Contact your retailer for support."},
    {105, FL_CRITICAL_FAILURE, POP_AUTO, "E105 High Voltage",       "Remove 230 VAC load. Restart unit."},
    {120, FL_CRITICAL_FAILURE, POP_KEEP, "E120 DC Input Failure",   "Restart unit. If failure remains, contact retailer."},
    {121, FL_SIMPLE_FAILURE,   POP_AUTO, "E121 CAN Bus Failure",    "Check CAN cables. Contact retailer if failure remains."},
    {122, FL_WARNING,          POP_AUTO, "E122 Ext. High Temp",     "Extension getting too hot. Allow it to cool down."},
    {123, FL_SIMPLE_FAILURE,   POP_AUTO, "E123 Ext. High Temp",     "Extension is too hot. Allow it to cool down."},
    {124, FL_SIMPLE_FAILURE,   POP_AUTO, "E124 Ext. DC High",       "DC Input voltage too high. Check vehicle installation."},
    {125, FL_SIMPLE_FAILURE,   POP_AUTO, "E125 Extension Failure",  "Restart unit. If failure remains, contact retailer."},
    {126, FL_SIMPLE_FAILURE,   POP_AUTO, "E126 Extension Failure",  "Restart unit. If failure remains, contact retailer."},
    {127, FL_SIMPLE_FAILURE,   POP_AUTO, "E127 Extension Failure",  "Restart unit. If failure remains, contact retailer."},
    {130, FL_CRITICAL_FAILURE, POP_AUTO, "E130 Power Supply",       "Contact your retailer for support."},
    {131, FL_CRITICAL_FAILURE, POP_AUTO, "E131 Power Supply",       "Contact your retailer for support."},
    {132, FL_CRITICAL_FAILURE, POP_AUTO, "E132 Power Supply",       "Contact your retailer for support."},
    {133, FL_CRITICAL_FAILURE, POP_AUTO, "E133 Power Supply",       "Contact your retailer for support."},
    {135, FL_SIMPLE_FAILURE,   POP_AUTO, "E135 Power Supply",       "Contact your retailer for support."},
    {136, FL_SIMPLE_FAILURE,   POP_AUTO, "E136 Power Supply",       "Contact your retailer for support."},
    {137, FL_CRITICAL_FAILURE, POP_AUTO, "E137 Power Supply",       "Contact your retailer for support."},
    {138, FL_SIMPLE_FAILURE,   POP_AUTO, "E138 Power Supply",       "Contact your retailer for support."},
    {139, FL_SIMPLE_FAILURE,   POP_AUTO, "E139 Power Supply",       "Contact your retailer for support."},
    {140, FL_SIMPLE_FAILURE,   POP_AUTO, "E140 Power Supply",       "Contact your retailer for support."},
    {141, FL_SIMPLE_FAILURE,   POP_AUTO, "E141 Power Supply",       "Contact your retailer for support."},
    {142, FL_SIMPLE_FAILURE,   POP_AUTO, "E142 Power Supply",       "Contact your retailer for support."},
    {150, FL_SIMPLE_FAILURE,   POP_AUTO, "E150 230 VAC Overload",   "Remove 230 VAC load. Restart unit."},
    {151, FL_SIMPLE_FAILURE,   POP_AUTO, "E151 230 VAC Overload",   "Remove 230 VAC load. Restart unit."},
    {152, FL_SIMPLE_FAILURE,   POP_AUTO, "E152 230 VAC Overload",   "Remove 230 VAC load. Restart unit."},
    {153, FL_CRITICAL_FAILURE, POP_AUTO, "E153 230 VAC Failure",    "Contact your retailer for support."},
    {154, FL_CRITICAL_FAILURE, POP_AUTO, "E154 230 VAC Failure",    "Contact your retailer for support."},
    {155, FL_WARNING,          POP_HIDE, "E155 230 VAC Overload",   "Remove 230 VAC load. Restart unit."},
    {157, FL_WARNING,          POP_AUTO, "E157 Inverter Cut-Off",   "Battery SOC below cut-off limit. Charge or decrease limit."},
    {200, FL_SIMPLE_FAILURE,   POP_AUTO, "E200 AC Input Failure",   "Restart unit. If failure remains, contact retailer."},
    {202, FL_SIMPLE_FAILURE,   POP_AUTO, "E202 AC Input Failure",   "Restart unit. If failure remains, contact retailer."},
    {203, FL_SIMPLE_FAILURE,   POP_AUTO, "E203 AC Input Failure",   "Remove 230 VAC load. Restart unit."},
    {204, FL_CRITICAL_FAILURE, POP_AUTO, "E204 AC Input",           "Contact your retailer for support."},
    {205, FL_CRITICAL_FAILURE, POP_AUTO, "E205 AC Input",           "Contact your retailer for support."},
    {206, FL_SIMPLE_FAILURE,   POP_AUTO, "E206 AC In Voltage Low",  "230 VAC IN voltage too low. Check supply/cables."},
    {207, FL_SIMPLE_FAILURE,   POP_AUTO, "E207 AC In Voltage High", "230 VAC IN voltage too high. Check mains supply."},
    {220, FL_CRITICAL_FAILURE, POP_AUTO, "E220 System AC",          "Contact your retailer for support."},
    {221, FL_CRITICAL_FAILURE, POP_AUTO, "E221 System DC",          "Contact your retailer for support."},
    {222, FL_CRITICAL_FAILURE, POP_AUTO, "E222 System",             "Contact your retailer for support."},
    {223, FL_CRITICAL_FAILURE, POP_AUTO, "E223 Communication",      "Check CAN cables. Contact retailer if failure remains."},
    {240, FL_CRITICAL_FAILURE, POP_AUTO, "E240 System Locked",      "Cell voltage critically low. Unit locked. Contact retailer."},
    {241, FL_CRITICAL_FAILURE, POP_AUTO, "E241 System Locked",      "Cell voltage critically high. Unit locked. Contact retailer."},
    {242, FL_CRITICAL_FAILURE, POP_AUTO, "E242 System Locked",      "Battery voltage critically low. Unit locked. Contact retailer."},
    {243, FL_CRITICAL_FAILURE, POP_AUTO, "E243 System Locked",      "Battery voltage critically high. Unit locked. Contact retailer."},
    {244, FL_CRITICAL_FAILURE, POP_AUTO, "E244 System Locked",      "Temperature critically low. Unit locked. Contact retailer."},
    {245, FL_CRITICAL_FAILURE, POP_AUTO, "E245 System Locked",      "Temperature critically high. Unit locked. Contact retailer."},
    {246, FL_CRITICAL_FAILURE, POP_AUTO, "E246 System Locked",      "Charge current critically high. Unit locked. Contact retailer."},
    {247, FL_CRITICAL_FAILURE, POP_AUTO, "E247 System Locked",      "Discharge current critically high. Unit locked. Contact retailer."},
};
#define ERROR_TABLE_SIZE (sizeof(error_table) / sizeof(error_table[0]))

static const error_def_t *lookup_error(uint8_t code)
{
    for (int i = 0; i < (int)ERROR_TABLE_SIZE; i++) {
        if (error_table[i].code == code)
            return &error_table[i];
    }
    return NULL;
}

// ---------------------------------------------------------------------------
//  Device Identification — decode 0x19FFxx responses
// ---------------------------------------------------------------------------
static void classify_device_type(int idx)
{
    unit_entry_t *u = &unit_table[idx];
    if (strncmp(u->part_number, "CL2", 3) == 0) {
        u->device_type = DEV_LPS;
        ESP_LOGI(TAG, "Unit #%d classified as LPS (PN: %s)", idx, u->part_number);
    } else if (strncmp(u->part_number, "CB2", 3) == 0) {
        u->device_type = DEV_BMS;
        ESP_LOGI(TAG, "Unit #%d classified as BMS (PN: %s)", idx, u->part_number);
    } else {
        u->device_type = DEV_UNKNOWN;
        ESP_LOGW(TAG, "Unit #%d unknown type (PN: %s)", idx, u->part_number);
    }
    if (idx == selected_unit)
        switch_active_menu(u->device_type);
}

static void decode_identification(int unit_idx, uint8_t pgn_byte, uint8_t *data, uint8_t len)
{
    if (len < 8) return;
    unit_entry_t *u = &unit_table[unit_idx];
    u->id_msg_mask |= (1 << pgn_byte);

    switch (pgn_byte) {
    case 0x00:
        if (data[2] != 0xFF)
            snprintf(u->serial_str, sizeof(u->serial_str), "%02u.%02u.%02u-%02u%02u%02u-%02u%02u",
                     data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
        else
            snprintf(u->serial_str, sizeof(u->serial_str), "%02u.%02u-%02u%02u%02u-%02u%02u",
                     data[0], data[1], data[3], data[4], data[5], data[6], data[7]);
        break;
    case 0x02:
        u->sw_ver[0] = data[0]; u->sw_ver[1] = data[1]; u->sw_ver[2] = data[2];
        u->hw_ver[0] = data[3]; u->hw_ver[1] = data[4];
        break;
    case 0x03:
        memcpy(&u->part_number[0], data, 8);
        break;
    case 0x04:
        memcpy(&u->part_number[8], data, 7);
        u->part_number[15] = '\0';
        if (!u->id_complete) {
            u->id_complete = true;
            for (int i = 14; i >= 0; i--) {
                if (u->part_number[i] == ' ' || u->part_number[i] == '\0')
                    u->part_number[i] = '\0';
                else break;
            }
            classify_device_type(unit_idx);
            ESP_LOGI(TAG, "Unit #%d identified: PN=%s SN=%s SW=%d.%d.%d HW=%d.%d",
                     unit_idx, u->part_number, u->serial_str,
                     u->sw_ver[0], u->sw_ver[1], u->sw_ver[2],
                     u->hw_ver[0], u->hw_ver[1]);
        }
        break;
    case 0x05: memcpy(&u->part_desc[0], data, 8); break;
    case 0x06: memcpy(&u->part_desc[8], data, 8); break;
    case 0x07: memcpy(&u->part_desc[16], data, 8); break;
    case 0x08:
        memcpy(&u->part_desc[24], data, 7);
        u->part_desc[31] = '\0';
        break;
    }
}

// ---------------------------------------------------------------------------
//  CAN Message Decoders
// ---------------------------------------------------------------------------
static void decode_broadcast(lps_data_t *d, uint32_t can_id, uint8_t *data, uint8_t len)
{
    uint16_t upper = (can_id >> 16) & 0xFFFF;
    if (upper == 0x18FF || upper == 0x14FF || upper == 0x19FF) {
        uint8_t pgn = (can_id >> 8) & 0xFF;
        switch (pgn) {
        case 0x00:
            d->soc_percent = BYTES_TO_UINT16(data[0], data[1]) / 65535.0f * 100.0f;
            d->battery_current_a = BYTES_TO_SINT16(data[2], data[3]) / 10.0f;
            d->soc_time_min = BYTES_TO_SINT16(data[4], data[5]);
            d->battery_dod_ah = BYTES_TO_UINT16(data[6], data[7]);
            break;
        case 0x01:
            d->dc_input_voltage_v = BYTES_TO_UINT16(data[0], data[1]) / 100.0f;
            d->dc_input_current_a = BYTES_TO_SINT16(data[2], data[3]) / 100.0f;
            d->dc_output_voltage_v = BYTES_TO_UINT16(data[4], data[5]) / 1000.0f;
            d->dc_output_current_a = BYTES_TO_SINT16(data[6], data[7]) / 10.0f;
            break;
        case 0x03:
            d->operating_state = (int8_t)data[0];
            d->failure_level = data[1];
            d->cell_count = data[6];
            d->temp_sensor_count = data[7];
            break;
        case 0x04:
            d->inverter_state = (int8_t)data[0];
            d->inverter_failure = data[1];
            d->charger_state = (int8_t)data[2];
            d->charger_failure = data[3];
            d->dc_input_state = (int8_t)data[4];
            d->dc_input_failure = data[5];
            d->dc_output_state = (int8_t)data[6];
            d->dc_output_failure = data[7];
            break;
        case 0x05: {
            uint8_t old_codes[8];
            memcpy(old_codes, d->failure_codes, 8);
            d->failure_code_count = 0;
            for (int i = 0; i < 8; i++) {
                d->failure_codes[i] = data[i];
                if (data[i] != 0) d->failure_code_count++;
            }
            if (d == lps_ptr_) {
                for (int i = 0; i < 8; i++) {
                    uint8_t oc = old_codes[i];
                    if (oc == 0) continue;
                    bool still_in_buf = false;
                    for (int j = 0; j < 8; j++)
                        if (d->failure_codes[j] == oc) { still_in_buf = true; break; }
                    if (!still_in_buf) { error_flags[oc].active = 0; error_flags[oc].minimized = 0; }
                }
                for (int i = 0; i < 8; i++) {
                    uint8_t nc = d->failure_codes[i];
                    if (nc != 0) error_flags[nc].active = 1;
                }
            }
            break;
        }
        case 0x06:
            d->temp_internal[0] = BYTES_TO_SINT16(data[0], data[1]) / 256.0f;
            d->temp_internal[1] = BYTES_TO_SINT16(data[2], data[3]) / 256.0f;
            d->temp_internal[2] = BYTES_TO_SINT16(data[4], data[5]) / 256.0f;
            d->temp_cell_avg = BYTES_TO_SINT16(data[6], data[7]) / 256.0f;
            break;
        case 0x09:
            d->ac_input_voltage_v = BYTES_TO_UINT16(data[0], data[1]) / 10.0f;
            d->ac_input_current_a = BYTES_TO_UINT16(data[2], data[3]) / 1000.0f;
            d->ac_output_voltage_v = BYTES_TO_UINT16(data[4], data[5]) / 10.0f;
            d->ac_output_current_a = BYTES_TO_UINT16(data[6], data[7]) / 100.0f;
            break;
        case 0x10:
            d->battery_voltage_v = 0;
            for (int i = 0; i < 4; i++) {
                d->cell_voltage[i] = BYTES_TO_UINT16(data[i*2], data[i*2+1]) / 8192.0f;
                d->battery_voltage_v += d->cell_voltage[i];
            }
            break;
        case 0x20:
            d->solar_current_a = BYTES_TO_SINT16(data[4], data[5]) / 100.0f;
            d->solar_state = (int8_t)data[6];
            d->solar_failure = data[7];
            break;
        case 0x22:
            d->ac_input_power_w = BYTES_TO_UINT16(data[0], data[1]);
            d->ac_output_power_w = BYTES_TO_UINT16(data[2], data[3]);
            break;
        }
    }
}

static void decode_broadcast_bms(lps_data_t *d, uint32_t can_id, uint8_t *data, uint8_t len)
{
    uint16_t upper = (can_id >> 16) & 0xFFFF;
    if (upper == 0x18FF || upper == 0x14FF) {
        uint8_t pgn = (can_id >> 8) & 0xFF;
        switch (pgn) {
        case 0x00:
            d->operating_state = (int8_t)data[0];
            d->failure_level = data[1];
            d->bms_wakeup_flags = ((uint16_t)data[4] << 8) | data[5];
            d->bms_output_status = data[6];
            d->bms_batt_count = data[7];
            break;
        case 0x01:
            d->soc_percent = BYTES_TO_UINT16(data[0], data[1]) / 65535.0f * 100.0f;
            d->soc_time_min = BYTES_TO_SINT16(data[2], data[3]);
            d->battery_dod_ah = BYTES_TO_UINT16(data[4], data[5]);
            break;
        case 0x02:
            d->battery_voltage_v = BYTES_TO_UINT16(data[0], data[1]) / 1000.0f;
            d->battery_current_a = BYTES_TO_SINT16(data[2], data[3]) / 10.0f;
            d->cell_v_min = BYTES_TO_UINT16(data[4], data[5]) / 8192.0f;
            d->cell_v_max = BYTES_TO_UINT16(data[6], data[7]) / 8192.0f;
            break;
        case 0x03:
            d->dc_output_voltage_v = BYTES_TO_UINT16(data[0], data[1]) / 1000.0f;
            break;
        case 0x05: {
            uint8_t old_codes[8];
            memcpy(old_codes, d->failure_codes, 8);
            d->failure_code_count = 0;
            for (int i = 0; i < 8; i++) {
                d->failure_codes[i] = data[i];
                if (data[i] != 0) d->failure_code_count++;
            }
            if (d == lps_ptr_) {
                for (int i = 0; i < 8; i++) {
                    uint8_t oc = old_codes[i];
                    if (oc == 0) continue;
                    bool still = false;
                    for (int j = 0; j < 8; j++)
                        if (d->failure_codes[j] == oc) { still = true; break; }
                    if (!still) { error_flags[oc].active = 0; error_flags[oc].minimized = 0; }
                }
                for (int i = 0; i < 8; i++)
                    if (d->failure_codes[i] != 0) error_flags[d->failure_codes[i]].active = 1;
            }
            break;
        }
        case 0x06:
            d->temp_internal[0] = BYTES_TO_SINT16(data[0], data[1]) / 256.0f;
            d->temp_internal[1] = BYTES_TO_SINT16(data[2], data[3]) / 256.0f;
            d->temp_internal[2] = BYTES_TO_SINT16(data[4], data[5]) / 256.0f;
            break;
        case 0x08:
            d->neg_term_temp = BYTES_TO_SINT16(data[0], data[1]) / 256.0f;
            d->pos_term_temp = BYTES_TO_SINT16(data[2], data[3]) / 256.0f;
            break;
        case 0x10:
            d->battery_voltage_v = 0;
            for (int i = 0; i < 4; i++) {
                d->cell_voltage[i] = BYTES_TO_UINT16(data[i*2], data[i*2+1]) / 8192.0f;
                d->battery_voltage_v += d->cell_voltage[i];
            }
            break;
        }
    }
}

static void decode_can_extra(uint8_t *data, uint8_t len)
{
    if (len < 8 || data[0] != 0x00) return;
    uint8_t cmd   = data[1];
    uint8_t block = data[2];
    uint8_t id    = data[3];
    int32_t value = (int32_t)data[4]
                  | ((int32_t)data[5] << 8)
                  | ((int32_t)data[6] << 16)
                  | ((int32_t)data[7] << 24);

    if (cmd == CAN_CMD_GET_VAL || cmd == CAN_CMD_GET_MIN || cmd == CAN_CMD_GET_MAX) {
        pending_mark_response(cmd, block, id);
    }

    // Update active_menu if the setting is present (for local LCD display)
    for (int c = 0; c < active_menu_count; c++) {
        for (int s = 0; s < active_menu[c].count; s++) {
            setting_t *st = &active_menu[c].settings[s];
            if (st->block == block && st->id == id) {
                switch (cmd) {
                case CAN_CMD_GET_VAL: case CAN_CMD_SET_VAL:
                    st->current_val = value; st->val_received = true;
                    break;
                case CAN_CMD_GET_MIN: st->min_val = value; st->min_received = true; break;
                case CAN_CMD_GET_MAX: st->max_val = value; st->max_received = true; break;
                }
            }
        }
    }
}

static void decode_can_message(uint32_t can_id, uint8_t *data, uint8_t len)
{
    if (ble_can_passthrough_enabled && ble_gateway_is_connected()) {
        // Passthrough mode makes the ESP32 act as a CAN gateway; the app filters frames.
        if (should_log_boot_can_frame(can_id)) {
            log_boot_diag_frame("CAN->BLE", can_id, data, len);
        }
        ble_gateway_send_can_frame(can_id, data, len);
    }

    uint8_t src_addr = can_id & 0xFF;
    uint16_t upper = (can_id >> 16) & 0xFFFF;

    if (upper == 0x19EF || upper == 0x15EF) {
        if (selected_unit >= 0 && unit_table[selected_unit].can_addr == src_addr)
            decode_can_extra(data, len);
        int idx = unit_find_by_addr(src_addr);
        if (idx >= 0) {
            unit_table[idx].status = UNIT_ONLINE;
            unit_table[idx].data.last_msg_time_ms = now_ms();
            unit_table[idx].data.msg_count++;
            unit_table[idx].data.connected = true;
        }
        return;
    }

    if (upper == 0x19FF) {
        uint8_t pgn_byte = (can_id >> 8) & 0xFF;
        int idx = unit_add(src_addr);
        if (idx >= 0) {
            unit_table[idx].data.last_msg_time_ms = now_ms();
            unit_table[idx].data.msg_count++;
            unit_table[idx].data.connected = true;
            if (pgn_byte <= 0x08)
                decode_identification(idx, pgn_byte, data, len);
        }
        return;
    }

    int idx = unit_add(src_addr);
    if (idx >= 0) {
        lps_data_t *d = &unit_table[idx].data;
        d->last_msg_time_ms = now_ms();
        d->msg_count++;
        d->connected = true;
        if (unit_table[idx].device_type == DEV_BMS)
            decode_broadcast_bms(d, can_id, data, len);
        else
            decode_broadcast(d, can_id, data, len);
    }
}

// ---------------------------------------------------------------------------
//  Color Theme
// ---------------------------------------------------------------------------
#define COL_BG_DARK    lv_color_hex(0x0d1117)
#define COL_BG_PANEL   lv_color_hex(0x161b22)
#define COL_BG_CARD    lv_color_hex(0x21262d)
#define COL_ACCENT     lv_color_hex(0x00b4d8)
#define COL_GREEN      lv_color_hex(0x3fb950)
#define COL_ORANGE     lv_color_hex(0xd29922)
#define COL_RED        lv_color_hex(0xf85149)
#define COL_TEXT       lv_color_hex(0xe6edf3)
#define COL_TEXT_DIM   lv_color_hex(0x8b949e)
#define COL_SOC_ARC    lv_color_hex(0x00d4ff)
#define COL_SOC_BG     lv_color_hex(0x1a2332)
#define COL_SOLAR      lv_color_hex(0xf0c000)

// ---------------------------------------------------------------------------
//  Screen dimensions (1024x600)
// ---------------------------------------------------------------------------
#define SCREEN_W  1024
#define SCREEN_H  600

// ---------------------------------------------------------------------------
//  Page management
// ---------------------------------------------------------------------------
typedef enum { PAGE_DASHBOARD, PAGE_SETTINGS_GRID, PAGE_SETTINGS_DETAIL, PAGE_ERRORS } page_t;
static page_t current_page = PAGE_DASHBOARD;

static lv_obj_t *page_dashboard;
static lv_obj_t *page_grid;
static lv_obj_t *page_detail;
static lv_obj_t *page_errors;
static device_type_t grid_built_for = DEV_UNKNOWN;

// Dashboard elements
static lv_obj_t *arc_soc;
static lv_obj_t *lbl_soc_pct;
static lv_obj_t *lbl_time_left;
static lv_obj_t *lbl_batt_info;
static lv_obj_t *btn_settings;
static lv_obj_t *btn_inv_toggle;
static lv_obj_t *lbl_inv_toggle;
static lv_obj_t *btn_dcout_toggle;
static lv_obj_t *lbl_dcout_toggle;
static lv_obj_t *lbl_icon_ac;
static lv_obj_t *lbl_icon_dc;
static lv_obj_t *lbl_icon_solar;
static lv_obj_t *btn_error_badge;
static lv_obj_t *lbl_error_badge;

// Device selector
static lv_obj_t *dev_sel_container;
static lv_obj_t *btn_dev_prev;
static lv_obj_t *btn_dev_next;
static lv_obj_t *lbl_dev_name;
static lv_obj_t *lbl_dev_status;

// Error page
static lv_obj_t *error_content;
#define MAX_ERROR_DISPLAY 8
static lv_obj_t *error_title_labels[MAX_ERROR_DISPLAY];
static lv_obj_t *error_desc_labels[MAX_ERROR_DISPLAY];
static lv_obj_t *error_level_dots[MAX_ERROR_DISPLAY];
static lv_obj_t *error_rows[MAX_ERROR_DISPLAY];
static uint8_t   error_row_codes[MAX_ERROR_DISPLAY];
static lv_obj_t *lbl_no_errors;

// Error popup
static lv_obj_t *error_popup;
static lv_obj_t *error_popup_title;
static lv_obj_t *error_popup_desc;
static lv_obj_t *error_popup_level;
static lv_obj_t *error_popup_code;

// Settings detail
static lv_obj_t *detail_content;
static int detail_cat_idx = -1;
#define MAX_ITEMS_PER_CAT 16
static lv_obj_t *detail_val_labels[MAX_ITEMS_PER_CAT];
static lv_obj_t *info_val_labels[MAX_INFO_ROWS];
static int info_row_count = 0;

// Setting editor popup
static lv_obj_t *editor_overlay;
static setting_t *editor_setting;
static lv_obj_t *editor_lbl_name;
static lv_obj_t *editor_lbl_value;
static lv_obj_t *editor_lbl_range;

// ---------------------------------------------------------------------------
//  Helper Functions
// ---------------------------------------------------------------------------
static const char *get_operating_state_str(int8_t state)
{
    switch (state) {
    case -2: return "TEST";    case -1: return "DISABLED";
    case 0:  return "INIT";    case 1:  return "DEEP SLEEP";
    case 2:  return "SLEEP";   case 3:  return "CC PWR DOWN";
    case 4:  return "CC PWR UP"; case 5: return "CC ON";
    case 6:  return "PS STARTING"; case 7: return "PS STOPPING";
    case 8:  return "PS ON";
    default: return "UNKNOWN";
    }
}

static const char *get_func_state_str(int8_t state)
{
    switch (state) {
    case -2: return "TEST"; case -1: return "OFF"; case 0: return "OFF";
    case 1: return "WAKE"; case 2: return "READY"; case 3: return "START";
    case 4: return "STOP"; case 5: return "ON";
    default: return "?";
    }
}

static lv_color_t get_failure_color(uint8_t level)
{
    switch (level) {
    case 0: return COL_GREEN; case 1: case 2: return COL_ORANGE;
    case 3: case 4: return COL_RED;
    default: return COL_TEXT_DIM;
    }
}

static lv_color_t get_soc_color(float soc)
{
    if (soc > 50.0f) return COL_GREEN;
    if (soc > 20.0f) return COL_ORANGE;
    return COL_RED;
}

static void format_setting_value(char *buf, size_t buflen, setting_t *s, int32_t val)
{
    float fval = Q16_TO_FLOAT(val);
    switch (s->prefix) {
    case PREFIX_VOLTAGE:
        snprintf(buf, buflen, "%.*f V", s->decimals, (double)fval); break;
    case PREFIX_CURRENT:
        snprintf(buf, buflen, "%.*f A", s->decimals, (double)fval); break;
    case PREFIX_POWER:
        snprintf(buf, buflen, "%.0f W", (double)fval); break;
    case PREFIX_PROCENT:
        snprintf(buf, buflen, "%.0f %%", (double)(fval * 100.0f)); break;
    case PREFIX_TIME_HHMMSS: {
        int32_t abs_val = val < 0 ? -val : val;
        int64_t total_secs_q16 = (int64_t)abs_val * 3600;
        int total_secs = (int)(total_secs_q16 >> 16);
        if ((total_secs_q16 & 0xFFFF) > 0x8000) total_secs++;
        int h = total_secs / 3600;
        int m = (total_secs % 3600) / 60;
        if (total_secs == 0)     snprintf(buf, buflen, "OFF");
        else if (h > 0)         snprintf(buf, buflen, "%dh %02dm", h, m);
        else                    snprintf(buf, buflen, "%d min", m);
        break;
    }
    case PREFIX_ENUM: {
        int idx = (int)(val >> 16);
        if (s->enum_labels && idx >= 0 && idx < s->enum_count)
            snprintf(buf, buflen, "%s", s->enum_labels[idx]);
        else
            snprintf(buf, buflen, "%d", idx);
        break;
    }
    default:
        snprintf(buf, buflen, "%.*f %s", s->decimals, (double)fval, s->unit); break;
    }
}

// BMS operating state string
static const char *get_bms_operating_state_str(uint8_t state)
{
    switch (state) {
    case 0x00: return "INIT";    case 0x10: return "SLEEP";
    case 0x14: return "CC PWR DOWN"; case 0x15: return "CC PWR UP";
    case 0x20: return "CC READY";  case 0x24: return "MPSU PWR DOWN";
    case 0x25: return "MPSU PWR UP"; case 0x30: return "MPSU READY";
    case 0x40: return "CHARGING";  case 0x41: return "DISCHARGING";
    case 0x42: return "CONNECTED"; case 0xFE: return "TEST";
    default:   return "UNKNOWN";
    }
}

// Info row labels per info_type
static const char *info_labels_ac_out[]     = {"Status", "Power", "Voltage", "Current"};
static const char *info_labels_ac_in[]      = {"Status", "Power", "Voltage", "Current"};
static const char *info_labels_dc_out[]     = {"Status", "Power", "Voltage", "Current"};
static const char *info_labels_dc_in[]      = {"Status", "Power", "Voltage", "Current"};
static const char *info_labels_solar[]      = {"Status", "Current"};
static const char *info_labels_lps_status[] = {"State", "SOC", "Time Left", "Power",
                                               "Voltage", "Current",
                                               "Cell 1", "Cell 2", "Cell 3", "Cell 4"};
static const char *info_labels_lps_temp[]   = {"Internal 1", "Internal 2", "Internal 3", "Cell Avg"};
static const char *info_labels_bms_status[] = {"State", "SOC", "Time Left",
                                               "Voltage", "Current", "DOD",
                                               "Cell Min", "Cell Max", "Cell 1", "Cell 2"};
static const char *info_labels_bms_temp[]   = {"Cell Temp 1", "Cell Temp 2", "Cell Temp 3",
                                               "Neg Terminal", "Pos Terminal"};

static int get_info_count(int info_type)
{
    switch (info_type) {
    case INFO_AC_OUT: case INFO_AC_IN: case INFO_DC_OUT: case INFO_DC_IN: return 4;
    case INFO_SOLAR: return 2;
    case INFO_LPS_STATUS: case INFO_BMS_STATUS: return 10;
    case INFO_LPS_TEMP: return 4;
    case INFO_BMS_TEMP: return 5;
    default: return 0;
    }
}

static const char **get_info_labels(int info_type)
{
    switch (info_type) {
    case INFO_AC_OUT:     return info_labels_ac_out;
    case INFO_AC_IN:      return info_labels_ac_in;
    case INFO_DC_OUT:     return info_labels_dc_out;
    case INFO_DC_IN:      return info_labels_dc_in;
    case INFO_SOLAR:      return info_labels_solar;
    case INFO_LPS_STATUS: return info_labels_lps_status;
    case INFO_LPS_TEMP:   return info_labels_lps_temp;
    case INFO_BMS_STATUS: return info_labels_bms_status;
    case INFO_BMS_TEMP:   return info_labels_bms_temp;
    default: return NULL;
    }
}

static void format_info_value(char *buf, size_t sz, int cat_idx, int row)
{
    int info_type = INFO_NONE;
    if (cat_idx >= 0 && cat_idx < active_menu_count)
        info_type = active_menu[cat_idx].info_type;

    switch (info_type) {
    case INFO_AC_OUT:
        switch (row) {
        case 0: snprintf(buf, sz, "%s", get_func_state_str(lps.inverter_state)); return;
        case 1: snprintf(buf, sz, "%u W", lps.ac_output_power_w); return;
        case 2: snprintf(buf, sz, "%.1f V", (double)lps.ac_output_voltage_v); return;
        case 3: snprintf(buf, sz, "%.1f A", (double)lps.ac_output_current_a); return;
        } break;
    case INFO_AC_IN:
        switch (row) {
        case 0: snprintf(buf, sz, "%s", get_func_state_str(lps.charger_state)); return;
        case 1: snprintf(buf, sz, "%u W", lps.ac_input_power_w); return;
        case 2: snprintf(buf, sz, "%.1f V", (double)lps.ac_input_voltage_v); return;
        case 3: snprintf(buf, sz, "%.1f A", (double)lps.ac_input_current_a); return;
        } break;
    case INFO_DC_OUT:
        switch (row) {
        case 0: snprintf(buf, sz, "%s", get_func_state_str(lps.dc_output_state)); return;
        case 1: { float p = lps.dc_output_voltage_v * lps.dc_output_current_a;
                  snprintf(buf, sz, "%.0f W", (double)p); return; }
        case 2: snprintf(buf, sz, "%.2f V", (double)lps.dc_output_voltage_v); return;
        case 3: snprintf(buf, sz, "%.2f A", (double)lps.dc_output_current_a); return;
        } break;
    case INFO_DC_IN:
        switch (row) {
        case 0: snprintf(buf, sz, "%s", get_func_state_str(lps.dc_input_state)); return;
        case 1: { float p = lps.dc_input_voltage_v * lps.dc_input_current_a;
                  snprintf(buf, sz, "%.0f W", (double)p); return; }
        case 2: snprintf(buf, sz, "%.2f V", (double)lps.dc_input_voltage_v); return;
        case 3: snprintf(buf, sz, "%.2f A", (double)lps.dc_input_current_a); return;
        } break;
    case INFO_SOLAR:
        switch (row) {
        case 0: snprintf(buf, sz, "%s", get_func_state_str(lps.solar_state)); return;
        case 1: snprintf(buf, sz, "%.2f A", (double)lps.solar_current_a); return;
        } break;
    case INFO_LPS_STATUS:
        switch (row) {
        case 0: snprintf(buf, sz, "%s", get_operating_state_str(lps.operating_state)); return;
        case 1: snprintf(buf, sz, "%.0f %%", (double)lps.soc_percent); return;
        case 2: {
            if (lps.soc_time_min > 0) {
                int h = lps.soc_time_min / 60, m = lps.soc_time_min % 60;
                if (h > 0) snprintf(buf, sz, "%dh %dm", h, m);
                else       snprintf(buf, sz, "%d min", m);
            } else snprintf(buf, sz, "--");
            return; }
        case 3: { float p = lps.battery_voltage_v * lps.battery_current_a;
                  snprintf(buf, sz, "%.0f W", (double)p); return; }
        case 4: snprintf(buf, sz, "%.2f V", (double)lps.battery_voltage_v); return;
        case 5: snprintf(buf, sz, "%+.1f A", (double)lps.battery_current_a); return;
        case 6: snprintf(buf, sz, "%.3f V", (double)lps.cell_voltage[0]); return;
        case 7: snprintf(buf, sz, "%.3f V", (double)lps.cell_voltage[1]); return;
        case 8: snprintf(buf, sz, "%.3f V", (double)lps.cell_voltage[2]); return;
        case 9: snprintf(buf, sz, "%.3f V", (double)lps.cell_voltage[3]); return;
        } break;
    case INFO_LPS_TEMP:
        switch (row) {
        case 0: snprintf(buf, sz, "%.1f C", (double)lps.temp_internal[0]); return;
        case 1: snprintf(buf, sz, "%.1f C", (double)lps.temp_internal[1]); return;
        case 2: snprintf(buf, sz, "%.1f C", (double)lps.temp_internal[2]); return;
        case 3: snprintf(buf, sz, "%.1f C", (double)lps.temp_cell_avg); return;
        } break;
    case INFO_BMS_STATUS:
        switch (row) {
        case 0: snprintf(buf, sz, "%s", get_bms_operating_state_str((uint8_t)lps.operating_state)); return;
        case 1: snprintf(buf, sz, "%.0f %%", (double)lps.soc_percent); return;
        case 2: {
            int t = lps.soc_time_min < 0 ? -lps.soc_time_min : lps.soc_time_min;
            if (t > 0) {
                int h = t / 60, m = t % 60;
                if (h > 0) snprintf(buf, sz, "%dh %dm", h, m);
                else       snprintf(buf, sz, "%d min", m);
            } else snprintf(buf, sz, "--");
            return; }
        case 3: snprintf(buf, sz, "%.2f V", (double)lps.battery_voltage_v); return;
        case 4: snprintf(buf, sz, "%+.1f A", (double)lps.battery_current_a); return;
        case 5: snprintf(buf, sz, "%u Ah", lps.battery_dod_ah); return;
        case 6: snprintf(buf, sz, "%.3f V", (double)lps.cell_v_min); return;
        case 7: snprintf(buf, sz, "%.3f V", (double)lps.cell_v_max); return;
        case 8: snprintf(buf, sz, "%.3f V", (double)lps.cell_voltage[0]); return;
        case 9: snprintf(buf, sz, "%.3f V", (double)lps.cell_voltage[1]); return;
        } break;
    case INFO_BMS_TEMP:
        switch (row) {
        case 0: snprintf(buf, sz, "%.1f C", (double)lps.temp_internal[0]); return;
        case 1: snprintf(buf, sz, "%.1f C", (double)lps.temp_internal[1]); return;
        case 2: snprintf(buf, sz, "%.1f C", (double)lps.temp_internal[2]); return;
        case 3: snprintf(buf, sz, "%.1f C", (double)lps.neg_term_temp); return;
        case 4: snprintf(buf, sz, "%.1f C", (double)lps.pos_term_temp); return;
        } break;
    }
    snprintf(buf, sz, "--");
}

// Create a styled panel/card
static lv_obj_t *create_card(lv_obj_t *parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, COL_BG_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x30363d), 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, 6, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

static lv_obj_t *create_section_label(lv_obj_t *parent, const char *text,
                                       lv_align_t align, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, COL_ACCENT, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_align(lbl, align, x, y);
    return lbl;
}

static lv_obj_t *create_value_label(lv_obj_t *parent, const char *text,
                                     const lv_font_t *font,
                                     lv_align_t align, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, COL_TEXT, 0);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_align(lbl, align, x, y);
    return lbl;
}

// ---------------------------------------------------------------------------
//  Settings editor popup
// ---------------------------------------------------------------------------
static void editor_update_display(void)
{
    if (!editor_setting) return;
    char buf[80];
    format_setting_value(buf, sizeof(buf), editor_setting, editor_setting->current_val);
    lv_label_set_text(editor_lbl_value, buf);

    if (editor_setting->min_received && editor_setting->max_received) {
        char bmin[32], bmax[32];
        format_setting_value(bmin, sizeof(bmin), editor_setting, editor_setting->min_val);
        format_setting_value(bmax, sizeof(bmax), editor_setting, editor_setting->max_val);
        snprintf(buf, sizeof(buf), "%s .. %s", bmin, bmax);
        lv_label_set_text(editor_lbl_range, buf);
    }
}

static void editor_btn_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    const char *txt = lv_label_get_text(lv_obj_get_child(btn, 0));
    buzzer_click();

    if (strcmp(txt, LV_SYMBOL_CLOSE) == 0) {
        can_get_value(editor_setting->block, editor_setting->id);
        lv_obj_add_flag(editor_overlay, LV_OBJ_FLAG_HIDDEN);
        editor_setting = NULL;
        return;
    }
    if (strcmp(txt, LV_SYMBOL_OK) == 0) {
        can_set_value(editor_setting->block, editor_setting->id, editor_setting->current_val);
        lv_obj_add_flag(editor_overlay, LV_OBJ_FLAG_HIDDEN);
        editor_setting = NULL;
        return;
    }
    if (!editor_setting) return;

    if (strcmp(txt, LV_SYMBOL_PLUS) == 0) {
        int32_t nv = editor_setting->current_val + editor_setting->step_slow;
        if (editor_setting->max_received && nv > editor_setting->max_val) nv = editor_setting->max_val;
        editor_setting->current_val = nv;
    } else if (strcmp(txt, LV_SYMBOL_MINUS) == 0) {
        int32_t nv = editor_setting->current_val - editor_setting->step_slow;
        if (editor_setting->min_received && nv < editor_setting->min_val) nv = editor_setting->min_val;
        editor_setting->current_val = nv;
    }
    editor_update_display();
}

static void create_editor_overlay(lv_obj_t *parent)
{
    editor_overlay = lv_obj_create(parent);
    lv_obj_set_size(editor_overlay, 500, 300);
    lv_obj_center(editor_overlay);
    lv_obj_set_style_bg_color(editor_overlay, lv_color_hex(0x161b22), 0);
    lv_obj_set_style_bg_opa(editor_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(editor_overlay, 2, 0);
    lv_obj_set_style_border_color(editor_overlay, COL_ACCENT, 0);
    lv_obj_set_style_radius(editor_overlay, 12, 0);
    lv_obj_set_style_pad_all(editor_overlay, 16, 0);
    lv_obj_clear_flag(editor_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(editor_overlay, LV_OBJ_FLAG_HIDDEN);

    editor_lbl_name = lv_label_create(editor_overlay);
    lv_label_set_text(editor_lbl_name, "Setting");
    lv_obj_set_style_text_color(editor_lbl_name, COL_ACCENT, 0);
    lv_obj_set_style_text_font(editor_lbl_name, &lv_font_montserrat_20, 0);
    lv_obj_align(editor_lbl_name, LV_ALIGN_TOP_MID, 0, 0);

    editor_lbl_value = lv_label_create(editor_overlay);
    lv_label_set_text(editor_lbl_value, "---");
    lv_obj_set_style_text_color(editor_lbl_value, COL_TEXT, 0);
    lv_obj_set_style_text_font(editor_lbl_value, &lv_font_montserrat_28, 0);
    lv_obj_align(editor_lbl_value, LV_ALIGN_TOP_MID, 0, 45);

    editor_lbl_range = lv_label_create(editor_overlay);
    lv_label_set_text(editor_lbl_range, "");
    lv_obj_set_style_text_color(editor_lbl_range, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(editor_lbl_range, &lv_font_montserrat_14, 0);
    lv_obj_align(editor_lbl_range, LV_ALIGN_TOP_MID, 0, 95);

    int btn_w = 100, btn_h = 56, btn_y = 130;
    const char *syms[] = {LV_SYMBOL_MINUS, LV_SYMBOL_PLUS};
    int x_pos[] = {-110, 110};
    for (int i = 0; i < 2; i++) {
        lv_obj_t *b = lv_btn_create(editor_overlay);
        lv_obj_set_size(b, btn_w, btn_h);
        lv_obj_align(b, LV_ALIGN_TOP_MID, x_pos[i], btn_y);
        lv_obj_set_style_bg_color(b, COL_BG_CARD, 0);
        lv_obj_set_style_radius(b, 8, 0);
        lv_obj_t *lb = lv_label_create(b);
        lv_label_set_text(lb, syms[i]);
        lv_obj_set_style_text_font(lb, &lv_font_montserrat_24, 0);
        lv_obj_center(lb);
        lv_obj_add_event_cb(b, editor_btn_cb, LV_EVENT_CLICKED, NULL);
    }

    int ok_y = 215;
    const char *ok_syms[] = {LV_SYMBOL_OK, LV_SYMBOL_CLOSE};
    lv_color_t ok_cols[] = {COL_GREEN, COL_RED};
    int ok_x[] = {-80, 80};
    for (int i = 0; i < 2; i++) {
        lv_obj_t *b = lv_btn_create(editor_overlay);
        lv_obj_set_size(b, 130, 50);
        lv_obj_align(b, LV_ALIGN_TOP_MID, ok_x[i], ok_y);
        lv_obj_set_style_bg_color(b, ok_cols[i], 0);
        lv_obj_set_style_radius(b, 8, 0);
        lv_obj_t *lb = lv_label_create(b);
        lv_label_set_text(lb, ok_syms[i]);
        lv_obj_set_style_text_font(lb, &lv_font_montserrat_20, 0);
        lv_obj_center(lb);
        lv_obj_add_event_cb(b, editor_btn_cb, LV_EVENT_CLICKED, NULL);
    }
}

static void open_editor(setting_t *s)
{
    editor_setting = s;
    lv_label_set_text(editor_lbl_name, s->label);
    editor_update_display();
    lv_obj_clear_flag(editor_overlay, LV_OBJ_FLAG_HIDDEN);
    can_get_value(s->block, s->id);
    can_get_min(s->block, s->id);
    can_get_max(s->block, s->id);
}

// ---------------------------------------------------------------------------
//  Page switching
// ---------------------------------------------------------------------------
static void show_page(page_t p);
static void open_category(int cat_idx);
static void rebuild_settings_grid(void);

static void btn_settings_cb(lv_event_t *e) { (void)e; buzzer_click(); show_page(PAGE_SETTINGS_GRID); }
static void btn_grid_back_cb(lv_event_t *e) { (void)e; buzzer_click(); show_page(PAGE_DASHBOARD); }

static void btn_detail_back_cb(lv_event_t *e)
{
    (void)e; buzzer_click();
    memset(info_val_labels, 0, sizeof(info_val_labels));
    memset(detail_val_labels, 0, sizeof(detail_val_labels));
    info_row_count = 0;
    lv_obj_clean(detail_content);
    show_page(PAGE_SETTINGS_GRID);
}

static void show_page(page_t p)
{
    current_page = p;
    lv_obj_add_flag(page_dashboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(page_grid, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(page_detail, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(page_errors, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *target = NULL;
    switch (p) {
    case PAGE_DASHBOARD:       target = page_dashboard; break;
    case PAGE_SETTINGS_GRID:   target = page_grid; rebuild_settings_grid(); break;
    case PAGE_SETTINGS_DETAIL: target = page_detail; break;
    case PAGE_ERRORS:          target = page_errors; break;
    }
    if (target) lv_obj_clear_flag(target, LV_OBJ_FLAG_HIDDEN);

    lv_obj_scroll_to(lv_scr_act(), 0, 0, LV_ANIM_OFF);
    lv_obj_scroll_to(detail_content, 0, 0, LV_ANIM_OFF);
    lv_obj_scroll_to(error_content, 0, 0, LV_ANIM_OFF);
    lv_obj_invalidate(lv_scr_act());
}

// ---------------------------------------------------------------------------
//  Category grid page — icon tiles
// ---------------------------------------------------------------------------
static void cat_tile_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= active_menu_count) return;
    buzzer_click();
    open_category(idx);
    show_page(PAGE_SETTINGS_DETAIL);
}

static void create_page_fullscreen(lv_obj_t **page, lv_obj_t *parent)
{
    *page = lv_obj_create(parent);
    lv_obj_set_size(*page, SCREEN_W, SCREEN_H);
    lv_obj_set_style_bg_color(*page, COL_BG_DARK, 0);
    lv_obj_set_style_bg_opa(*page, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(*page, 0, 0);
    lv_obj_set_style_pad_all(*page, 0, 0);
    lv_obj_set_style_radius(*page, 0, 0);
    lv_obj_align(*page, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_clear_flag(*page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(*page, LV_OBJ_FLAG_CLICKABLE);
}

// Grid layout constants for 1024×600
#define TILE_W    200
#define TILE_H    140
#define TILE_GAP  20
#define GRID_COLS 4
#define GRID_X0   ((SCREEN_W - GRID_COLS*TILE_W - (GRID_COLS-1)*TILE_GAP) / 2)
#define GRID_Y0   56

static void build_grid_tiles(void)
{
    for (int i = 0; i < active_menu_count; i++) {
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;
        int x = GRID_X0 + col * (TILE_W + TILE_GAP);
        int y = GRID_Y0 + row * (TILE_H + TILE_GAP);

        lv_obj_t *tile = lv_btn_create(page_grid);
        lv_obj_set_pos(tile, x, y);
        lv_obj_set_size(tile, TILE_W, TILE_H);
        lv_obj_set_style_bg_color(tile, COL_BG_CARD, 0);
        lv_obj_set_style_radius(tile, 12, 0);
        lv_obj_set_style_border_width(tile, 1, 0);
        lv_obj_set_style_border_color(tile, COL_BG_PANEL, 0);
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x2a3040), LV_STATE_PRESSED);
        lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(tile, 10, 0);

        lv_obj_t *icon = lv_label_create(tile);
        lv_label_set_text(icon, active_menu[i].icon);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(icon, COL_ACCENT, 0);

        lv_obj_t *name = lv_label_create(tile);
        lv_label_set_text(name, active_menu[i].title);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(name, COL_TEXT, 0);
        lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_add_event_cb(tile, cat_tile_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}

static void create_settings_grid(lv_obj_t *parent)
{
    create_page_fullscreen(&page_grid, parent);

    lv_obj_t *lbl = lv_label_create(page_grid);
    lv_label_set_text(lbl, LV_SYMBOL_SETTINGS "  SETTINGS");
    lv_obj_set_style_text_color(lbl, COL_ACCENT, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 16, 12);

    lv_obj_t *btn = lv_btn_create(page_grid);
    lv_obj_set_size(btn, 100, 40);
    lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -12, 8);
    lv_obj_set_style_bg_color(btn, COL_BG_CARD, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_t *lb = lv_label_create(btn);
    lv_label_set_text(lb, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_14, 0);
    lv_obj_center(lb);
    lv_obj_add_event_cb(btn, btn_grid_back_cb, LV_EVENT_CLICKED, NULL);

    build_grid_tiles();
    grid_built_for = (selected_unit >= 0) ? unit_table[selected_unit].device_type : DEV_UNKNOWN;
    lv_obj_add_flag(page_grid, LV_OBJ_FLAG_HIDDEN);
}

static void rebuild_settings_grid(void)
{
    device_type_t cur = (selected_unit >= 0) ? unit_table[selected_unit].device_type : DEV_UNKNOWN;
    if (cur == grid_built_for) return;

    uint32_t child_cnt = lv_obj_get_child_cnt(page_grid);
    for (int i = (int)child_cnt - 1; i >= 2; i--)
        lv_obj_del(lv_obj_get_child(page_grid, i));

    build_grid_tiles();
    grid_built_for = cur;
}

// ---------------------------------------------------------------------------
//  Category detail page
// ---------------------------------------------------------------------------
#define DETAIL_ROW_W 960

static void setting_item_cb(lv_event_t *e)
{
    setting_t *s = (setting_t *)lv_event_get_user_data(e);
    buzzer_click();
    open_editor(s);
}

static void populate_detail(int cat_idx)
{
    detail_cat_idx = cat_idx;
    menu_category_t *cat = &active_menu[cat_idx];

    lv_obj_clean(detail_content);
    memset(detail_val_labels, 0, sizeof(detail_val_labels));
    memset(info_val_labels, 0, sizeof(info_val_labels));
    info_row_count = 0;

    // Info rows (read-only)
    int n_info = get_info_count(cat->info_type);
    const char **labels = get_info_labels(cat->info_type);
    for (int i = 0; i < n_info && i < MAX_INFO_ROWS; i++) {
        lv_obj_t *row = lv_obj_create(detail_content);
        lv_obj_set_size(row, DETAIL_ROW_W, 40);
        lv_obj_set_style_bg_color(row, COL_BG_PANEL, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(row, 6, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_hor(row, 18, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl_name = lv_label_create(row);
        lv_label_set_text(lbl_name, labels[i]);
        lv_obj_set_style_text_color(lbl_name, COL_TEXT_DIM, 0);
        lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_name, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *lbl_val = lv_label_create(row);
        char vbuf[32];
        format_info_value(vbuf, sizeof(vbuf), cat_idx, i);
        lv_label_set_text(lbl_val, vbuf);
        lv_obj_set_style_text_color(lbl_val, COL_TEXT, 0);
        lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_16, 0);
        lv_obj_align(lbl_val, LV_ALIGN_RIGHT_MID, 0, 0);
        info_val_labels[i] = lbl_val;
    }
    info_row_count = n_info < MAX_INFO_ROWS ? n_info : MAX_INFO_ROWS;

    // Separator
    if (n_info > 0 && cat->count > 0) {
        lv_obj_t *sep = lv_obj_create(detail_content);
        lv_obj_set_size(sep, DETAIL_ROW_W - 20, 1);
        lv_obj_set_style_bg_color(sep, COL_BG_CARD, 0);
        lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(sep, 0, 0);
        lv_obj_set_style_pad_all(sep, 0, 0);
        lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE);
    }

    // Setting rows
    reset_request_queue();
    for (int s = 0; s < cat->count; s++)
        queue_get_value(cat->settings[s].block, cat->settings[s].id);

    for (int s = 0; s < cat->count && s < MAX_ITEMS_PER_CAT; s++) {
        setting_t *st = &cat->settings[s];
        lv_obj_t *row = lv_btn_create(detail_content);
        lv_obj_set_size(row, DETAIL_ROW_W, 50);
        lv_obj_set_style_bg_color(row, COL_BG_CARD, 0);
        lv_obj_set_style_bg_color(row, lv_color_hex(0x2a3040), LV_STATE_PRESSED);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_pad_hor(row, 18, 0);

        lv_obj_t *lbl_name = lv_label_create(row);
        lv_label_set_text(lbl_name, st->label);
        lv_obj_set_style_text_color(lbl_name, COL_TEXT, 0);
        lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_16, 0);
        lv_obj_align(lbl_name, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *lbl_val = lv_label_create(row);
        if (st->val_received) {
            char vbuf[32];
            format_setting_value(vbuf, sizeof(vbuf), st, st->current_val);
            lv_label_set_text(lbl_val, vbuf);
        } else {
            lv_label_set_text(lbl_val, "...");
        }
        lv_obj_set_style_text_color(lbl_val, COL_ACCENT, 0);
        lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_16, 0);
        lv_obj_align(lbl_val, LV_ALIGN_RIGHT_MID, 0, 0);
        detail_val_labels[s] = lbl_val;
        lv_obj_add_event_cb(row, setting_item_cb, LV_EVENT_CLICKED, st);
    }
    lv_obj_scroll_to(detail_content, 0, 0, LV_ANIM_OFF);
}

static void open_category(int cat_idx)
{
    if (cat_idx < 0 || cat_idx >= active_menu_count) return;
    menu_category_t *cat = &active_menu[cat_idx];
    lv_obj_t *title = lv_obj_get_child(page_detail, 0);
    char buf[64];
    snprintf(buf, sizeof(buf), "%s  %s", cat->icon, cat->title);
    lv_label_set_text(title, buf);
    populate_detail(cat_idx);
}

static void update_settings_detail(void)
{
    if (detail_cat_idx < 0 || detail_cat_idx >= active_menu_count) return;
    menu_category_t *cat = &active_menu[detail_cat_idx];

    for (int i = 0; i < info_row_count; i++) {
        if (!info_val_labels[i]) continue;
        char vbuf[32];
        format_info_value(vbuf, sizeof(vbuf), detail_cat_idx, i);
        lv_label_set_text(info_val_labels[i], vbuf);
    }
    for (int s = 0; s < cat->count && s < MAX_ITEMS_PER_CAT; s++) {
        if (!detail_val_labels[s]) continue;
        setting_t *st = &cat->settings[s];
        if (st->val_received) {
            char vbuf[32];
            format_setting_value(vbuf, sizeof(vbuf), st, st->current_val);
            lv_label_set_text(detail_val_labels[s], vbuf);
        }
    }
}

static void create_settings_detail(lv_obj_t *parent)
{
    create_page_fullscreen(&page_detail, parent);

    lv_obj_t *lbl = lv_label_create(page_detail);
    lv_label_set_text(lbl, "");
    lv_obj_set_style_text_color(lbl, COL_ACCENT, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 16, 12);

    lv_obj_t *btn = lv_btn_create(page_detail);
    lv_obj_set_size(btn, 100, 40);
    lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -12, 8);
    lv_obj_set_style_bg_color(btn, COL_BG_CARD, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_t *lb = lv_label_create(btn);
    lv_label_set_text(lb, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_14, 0);
    lv_obj_center(lb);
    lv_obj_add_event_cb(btn, btn_detail_back_cb, LV_EVENT_CLICKED, NULL);

    detail_content = lv_obj_create(page_detail);
    lv_obj_set_size(detail_content, SCREEN_W - 20, SCREEN_H - 60);
    lv_obj_align(detail_content, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_opa(detail_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(detail_content, 0, 0);
    lv_obj_set_style_pad_all(detail_content, 4, 0);
    lv_obj_set_flex_flow(detail_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(detail_content, 4, 0);
    lv_obj_clear_flag(detail_content, LV_OBJ_FLAG_SCROLL_CHAIN);

    create_editor_overlay(page_detail);
    lv_obj_add_flag(page_detail, LV_OBJ_FLAG_HIDDEN);
}

// ---------------------------------------------------------------------------
//  Error Page
// ---------------------------------------------------------------------------
static void btn_error_badge_cb(lv_event_t *e) { (void)e; buzzer_click(); show_page(PAGE_ERRORS); }
static void btn_error_back_cb(lv_event_t *e)  { (void)e; buzzer_click(); show_page(PAGE_DASHBOARD); }

static lv_color_t get_error_level_color(uint8_t level)
{
    switch (level) {
    case FL_WARNING:          return COL_ORANGE;
    case FL_SIMPLE_FAILURE:   return COL_RED;
    case FL_CRITICAL_FAILURE: return COL_RED;
    default:                  return COL_TEXT_DIM;
    }
}

static const char *get_error_level_str(uint8_t level)
{
    switch (level) {
    case FL_WARNING:          return "WARNING";
    case FL_SIMPLE_FAILURE:   return "FAILURE";
    case FL_CRITICAL_FAILURE: return "CRITICAL";
    default:                  return "UNKNOWN";
    }
}

static void error_row_click_cb(lv_event_t *e);
static void show_error_popup(uint8_t code);

#define ERROR_ROW_W 960

static void create_error_page(lv_obj_t *parent)
{
    create_page_fullscreen(&page_errors, parent);

    lv_obj_t *lbl = lv_label_create(page_errors);
    lv_label_set_text(lbl, LV_SYMBOL_WARNING "  ERRORS");
    lv_obj_set_style_text_color(lbl, COL_RED, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 16, 12);

    lv_obj_t *btn = lv_btn_create(page_errors);
    lv_obj_set_size(btn, 100, 40);
    lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -12, 8);
    lv_obj_set_style_bg_color(btn, COL_BG_CARD, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_t *lb = lv_label_create(btn);
    lv_label_set_text(lb, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_14, 0);
    lv_obj_center(lb);
    lv_obj_add_event_cb(btn, btn_error_back_cb, LV_EVENT_CLICKED, NULL);

    error_content = lv_obj_create(page_errors);
    lv_obj_set_size(error_content, SCREEN_W - 20, SCREEN_H - 60);
    lv_obj_align(error_content, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_opa(error_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(error_content, 0, 0);
    lv_obj_set_style_pad_all(error_content, 4, 0);
    lv_obj_set_flex_flow(error_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(error_content, 6, 0);
    lv_obj_clear_flag(error_content, LV_OBJ_FLAG_SCROLL_CHAIN);

    for (int i = 0; i < MAX_ERROR_DISPLAY; i++) {
        lv_obj_t *row = lv_obj_create(error_content);
        lv_obj_set_size(row, ERROR_ROW_W, 80);
        lv_obj_set_style_bg_color(row, COL_BG_CARD, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, COL_RED, 0);
        lv_obj_set_style_pad_all(row, 12, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, error_row_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        error_rows[i] = row;
        error_row_codes[i] = 0;

        lv_obj_t *dot = lv_obj_create(row);
        lv_obj_set_size(dot, 12, 12);
        lv_obj_set_style_radius(dot, 6, 0);
        lv_obj_set_style_bg_color(dot, COL_RED, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_align(dot, LV_ALIGN_TOP_LEFT, 0, 4);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        error_level_dots[i] = dot;

        lv_obj_t *title = lv_label_create(row);
        lv_label_set_text(title, "");
        lv_obj_set_style_text_color(title, COL_TEXT, 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 22, 0);
        error_title_labels[i] = title;

        lv_obj_t *desc = lv_label_create(row);
        lv_label_set_text(desc, "");
        lv_obj_set_style_text_color(desc, COL_TEXT_DIM, 0);
        lv_obj_set_style_text_font(desc, &lv_font_montserrat_12, 0);
        lv_obj_set_width(desc, ERROR_ROW_W - 50);
        lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);
        lv_obj_align(desc, LV_ALIGN_TOP_LEFT, 22, 24);
        error_desc_labels[i] = desc;
    }

    lbl_no_errors = lv_label_create(error_content);
    lv_label_set_text(lbl_no_errors, "No active errors");
    lv_obj_set_style_text_color(lbl_no_errors, COL_GREEN, 0);
    lv_obj_set_style_text_font(lbl_no_errors, &lv_font_montserrat_20, 0);
    lv_obj_set_style_pad_top(lbl_no_errors, 60, 0);
    lv_obj_set_style_text_align(lbl_no_errors, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(lbl_no_errors, ERROR_ROW_W);

    lv_obj_add_flag(page_errors, LV_OBJ_FLAG_HIDDEN);
}

static void error_row_click_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= MAX_ERROR_DISPLAY) return;
    uint8_t code = error_row_codes[idx];
    if (code == 0) return;
    buzzer_click();
    show_error_popup(code);
}

static void update_error_page(void)
{
    int shown = 0;
    for (int i = 0; i < 8 && shown < MAX_ERROR_DISPLAY; i++) {
        uint8_t code = lps.failure_codes[i];
        if (code == 0) continue;
        const error_def_t *def = lookup_error(code);
        error_row_codes[shown] = code;
        lv_obj_clear_flag(error_rows[shown], LV_OBJ_FLAG_HIDDEN);

        if (def) {
            lv_color_t col = get_error_level_color(def->level);
            lv_obj_set_style_border_color(error_rows[shown], col, 0);
            lv_obj_set_style_bg_color(error_level_dots[shown], col, 0);
            char title_buf[64];
            snprintf(title_buf, sizeof(title_buf), "%s  [%s]", def->title, get_error_level_str(def->level));
            lv_label_set_text(error_title_labels[shown], title_buf);
            lv_label_set_text(error_desc_labels[shown], def->desc);
        } else {
            lv_obj_set_style_border_color(error_rows[shown], COL_ORANGE, 0);
            lv_obj_set_style_bg_color(error_level_dots[shown], COL_ORANGE, 0);
            char title_buf[32];
            snprintf(title_buf, sizeof(title_buf), "E%03d Unknown", code);
            lv_label_set_text(error_title_labels[shown], title_buf);
            lv_label_set_text(error_desc_labels[shown], "Contact your retailer for support.");
        }
        shown++;
    }
    for (int i = shown; i < MAX_ERROR_DISPLAY; i++) {
        lv_obj_add_flag(error_rows[i], LV_OBJ_FLAG_HIDDEN);
        error_row_codes[i] = 0;
    }
    if (shown == 0) lv_obj_clear_flag(lbl_no_errors, LV_OBJ_FLAG_HIDDEN);
    else            lv_obj_add_flag(lbl_no_errors, LV_OBJ_FLAG_HIDDEN);
}

// ---------------------------------------------------------------------------
//  Error Popup
// ---------------------------------------------------------------------------
static void error_popup_clear_cb(lv_event_t *e)
{
    (void)e; buzzer_click();
    can_set_value(1, 252, 1234);
    error_flags[active_error_code].active = 0;
    error_flags[active_error_code].minimized = 0;
    error_popup_visible = false;
    lv_obj_add_flag(error_popup, LV_OBJ_FLAG_HIDDEN);
}

static void error_popup_minimize_cb(lv_event_t *e)
{
    (void)e; buzzer_click();
    error_flags[active_error_code].minimized = 1;
    error_popup_visible = false;
    lv_obj_add_flag(error_popup, LV_OBJ_FLAG_HIDDEN);
}

static void create_error_popup(lv_obj_t *parent)
{
    error_popup = lv_obj_create(parent);
    lv_obj_set_size(error_popup, 560, 340);
    lv_obj_center(error_popup);
    lv_obj_set_style_bg_color(error_popup, lv_color_hex(0x1a1018), 0);
    lv_obj_set_style_bg_opa(error_popup, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(error_popup, 2, 0);
    lv_obj_set_style_border_color(error_popup, COL_RED, 0);
    lv_obj_set_style_radius(error_popup, 14, 0);
    lv_obj_set_style_pad_all(error_popup, 20, 0);
    lv_obj_clear_flag(error_popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(error_popup, LV_OBJ_FLAG_HIDDEN);

    error_popup_level = lv_label_create(error_popup);
    lv_label_set_text(error_popup_level, LV_SYMBOL_WARNING "  CRITICAL");
    lv_obj_set_style_text_color(error_popup_level, COL_RED, 0);
    lv_obj_set_style_text_font(error_popup_level, &lv_font_montserrat_20, 0);
    lv_obj_align(error_popup_level, LV_ALIGN_TOP_MID, 0, 0);

    error_popup_title = lv_label_create(error_popup);
    lv_label_set_text(error_popup_title, "");
    lv_obj_set_style_text_color(error_popup_title, COL_TEXT, 0);
    lv_obj_set_style_text_font(error_popup_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(error_popup_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(error_popup_title, 510);
    lv_obj_align(error_popup_title, LV_ALIGN_TOP_MID, 0, 40);

    error_popup_desc = lv_label_create(error_popup);
    lv_label_set_text(error_popup_desc, "");
    lv_obj_set_style_text_color(error_popup_desc, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(error_popup_desc, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(error_popup_desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(error_popup_desc, 510);
    lv_label_set_long_mode(error_popup_desc, LV_LABEL_LONG_WRAP);
    lv_obj_align(error_popup_desc, LV_ALIGN_TOP_MID, 0, 90);

    error_popup_code = lv_label_create(error_popup);
    lv_label_set_text(error_popup_code, "Error code: 000");
    lv_obj_set_style_text_color(error_popup_code, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(error_popup_code, &lv_font_montserrat_14, 0);
    lv_obj_align(error_popup_code, LV_ALIGN_TOP_MID, 0, 160);

    lv_obj_t *btn_clr = lv_btn_create(error_popup);
    lv_obj_set_size(btn_clr, 200, 56);
    lv_obj_align(btn_clr, LV_ALIGN_BOTTOM_LEFT, 10, -6);
    lv_obj_set_style_bg_color(btn_clr, COL_RED, 0);
    lv_obj_set_style_radius(btn_clr, 10, 0);
    lv_obj_t *lb_clr = lv_label_create(btn_clr);
    lv_label_set_text(lb_clr, LV_SYMBOL_TRASH "  Clear");
    lv_obj_set_style_text_font(lb_clr, &lv_font_montserrat_20, 0);
    lv_obj_center(lb_clr);
    lv_obj_add_event_cb(btn_clr, error_popup_clear_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_ret = lv_btn_create(error_popup);
    lv_obj_set_size(btn_ret, 200, 56);
    lv_obj_align(btn_ret, LV_ALIGN_BOTTOM_RIGHT, -10, -6);
    lv_obj_set_style_bg_color(btn_ret, COL_BG_CARD, 0);
    lv_obj_set_style_radius(btn_ret, 10, 0);
    lv_obj_t *lb_ret = lv_label_create(btn_ret);
    lv_label_set_text(lb_ret, LV_SYMBOL_LEFT "  Return");
    lv_obj_set_style_text_font(lb_ret, &lv_font_montserrat_20, 0);
    lv_obj_center(lb_ret);
    lv_obj_add_event_cb(btn_ret, error_popup_minimize_cb, LV_EVENT_CLICKED, NULL);
}

static void show_error_popup(uint8_t code)
{
    active_error_code = code;
    error_popup_visible = true;
    const error_def_t *def = lookup_error(code);
    if (def) {
        lv_label_set_text_fmt(error_popup_level, LV_SYMBOL_WARNING "  %s", get_error_level_str(def->level));
        lv_color_t col = get_error_level_color(def->level);
        lv_obj_set_style_text_color(error_popup_level, col, 0);
        lv_obj_set_style_border_color(error_popup, col, 0);
        lv_label_set_text(error_popup_title, def->title);
        lv_label_set_text(error_popup_desc, def->desc);
    } else {
        lv_label_set_text(error_popup_level, LV_SYMBOL_WARNING "  UNKNOWN");
        lv_obj_set_style_text_color(error_popup_level, COL_ORANGE, 0);
        lv_obj_set_style_border_color(error_popup, COL_ORANGE, 0);
        char tbuf[32];
        snprintf(tbuf, sizeof(tbuf), "E%03d Unknown", code);
        lv_label_set_text(error_popup_title, tbuf);
        lv_label_set_text(error_popup_desc, "Contact your retailer for support.");
    }
    lv_label_set_text_fmt(error_popup_code, "Error code: %d", code);
    lv_obj_clear_flag(error_popup, LV_OBJ_FLAG_HIDDEN);
}

static bool check_for_error_popup(void)
{
    for (int i = 0; i < 8; i++) {
        uint8_t code = lps.failure_codes[i];
        if (code == 0) continue;
        if (!error_flags[code].active) continue;
        if (error_flags[code].minimized) continue;
        const error_def_t *def = lookup_error(code);
        uint8_t pl = def ? def->pop_level : POP_AUTO;
        if (pl == POP_HIDE) continue;
        return true;
    }
    return false;
}

static uint8_t get_popup_error_code(void)
{
    for (int i = 0; i < 8; i++) {
        uint8_t code = lps.failure_codes[i];
        if (code == 0) continue;
        if (!error_flags[code].active) continue;
        if (error_flags[code].minimized) continue;
        const error_def_t *def = lookup_error(code);
        uint8_t pl = def ? def->pop_level : POP_AUTO;
        if (pl == POP_HIDE) continue;
        return code;
    }
    return 0;
}

static bool check_popup_auto_cleared(void)
{
    if (!error_popup_visible) return false;
    const error_def_t *def = lookup_error(active_error_code);
    uint8_t pl = def ? def->pop_level : POP_AUTO;
    if (pl != POP_AUTO) return false;
    for (int i = 0; i < 8; i++)
        if (lps.failure_codes[i] == active_error_code) return false;
    return true;
}

// ---------------------------------------------------------------------------
//  Toggle callbacks
// ---------------------------------------------------------------------------
#define FUNC_BLOCK        0
#define FUNC_INVERTER_ID  158
#define FUNC_DCOUT_ID     159

static void btn_inv_toggle_cb(lv_event_t *e)
{
    (void)e;
    if (selected_unit >= 0 && unit_table[selected_unit].device_type == DEV_BMS) return;
    buzzer_click();
    int32_t val = (lps.inverter_state >= 1) ? 0 : FLOAT_TO_Q16(1);
    can_send_command(CAN_CMD_SET_VAL, FUNC_BLOCK, FUNC_INVERTER_ID, val);
}

static void btn_dcout_toggle_cb(lv_event_t *e)
{
    (void)e; buzzer_click();
    int32_t val = (lps.dc_output_state >= 1) ? 0 : FLOAT_TO_Q16(1);
    can_send_command(CAN_CMD_SET_VAL, FUNC_BLOCK, FUNC_DCOUT_ID, val);
}

// Device selector callbacks
static void refresh_error_flags_for_unit(void)
{
    memset(error_flags, 0, sizeof(error_flags));
    if (selected_unit < 0) return;
    lps_data_t *d = &unit_table[selected_unit].data;
    for (int i = 0; i < 8; i++)
        if (d->failure_codes[i] != 0)
            error_flags[d->failure_codes[i]].active = 1;
}

static void reset_all_settings_received(void)
{
    for (int c = 0; c < (int)NUM_CATEGORIES_LPS; c++)
        for (int s = 0; s < menu_categories_lps[c].count; s++)
            menu_categories_lps[c].settings[s].val_received = false;
    for (int c = 0; c < (int)NUM_CATEGORIES_BMS; c++)
        for (int s = 0; s < menu_categories_bms[c].count; s++)
            menu_categories_bms[c].settings[s].val_received = false;
}

static void btn_dev_prev_cb(lv_event_t *e)
{
    (void)e; buzzer_click();
    select_prev_unit();
    refresh_error_flags_for_unit();
    if (selected_unit >= 0) switch_active_menu(unit_table[selected_unit].device_type);
    reset_all_settings_received();
}

static void btn_dev_next_cb(lv_event_t *e)
{
    (void)e; buzzer_click();
    select_next_unit();
    refresh_error_flags_for_unit();
    if (selected_unit >= 0) switch_active_menu(unit_table[selected_unit].device_type);
    reset_all_settings_received();
}

static void update_device_selector(void)
{
    int count = unit_online_count();
    if (count <= 1) {
        lv_obj_add_flag(dev_sel_container, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_clear_flag(dev_sel_container, LV_OBJ_FLAG_HIDDEN);

    int unit_num = 0, pos = 0;
    for (int i = 0; i < UNIT_TABLE_SIZE; i++) {
        if (unit_table[i].status != UNIT_NULL) {
            pos++;
            if (i == selected_unit) unit_num = pos;
        }
    }

    char buf[80];
    if (selected_unit >= 0) {
        unit_entry_t *u = &unit_table[selected_unit];
        if (u->id_complete && u->part_number[0]) {
            if (u->serial_str[0])
                snprintf(buf, sizeof(buf), "%d/%d %s (%s)", unit_num, count, u->part_number, u->serial_str);
            else
                snprintf(buf, sizeof(buf), "%d/%d %s", unit_num, count, u->part_number);
        } else {
            snprintf(buf, sizeof(buf), "%d/%d (0x%02X)", unit_num, count, u->can_addr);
        }
    } else
        snprintf(buf, sizeof(buf), "No Unit");
    lv_label_set_text(lbl_dev_name, buf);

    if (selected_unit >= 0 && unit_table[selected_unit].status == UNIT_ONLINE)
        lv_obj_set_style_bg_color(lbl_dev_status, COL_GREEN, 0);
    else
        lv_obj_set_style_bg_color(lbl_dev_status, COL_TEXT_DIM, 0);
}

// ---------------------------------------------------------------------------
//  Dashboard UI Construction (1024×600 landscape)
// ---------------------------------------------------------------------------
static void create_dashboard(lv_obj_t *parent)
{
    page_dashboard = lv_obj_create(parent);
    lv_obj_set_size(page_dashboard, SCREEN_W, SCREEN_H);
    lv_obj_set_style_bg_color(page_dashboard, COL_BG_DARK, 0);
    lv_obj_set_style_bg_opa(page_dashboard, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(page_dashboard, 0, 0);
    lv_obj_set_style_pad_all(page_dashboard, 0, 0);
    lv_obj_set_style_radius(page_dashboard, 0, 0);
    lv_obj_align(page_dashboard, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_clear_flag(page_dashboard, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(page_dashboard, LV_OBJ_FLAG_CLICKABLE);

    // SOC Arc — dominant, centered
    arc_soc = lv_arc_create(page_dashboard);
    lv_obj_set_size(arc_soc, 380, 380);
    lv_arc_set_rotation(arc_soc, 135);
    lv_arc_set_bg_angles(arc_soc, 0, 270);
    lv_arc_set_range(arc_soc, 0, 100);
    lv_arc_set_value(arc_soc, 0);
    lv_obj_align(arc_soc, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_remove_style(arc_soc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_soc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc_soc, COL_SOC_BG, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_soc, 26, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_soc, COL_SOC_ARC, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_soc, 26, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc_soc, true, LV_PART_INDICATOR);

    // SOC percentage — large, inside arc
    lbl_soc_pct = lv_label_create(page_dashboard);
    lv_label_set_text(lbl_soc_pct, "--%");
    lv_obj_set_style_text_color(lbl_soc_pct, COL_TEXT, 0);
    lv_obj_set_style_text_font(lbl_soc_pct, &lv_font_montserrat_48, 0);
    lv_obj_align(lbl_soc_pct, LV_ALIGN_TOP_MID, 0, 150);

    // Time left
    lbl_time_left = lv_label_create(page_dashboard);
    lv_label_set_text(lbl_time_left, "-- min");
    lv_obj_set_style_text_color(lbl_time_left, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(lbl_time_left, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl_time_left, LV_ALIGN_TOP_MID, 0, 210);

    // Battery voltage + current
    lbl_batt_info = lv_label_create(page_dashboard);
    lv_label_set_text(lbl_batt_info, "--.- V   --.- A");
    lv_obj_set_style_text_color(lbl_batt_info, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(lbl_batt_info, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_batt_info, LV_ALIGN_TOP_MID, 0, 400);

    // Toggle buttons
    btn_inv_toggle = lv_btn_create(page_dashboard);
    lv_obj_set_size(btn_inv_toggle, 300, 56);
    lv_obj_align(btn_inv_toggle, LV_ALIGN_BOTTOM_LEFT, 100, -16);
    lv_obj_set_style_bg_color(btn_inv_toggle, COL_BG_CARD, 0);
    lv_obj_set_style_radius(btn_inv_toggle, 10, 0);
    lv_obj_set_style_border_width(btn_inv_toggle, 2, 0);
    lv_obj_set_style_border_color(btn_inv_toggle, COL_TEXT_DIM, 0);
    lbl_inv_toggle = lv_label_create(btn_inv_toggle);
    lv_label_set_text(lbl_inv_toggle, LV_SYMBOL_POWER " INVERTER");
    lv_obj_set_style_text_font(lbl_inv_toggle, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_inv_toggle);
    lv_obj_add_event_cb(btn_inv_toggle, btn_inv_toggle_cb, LV_EVENT_CLICKED, NULL);

    btn_dcout_toggle = lv_btn_create(page_dashboard);
    lv_obj_set_size(btn_dcout_toggle, 300, 56);
    lv_obj_align(btn_dcout_toggle, LV_ALIGN_BOTTOM_RIGHT, -100, -16);
    lv_obj_set_style_bg_color(btn_dcout_toggle, COL_BG_CARD, 0);
    lv_obj_set_style_radius(btn_dcout_toggle, 10, 0);
    lv_obj_set_style_border_width(btn_dcout_toggle, 2, 0);
    lv_obj_set_style_border_color(btn_dcout_toggle, COL_TEXT_DIM, 0);
    lbl_dcout_toggle = lv_label_create(btn_dcout_toggle);
    lv_label_set_text(lbl_dcout_toggle, LV_SYMBOL_DOWNLOAD " DC OUT");
    lv_obj_set_style_text_font(lbl_dcout_toggle, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_dcout_toggle);
    lv_obj_add_event_cb(btn_dcout_toggle, btn_dcout_toggle_cb, LV_EVENT_CLICKED, NULL);

    // Settings gear (top-right)
    btn_settings = lv_btn_create(page_dashboard);
    lv_obj_set_size(btn_settings, 48, 48);
    lv_obj_align(btn_settings, LV_ALIGN_TOP_RIGHT, -10, 8);
    lv_obj_set_style_bg_color(btn_settings, COL_ACCENT, 0);
    lv_obj_set_style_radius(btn_settings, 24, 0);
    lv_obj_t *gear = lv_label_create(btn_settings);
    lv_label_set_text(gear, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(gear, &lv_font_montserrat_24, 0);
    lv_obj_center(gear);
    lv_obj_add_event_cb(btn_settings, btn_settings_cb, LV_EVENT_CLICKED, NULL);

    // Device selector
    dev_sel_container = lv_obj_create(page_dashboard);
    lv_obj_set_size(dev_sel_container, 300, 40);
    lv_obj_align(dev_sel_container, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_color(dev_sel_container, COL_BG_CARD, 0);
    lv_obj_set_style_bg_opa(dev_sel_container, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dev_sel_container, 20, 0);
    lv_obj_set_style_border_width(dev_sel_container, 1, 0);
    lv_obj_set_style_border_color(dev_sel_container, lv_color_hex(0x30363d), 0);
    lv_obj_set_style_pad_all(dev_sel_container, 0, 0);
    lv_obj_clear_flag(dev_sel_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dev_sel_container, LV_OBJ_FLAG_HIDDEN);

    btn_dev_prev = lv_btn_create(dev_sel_container);
    lv_obj_set_size(btn_dev_prev, 40, 36);
    lv_obj_align(btn_dev_prev, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_set_style_bg_opa(btn_dev_prev, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(btn_dev_prev, 0, 0);
    lv_obj_t *lbl_prev = lv_label_create(btn_dev_prev);
    lv_label_set_text(lbl_prev, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(lbl_prev, COL_ACCENT, 0);
    lv_obj_set_style_text_font(lbl_prev, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl_prev);
    lv_obj_add_event_cb(btn_dev_prev, btn_dev_prev_cb, LV_EVENT_CLICKED, NULL);

    lbl_dev_status = lv_obj_create(dev_sel_container);
    lv_obj_set_size(lbl_dev_status, 10, 10);
    lv_obj_set_style_radius(lbl_dev_status, 5, 0);
    lv_obj_set_style_bg_color(lbl_dev_status, COL_GREEN, 0);
    lv_obj_set_style_bg_opa(lbl_dev_status, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(lbl_dev_status, 0, 0);
    lv_obj_align(lbl_dev_status, LV_ALIGN_LEFT_MID, 46, 0);
    lv_obj_clear_flag(lbl_dev_status, LV_OBJ_FLAG_SCROLLABLE);

    lbl_dev_name = lv_label_create(dev_sel_container);
    lv_label_set_text(lbl_dev_name, "Unit 1/1");
    lv_obj_set_style_text_color(lbl_dev_name, COL_TEXT, 0);
    lv_obj_set_style_text_font(lbl_dev_name, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_dev_name, LV_ALIGN_CENTER, 4, 0);

    btn_dev_next = lv_btn_create(dev_sel_container);
    lv_obj_set_size(btn_dev_next, 40, 36);
    lv_obj_align(btn_dev_next, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_obj_set_style_bg_opa(btn_dev_next, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(btn_dev_next, 0, 0);
    lv_obj_t *lbl_next = lv_label_create(btn_dev_next);
    lv_label_set_text(lbl_next, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(lbl_next, COL_ACCENT, 0);
    lv_obj_set_style_text_font(lbl_next, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl_next);
    lv_obj_add_event_cb(btn_dev_next, btn_dev_next_cb, LV_EVENT_CLICKED, NULL);

    // Charging source indicators
    lbl_icon_ac = lv_label_create(page_dashboard);
    lv_label_set_text(lbl_icon_ac, LV_SYMBOL_CHARGE " AC");
    lv_obj_set_style_text_color(lbl_icon_ac, COL_GREEN, 0);
    lv_obj_set_style_text_font(lbl_icon_ac, &lv_font_montserrat_20, 0);
    lv_obj_add_flag(lbl_icon_ac, LV_OBJ_FLAG_HIDDEN);

    lbl_icon_dc = lv_label_create(page_dashboard);
    lv_label_set_text(lbl_icon_dc, LV_SYMBOL_UPLOAD " DC");
    lv_obj_set_style_text_color(lbl_icon_dc, COL_GREEN, 0);
    lv_obj_set_style_text_font(lbl_icon_dc, &lv_font_montserrat_20, 0);
    lv_obj_add_flag(lbl_icon_dc, LV_OBJ_FLAG_HIDDEN);

    lbl_icon_solar = lv_label_create(page_dashboard);
    lv_label_set_text(lbl_icon_solar, LV_SYMBOL_IMAGE " Solar");
    lv_obj_set_style_text_color(lbl_icon_solar, COL_SOLAR, 0);
    lv_obj_set_style_text_font(lbl_icon_solar, &lv_font_montserrat_20, 0);
    lv_obj_add_flag(lbl_icon_solar, LV_OBJ_FLAG_HIDDEN);

    // Error warning badge (top-left)
    btn_error_badge = lv_btn_create(page_dashboard);
    lv_obj_set_size(btn_error_badge, 50, 44);
    lv_obj_align(btn_error_badge, LV_ALIGN_TOP_LEFT, 10, 8);
    lv_obj_set_style_bg_color(btn_error_badge, COL_RED, 0);
    lv_obj_set_style_radius(btn_error_badge, 8, 0);
    lv_obj_set_style_border_width(btn_error_badge, 0, 0);
    lbl_error_badge = lv_label_create(btn_error_badge);
    lv_label_set_text(lbl_error_badge, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_font(lbl_error_badge, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_error_badge, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_error_badge);
    lv_obj_add_event_cb(btn_error_badge, btn_error_badge_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(btn_error_badge, LV_OBJ_FLAG_HIDDEN);
}

// ---------------------------------------------------------------------------
//  Dashboard UI Update
// ---------------------------------------------------------------------------
static void update_dashboard(void)
{
    char buf[64];
    update_device_selector();

    uint32_t now = now_ms();
    if (lps.connected && (now - lps.last_msg_time_ms > 3000))
        lps.connected = false;

    // SOC Arc
    int soc_val = (int)(lps.soc_percent + 0.5f);
    if (soc_val < 0) soc_val = 0;
    if (soc_val > 100) soc_val = 100;
    lv_arc_set_value(arc_soc, soc_val);
    lv_color_t soc_col = get_soc_color(lps.soc_percent);
    lv_obj_set_style_arc_color(arc_soc, soc_col, LV_PART_INDICATOR);
    snprintf(buf, sizeof(buf), "%d%%", soc_val);
    lv_label_set_text(lbl_soc_pct, buf);
    lv_obj_set_style_text_color(lbl_soc_pct, soc_col, 0);

    // Time left
    {
        bool charging = lps.battery_current_a < -0.5f;
        bool discharging = lps.battery_current_a > 0.5f;
        int time_min = lps.soc_time_min < 0 ? -lps.soc_time_min : lps.soc_time_min;
        if (time_min > 0) {
            int hrs = time_min / 60, mins = time_min % 60;
            const char *arrow = charging ? LV_SYMBOL_UP : LV_SYMBOL_DOWN;
            if (hrs > 0) snprintf(buf, sizeof(buf), "%s %dh %dm", arrow, hrs, mins);
            else         snprintf(buf, sizeof(buf), "%s %d min", arrow, mins);
            lv_obj_set_style_text_color(lbl_time_left, charging ? COL_GREEN : COL_TEXT_DIM, 0);
        } else if (charging) {
            snprintf(buf, sizeof(buf), LV_SYMBOL_UP " Charging");
            lv_obj_set_style_text_color(lbl_time_left, COL_GREEN, 0);
        } else if (discharging) {
            snprintf(buf, sizeof(buf), LV_SYMBOL_DOWN " ---");
            lv_obj_set_style_text_color(lbl_time_left, COL_TEXT_DIM, 0);
        } else {
            snprintf(buf, sizeof(buf), "Standby");
            lv_obj_set_style_text_color(lbl_time_left, COL_TEXT_DIM, 0);
        }
        lv_label_set_text(lbl_time_left, buf);
    }

    // Battery info
    snprintf(buf, sizeof(buf), "%.1f V   %+.1f A",
             (double)lps.battery_voltage_v, (double)lps.battery_current_a);
    lv_label_set_text(lbl_batt_info, buf);

    // Toggle buttons
    device_type_t cur_dev_type = DEV_UNKNOWN;
    if (selected_unit >= 0) cur_dev_type = unit_table[selected_unit].device_type;

    if (cur_dev_type == DEV_BMS) {
        lv_obj_add_flag(btn_inv_toggle, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(btn_dcout_toggle, 600, 56);
        lv_obj_align(btn_dcout_toggle, LV_ALIGN_BOTTOM_MID, 0, -16);
    } else {
        lv_obj_clear_flag(btn_inv_toggle, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(btn_inv_toggle, 300, 56);
        lv_obj_align(btn_inv_toggle, LV_ALIGN_BOTTOM_LEFT, 100, -16);
        lv_obj_set_size(btn_dcout_toggle, 300, 56);
        lv_obj_align(btn_dcout_toggle, LV_ALIGN_BOTTOM_RIGHT, -100, -16);
    }

    // Inverter state
    if (cur_dev_type != DEV_BMS) {
        lv_color_t bg, border, txt_col;
        const char *text;
        int8_t  st = lps.inverter_state;
        uint8_t fl = lps.inverter_failure;
        if (fl >= 2 && st >= 1) {
            bg = (fl >= 3) ? COL_RED : COL_ORANGE; border = bg;
            txt_col = lv_color_hex(0xffffff); text = LV_SYMBOL_POWER " INVERTER  !";
        } else if (st >= 5) {
            bg = COL_GREEN; border = COL_GREEN;
            txt_col = lv_color_hex(0xffffff); text = LV_SYMBOL_POWER " INVERTER ON";
        } else if (st >= 1) {
            bg = COL_BG_CARD; border = COL_ORANGE;
            txt_col = COL_ORANGE; text = LV_SYMBOL_POWER " INVERTER ...";
        } else {
            bg = COL_BG_CARD; border = COL_TEXT_DIM;
            txt_col = COL_TEXT_DIM; text = LV_SYMBOL_POWER " INVERTER";
        }
        lv_obj_set_style_bg_color(btn_inv_toggle, bg, 0);
        lv_obj_set_style_border_color(btn_inv_toggle, border, 0);
        lv_label_set_text(lbl_inv_toggle, text);
        lv_obj_set_style_text_color(lbl_inv_toggle, txt_col, 0);
    }

    // DC Output state
    {
        lv_color_t bg, border, txt_col;
        const char *text;
        int8_t  st = lps.dc_output_state;
        uint8_t fl = lps.dc_output_failure;
        if (fl >= 2 && st >= 1) {
            bg = (fl >= 3) ? COL_RED : COL_ORANGE; border = bg;
            txt_col = lv_color_hex(0xffffff); text = LV_SYMBOL_DOWNLOAD " DC OUT  !";
        } else if (st >= 5) {
            bg = COL_GREEN; border = COL_GREEN;
            txt_col = lv_color_hex(0xffffff); text = LV_SYMBOL_DOWNLOAD " DC OUT ON";
        } else if (st >= 1) {
            bg = COL_BG_CARD; border = COL_ORANGE;
            txt_col = COL_ORANGE; text = LV_SYMBOL_DOWNLOAD " DC OUT ...";
        } else {
            bg = COL_BG_CARD; border = COL_TEXT_DIM;
            txt_col = COL_TEXT_DIM; text = LV_SYMBOL_DOWNLOAD " DC OUT";
        }
        lv_obj_set_style_bg_color(btn_dcout_toggle, bg, 0);
        lv_obj_set_style_border_color(btn_dcout_toggle, border, 0);
        lv_label_set_text(lbl_dcout_toggle, text);
        lv_obj_set_style_text_color(lbl_dcout_toggle, txt_col, 0);
    }

    // Charging source icons
    if (cur_dev_type == DEV_BMS) {
        lv_obj_add_flag(lbl_icon_ac, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(lbl_icon_dc, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(lbl_icon_solar, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_t *icons[] = {lbl_icon_ac, lbl_icon_dc, lbl_icon_solar};
        int8_t  states[]   = {lps.charger_state, lps.dc_input_state, lps.solar_state};
        uint8_t failures[] = {lps.charger_failure, lps.dc_input_failure, lps.solar_failure};
        lv_color_t ok_colors[] = {COL_GREEN, COL_GREEN, COL_SOLAR};
        const char *texts_ok[]   = {LV_SYMBOL_CHARGE " AC", LV_SYMBOL_UPLOAD " DC", LV_SYMBOL_IMAGE " Solar"};
        const char *texts_err[]  = {LV_SYMBOL_CHARGE " AC !", LV_SYMBOL_UPLOAD " DC !", LV_SYMBOL_IMAGE " Solar !"};
        const char *texts_wait[] = {LV_SYMBOL_CHARGE " AC ...", LV_SYMBOL_UPLOAD " DC ...", LV_SYMBOL_IMAGE " Solar ..."};

        bool visible[3];
        for (int i = 0; i < 3; i++) {
            visible[i] = (states[i] >= 1);
            if (visible[i]) {
                lv_color_t col; const char *txt;
                if (failures[i] >= 2 && states[i] >= 1) {
                    col = (failures[i] >= 3) ? COL_RED : COL_ORANGE; txt = texts_err[i];
                } else if (states[i] >= 3) {
                    col = ok_colors[i]; txt = texts_ok[i];
                } else {
                    col = COL_ORANGE; txt = texts_wait[i];
                }
                lv_obj_set_style_text_color(icons[i], col, 0);
                lv_label_set_text(icons[i], txt);
            }
        }

        int n_active = 0;
        for (int i = 0; i < 3; i++) if (visible[i]) n_active++;
        int spacing = 120;
        int start_x = -(n_active - 1) * spacing / 2;
        int pos = 0;
        for (int i = 0; i < 3; i++) {
            if (visible[i]) {
                lv_obj_clear_flag(icons[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_align(icons[i], LV_ALIGN_TOP_MID, start_x + pos * spacing, 440);
                pos++;
            } else {
                lv_obj_add_flag(icons[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    // Error badge
    if (lps.failure_code_count > 0) {
        lv_obj_clear_flag(btn_error_badge, LV_OBJ_FLAG_HIDDEN);
        lv_color_t badge_col = COL_ORANGE;
        if (lps.failure_level >= FL_SIMPLE_FAILURE) badge_col = COL_RED;
        lv_obj_set_style_bg_color(btn_error_badge, badge_col, 0);
    } else {
        lv_obj_add_flag(btn_error_badge, LV_OBJ_FLAG_HIDDEN);
    }
}

// ---------------------------------------------------------------------------
//  Power Management — sleep/wake state machine
// ---------------------------------------------------------------------------
//
// Flow:
//  Idle detected:
//    PWR_ACTIVE → splash 2s → display off (PWR_SLEEPING)
//  Wake (touch or CAN activity):
//    PWR_SLEEPING → splash 2s → dashboard 30s (PWR_DASHBOARD)
//  Dashboard timeout (no touch for 30s):
//    PWR_DASHBOARD → splash 2s → display off (PWR_SLEEPING)

#define PWR_SPLASH_MS         2000
#define PWR_DASHBOARD_TIMEOUT 30000
#define PWR_STARTUP_GRACE_MS  15000   // Don't check idle for first 15s after boot
#define PWR_IDLE_DEBOUNCE_MS  5000    // Must be idle for 5s straight before sleep

enum {
    PWR_ACTIVE,         // Normal operation — display on
    PWR_SLEEP_SPLASH,   // Showing "CLAYTON POWER" 2s before sleeping
    PWR_SLEEPING,       // Display off, only CAN polling
    PWR_WAKE_SPLASH,    // Waking up — showing splash for 2s
    PWR_DASHBOARD,      // Dashboard shown for 30s after wake
    PWR_RELEEP_SPLASH,  // Showing splash 2s before re-sleeping
};

static int  pwr_state = PWR_ACTIVE;
static uint32_t pwr_state_start_ms  = 0;
static uint32_t pwr_last_touch_ms   = 0;
static uint32_t pwr_boot_time_ms    = 0;    // Set once at init
static uint32_t pwr_idle_since_ms   = 0;    // When idle was first detected
static bool     pwr_was_idle        = false; // For debounce tracking
static bool     pwr_skip_grace      = false; // True when waking from deep sleep
static lv_obj_t *pwr_splash_scr     = NULL;

// Check if ALL connected units are idle (requires real status data)
static bool lps_is_idle(void)
{
    bool any_connected = false;
    for (int i = 0; i < UNIT_TABLE_SIZE; i++) {
        if (unit_table[i].status != UNIT_ONLINE) continue;
        if (!unit_table[i].data.connected) continue;
        // Require at least some CAN messages before trusting state data
        if (unit_table[i].data.msg_count < 10) return false;
        any_connected = true;
        lps_data_t *d = &unit_table[i].data;
        if (unit_table[i].device_type == DEV_BMS) {
            uint8_t os = (uint8_t)d->operating_state;
            if (os > 0x10) return false;
        } else {
            if (d->inverter_state >= 1 || d->charger_state >= 1 ||
                d->dc_input_state >= 1 || d->dc_output_state >= 1)
                return false;
        }
    }
    return any_connected;
}

// Check if touch panel was recently active (within last 1s)
// Uses LVGL display activity tracker — far more reliable than polling
// instantaneous indev->proc.state which can be missed between ticks.
// NOTE: Only valid when LVGL task is running (not during PWR_SLEEPING).
static bool touch_is_pressed(void)
{
    uint32_t inactive = lv_disp_get_inactive_time(NULL);
    return (inactive < 1000);
}

// Direct touch poll for sleep mode — GT911 stays running (not slept).
// Simpler and more reliable: just read the touch register.
// Direct touch poll for sleep mode — GT911 stays running.
// Caller is responsible for CPU frequency; no switching here.
static bool touch_is_pressed_direct(void)
{
    static uint32_t poll_count = 0;
    poll_count++;

    bool pressed = waveshare_touch_is_pressed();

    if (pressed || (poll_count % 5) == 1) {
        ESP_LOGI(TAG, "[TOUCH] poll #%lu: pressed=%d",
                 (unsigned long)poll_count, pressed);
    }

    return pressed;
}

static void pwr_show_splash(void)
{
    if (pwr_splash_scr) return;  // Already showing
    lv_obj_t *scr = lv_scr_act();

    pwr_splash_scr = lv_obj_create(scr);
    lv_obj_remove_style_all(pwr_splash_scr);
    lv_obj_set_size(pwr_splash_scr, 1024, 600);
    lv_obj_set_style_bg_color(pwr_splash_scr, COL_BG_DARK, 0);
    lv_obj_set_style_bg_opa(pwr_splash_scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(pwr_splash_scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(pwr_splash_scr);
    lv_label_set_text(title, "CLAYTON POWER");
    lv_obj_set_style_text_color(title, COL_ACCENT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *sub = lv_label_create(pwr_splash_scr);
    lv_label_set_text(sub, "Standby");
    lv_obj_set_style_text_color(sub, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 24);

    // NOTE: Do NOT call lv_refr_now() here — it hangs the task
    // due to RGB LCD bounce buffer DMA contention. Let the normal
    // LVGL refresh cycle render the splash within ~33ms.
}

static void pwr_remove_splash(void)
{
    if (pwr_splash_scr) {
        lv_obj_del(pwr_splash_scr);
        pwr_splash_scr = NULL;
    }
}

static void pwr_display_off(void)
{
    // 1) Turn off backlight
    esp_err_t ret;
    for (int i = 0; i < 3; i++) {
        ret = wavesahre_rgb_lcd_bl_off();
        if (ret == ESP_OK) break;
        ESP_LOGW(TAG, "[PWR] BL off I2C retry %d (err=%s)", i, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGI(TAG, "[PWR] Display off (ret=%s)", esp_err_to_name(ret));

    // 2) Suspend LVGL task — stops rendering, DMA bounce, and touch polling
    lvgl_port_suspend();

    // 3) Wait 120ms for ST7262 internal voltage discharge (T1 ≥ 100ms).
    //    Datasheet: "do not interrupt [DISP power-off] procedure,
    //    otherwise unexpected errors will occur."
    //    Do NOT assert LCD_RST during this time — it aborts the charge-pump
    //    shutdown and the IC stays at ~50 mA instead of 50 µA standby.
    vTaskDelay(pdMS_TO_TICKS(120));

    // 4) Float all LCD GPIO pins — ST7262 is now in standby, safe to release
    waveshare_lcd_pins_float();

    // 5) Assert LCD_RST (IO3=LOW) — safe AFTER T1 discharge is complete.
    //    ST7262 in HW reset draws far less than standby.
    waveshare_lcd_reset_assert();

    // 6) GT911 touch stays running — needed for reliable touch-wake detection.

    // 7) Set CH422G outputs low (except CTP_RST which stays HIGH for GT911)
    waveshare_ch422g_all_low();

    // 8) Stop TWAI — releases PM lock
    twai_stop();
    ESP_LOGI(TAG, "[PWR] TWAI stopped");

    // 9) Drop CPU to 80 MHz — BLE needs APB clock, can't go to 10 MHz
    esp_pm_config_t pm_cfg = {
        .max_freq_mhz = 80,
        .min_freq_mhz = 80,
        .light_sleep_enable = false,
    };
    esp_pm_configure(&pm_cfg);
    ESP_LOGI(TAG, "[PWR] CPU 80 MHz — sleeping (BLE active)");

    pwr_state = PWR_SLEEPING;
}

static void pwr_display_on(void)
{
    // 1) Restore CPU clock to 240MHz
    esp_pm_config_t pm_cfg = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 240,
        .light_sleep_enable = false,
    };
    esp_err_t ret = esp_pm_configure(&pm_cfg);
    ESP_LOGI(TAG, "[PWR] CPU 240MHz (ret=%s)", esp_err_to_name(ret));

    // 2) Start TWAI
    twai_start();

    // 3) Wake CH422G + release LCD_RST + CTP_RST (DISP stays LOW)
    waveshare_ch422g_wake();
    waveshare_lcd_reset_release();   // Sets IO1+IO3+IO4 HIGH, IO2(DISP) LOW
    ESP_LOGI(TAG, "[PWR] CH422G awake, LCD_RST released");

    // 4) GT911 was kept running — no wake needed.

    // 5) Restore LCD pins + restart panel DMA
    waveshare_lcd_pins_drive();

    // 6) Resume LVGL task — restarts rendering + touch polling
    lvgl_port_resume();

    // 7) Turn on backlight (DISP=HIGH → ST7262 power-on sequence)
    for (int i = 0; i < 3; i++) {
        ret = wavesahre_rgb_lcd_bl_on();
        if (ret == ESP_OK) break;
        ESP_LOGW(TAG, "[PWR] BL on I2C retry %d (err=%s)", i, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGI(TAG, "[PWR] Display on (ret=%s)", esp_err_to_name(ret));
}

// Returns true when display is off or showing splash (skip normal UI updates)
static bool pwr_management_tick(void)
{
    uint32_t now = now_ms();

    // Startup grace period — don't do idle detection for first 15s
    if (pwr_boot_time_ms == 0) {
        pwr_boot_time_ms = now;
        if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
            pwr_skip_grace = true;
            ESP_LOGI(TAG, "[PWR] Deep sleep wake — skipping grace period");
        }
    }
    if (pwr_state == PWR_ACTIVE && !pwr_skip_grace &&
        (now - pwr_boot_time_ms) < PWR_STARTUP_GRACE_MS)
        return false;

    // Keep the gateway awake while BLE raw CAN passthrough is active.
    // This avoids stop/start TWAI gaps that can drop bootloader traffic.
    if (ble_can_passthrough_enabled && ble_gateway_is_connected()) {
        pwr_last_touch_ms = now;
        pwr_was_idle = false;

        if (pwr_state == PWR_SLEEP_SPLASH || pwr_state == PWR_RELEEP_SPLASH) {
            pwr_remove_splash();
            pwr_state = PWR_DASHBOARD;
            ESP_LOGI(TAG, "[PWR] BLE passthrough active — canceling sleep splash");
            return false;
        }

        if (pwr_state == PWR_ACTIVE || pwr_state == PWR_DASHBOARD) {
            return false;
        }
    }

    // Idle debounce — only transition to sleep after 5s of continuous idle
    bool idle_now = lps_is_idle();
    if (pwr_state == PWR_ACTIVE) {
        if (idle_now) {
            if (!pwr_was_idle) {
                pwr_idle_since_ms = now;
                pwr_was_idle = true;
                ESP_LOGI(TAG, "[PWR] Idle detected, debouncing %dms", PWR_IDLE_DEBOUNCE_MS);
            }
            if ((now - pwr_idle_since_ms) < PWR_IDLE_DEBOUNCE_MS)
                return false;  // Still debouncing — normal operation
        } else {
            if (pwr_was_idle)
                ESP_LOGI(TAG, "[PWR] No longer idle, debounce reset");
            pwr_was_idle = false;
        }
    }

    switch (pwr_state) {

    case PWR_ACTIVE:
        if (pwr_was_idle) {
            pwr_show_splash();
            pwr_state_start_ms = now;
            pwr_state = PWR_SLEEP_SPLASH;
            pwr_was_idle = false;
            ESP_LOGI(TAG, "[PWR] System idle (debounced) — splash before sleep");
            return true;
        }
        return false;

    case PWR_SLEEP_SPLASH:
        if (!lps_is_idle()) {
            pwr_remove_splash();
            pwr_state = PWR_ACTIVE;
            ESP_LOGI(TAG, "[PWR] System active — cancel sleep");
            return false;
        }
        if (now - pwr_state_start_ms >= PWR_SPLASH_MS) {
            pwr_display_off();
        }
        return true;

    case PWR_SLEEPING: {
        bool sys_active = !lps_is_idle();
        bool touch = touch_is_pressed_direct();
        if (sys_active || touch) {
            ESP_LOGI(TAG, "[PWR] Wake (sys_active=%d, touch=%d)", sys_active, touch);
            pwr_display_on();
            pwr_remove_splash();
            pwr_show_splash();
            pwr_state_start_ms = now;
            pwr_state = PWR_WAKE_SPLASH;
            return true;
        }
        return true;
    }

    case PWR_WAKE_SPLASH:
        if (now - pwr_state_start_ms >= PWR_SPLASH_MS) {
            pwr_remove_splash();
            lv_obj_invalidate(lv_scr_act());
            if (!lps_is_idle()) {
                pwr_state = PWR_ACTIVE;
                ESP_LOGI(TAG, "[PWR] Splash done — system active");
                return false;
            } else {
                pwr_last_touch_ms = now;
                pwr_state = PWR_DASHBOARD;
                ESP_LOGI(TAG, "[PWR] Splash done — dashboard 30s");
            }
        }
        return true;

    case PWR_DASHBOARD:
        if (!lps_is_idle()) {
            pwr_state = PWR_ACTIVE;
            ESP_LOGI(TAG, "[PWR] System active — staying on");
            return false;
        }
        if (touch_is_pressed())
            pwr_last_touch_ms = now;
        if (now - pwr_last_touch_ms >= PWR_DASHBOARD_TIMEOUT) {
            pwr_show_splash();
            pwr_state_start_ms = now;
            pwr_state = PWR_RELEEP_SPLASH;
            ESP_LOGI(TAG, "[PWR] 30s timeout — splash before re-sleep");
            return true;
        }
        return false;

    case PWR_RELEEP_SPLASH:
        if (!lps_is_idle()) {
            pwr_remove_splash();
            pwr_state = PWR_ACTIVE;
            ESP_LOGI(TAG, "[PWR] System active — cancel re-sleep");
            return false;
        }
        if (touch_is_pressed()) {
            pwr_remove_splash();
            lv_obj_invalidate(lv_scr_act());
            pwr_last_touch_ms = now;
            pwr_state = PWR_DASHBOARD;
            ESP_LOGI(TAG, "[PWR] Touch — cancel re-sleep");
            return false;
        }
        if (now - pwr_state_start_ms >= PWR_SPLASH_MS) {
            pwr_display_off();
        }
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
//  BLE Command Handler — called from BLE RX thread
// ---------------------------------------------------------------------------

static void ble_cmd_handler(uint8_t cmd, const uint8_t *payload, uint16_t len)
{
    switch (cmd) {
    case BLE_CMD_SET_CAN_PASSTHROUGH:
        if (len >= 1) {
            ble_can_passthrough_enabled = payload[0] ? true : false;
            ESP_LOGI(TAG, "BLE CAN passthrough %s", ble_can_passthrough_enabled ? "enabled" : "disabled");

            if (ble_can_passthrough_enabled) {
                pwr_last_touch_ms = now_ms();
                pwr_was_idle = false;
                pwr_force_wake_request = true;
            }
        }
        break;

    case BLE_CMD_SEND_CAN_FRAME:
        if (len >= 13) {
            uint32_t can_id = ((uint32_t)payload[0]) |
                              ((uint32_t)payload[1] << 8) |
                              ((uint32_t)payload[2] << 16) |
                              ((uint32_t)payload[3] << 24);
            uint8_t dlc = payload[4] > 8 ? 8 : payload[4];
            if (should_log_boot_can_frame(can_id)) {
                log_boot_diag_frame("BLE->CAN", can_id, &payload[5], dlc);
            }
            can_send_raw_ext(can_id, &payload[5], dlc);
        }
        break;

    case BLE_CMD_SEND_CAN_FRAMES:
        if (len >= 1) {
            uint8_t count = payload[0];
            size_t offset = 1;
            uint8_t sent = 0;
            for (uint8_t i = 0; i < count && offset + 13 <= len; i++) {
                uint32_t can_id = ((uint32_t)payload[offset]) |
                                  ((uint32_t)payload[offset + 1] << 8) |
                                  ((uint32_t)payload[offset + 2] << 16) |
                                  ((uint32_t)payload[offset + 3] << 24);
                uint8_t dlc = payload[offset + 4] > 8 ? 8 : payload[offset + 4];
                if (should_log_boot_can_frame(can_id)) {
                    log_boot_diag_frame("BLE->CAN batch", can_id, &payload[offset + 5], dlc);
                }
                can_send_raw_ext(can_id, &payload[offset + 5], dlc);
                offset += 13;
                sent++;
            }
            if (sent != count) {
                ESP_LOGW(TAG, "[BLE-CAN] Short CAN batch sent=%u expected=%u len=%u",
                         sent, count, len);
            }
        }
        break;

    default:
        ESP_LOGW(TAG, "[BLE] Unknown cmd 0x%02X", cmd);
        break;
    }
}

// BLE status indicator on dashboard
static lv_obj_t *ble_status_icon = NULL;
static lv_obj_t *ble_pin_label   = NULL;

// ---------------------------------------------------------------------------
//  Public API — called from main.c
// ---------------------------------------------------------------------------
void can_hmi_init(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, COL_BG_DARK, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    create_dashboard(scr);
    create_settings_grid(scr);
    create_settings_detail(scr);
    create_error_page(scr);
    create_error_popup(scr);
    show_page(PAGE_DASHBOARD);

    // --- BLE Gateway ---
    uint32_t ble_pin = ble_gateway_init();
    ble_gateway_set_cmd_callback(ble_cmd_handler);

    // Create BLE status icon + PIN label on dashboard (top-right area)
    ble_status_icon = lv_label_create(scr);
    lv_label_set_text(ble_status_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(ble_status_icon, lv_color_hex(0x555555), 0);
    lv_obj_set_style_text_font(ble_status_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(ble_status_icon, LV_ALIGN_TOP_RIGHT, -10, 8);

    ble_pin_label = lv_label_create(scr);
    lv_label_set_text_fmt(ble_pin_label, "BLE PIN: %06lu", (unsigned long)ble_pin);
    lv_obj_set_style_text_color(ble_pin_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(ble_pin_label, &lv_font_montserrat_16, 0);
    lv_obj_align(ble_pin_label, LV_ALIGN_TOP_RIGHT, -40, 10);

    ESP_LOGI(TAG, "CAN HMI UI initialized (1024x600), BLE PIN=%06lu", (unsigned long)ble_pin);
}

void can_hmi_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "CAN HMI task started");

    uint32_t last_ui_update = 0;
    uint32_t last_busoff_check = 0;
    uint32_t last_heartbeat = 0;
    uint32_t last_ble_status_update = 0;

    while (1) {
        bool raw_passthrough_active = ble_can_passthrough_enabled && ble_gateway_is_connected();

        if (pwr_force_wake_request) {
            if (pwr_state == PWR_SLEEPING) {
                uint32_t now = now_ms();
                ESP_LOGI(TAG, "[PWR] Forcing wake for BLE CAN passthrough");
                pwr_display_on();
                pwr_remove_splash();
                pwr_show_splash();
                pwr_state_start_ms = now;
                pwr_state = PWR_WAKE_SPLASH;
                pwr_last_touch_ms = now;
            }
            pwr_force_wake_request = false;
        }

        // =============================================================
        // Sleep mode: brief TWAI poll + touch, CPU at 80MHz
        // BLE connected: faster cycle so command/response works
        // =============================================================
        if (pwr_state == PWR_SLEEPING) {
            bool ble_active = ble_gateway_is_connected();

            esp_pm_config_t pm_up = { .max_freq_mhz = 80, .min_freq_mhz = 80, .light_sleep_enable = false };
            esp_pm_configure(&pm_up);

            twai_start();
            vTaskDelay(pdMS_TO_TICKS(ble_active ? 50 : 10));

            twai_message_t rx_msg;
            while (twai_receive(&rx_msg, 0) == ESP_OK) {
                if (!rx_msg.rtr)
                    decode_can_message(rx_msg.identifier, rx_msg.data, rx_msg.data_length_code);
            }

            // Process staggered settings CAN requests.
            if (ble_active && !raw_passthrough_active) {
                uint32_t now_q = now_ms();
                if (req_queue_head < req_queue_count &&
                    (now_q - req_queue_last_ms >= REQ_SPACING_MS)) {
                    can_req_t *r = &req_queue[req_queue_head];
                    can_send_command(r->cmd, r->block, r->id, r->value);
                    pending_track_sent(r->cmd, r->block, r->id, r->value);
                    req_queue_head++;
                    req_queue_last_ms = now_q;
                    if (req_queue_head >= req_queue_count) {
                        reset_request_queue();
                    }
                }
                process_pending_timeouts();
            }

            twai_stop();

            // Touch + state machine
            if (lvgl_port_lock(10)) {
                pwr_management_tick();
                lvgl_port_unlock();
            }

            // Heartbeat
            uint32_t now_hb = now_ms();
            if (now_hb - last_heartbeat >= 10000) {
                ESP_LOGI(TAG, "[HB] alive t=%lu pwr=%d ble=%d", (unsigned long)now_hb, pwr_state, ble_active);
                last_heartbeat = now_hb;
            }

            if (pwr_state == PWR_SLEEPING) {
                esp_pm_config_t pm_dn = { .max_freq_mhz = 80, .min_freq_mhz = 80, .light_sleep_enable = false };
                esp_pm_configure(&pm_dn);
                vTaskDelay(pdMS_TO_TICKS(ble_active ? 50 : 500));
            }
            continue;
        }

        // =============================================================
        // Normal active mode
        // =============================================================
        // Poll TWAI — drain all available messages
        twai_message_t rx_msg;
        while (twai_receive(&rx_msg, 0) == ESP_OK) {
            if (!rx_msg.rtr) {
                decode_can_message(rx_msg.identifier, rx_msg.data, rx_msg.data_length_code);
            }
        }

        // Heartbeat every 10s to prove task is alive
        uint32_t now_bo = now_ms();
        if (now_bo - last_heartbeat >= 10000) {
            ESP_LOGI(TAG, "[HB] alive t=%lu pwr=%d", (unsigned long)now_bo, pwr_state);
            last_heartbeat = now_bo;
        }

        // Periodic checks every 1s
        if (now_bo - last_busoff_check >= 1000) {
            // Check TWAI alerts (bus-off recovery is automatic in ESP-IDF)
            uint32_t alerts;
            if (twai_read_alerts(&alerts, 0) == ESP_OK) {
                if (alerts & TWAI_ALERT_BUS_OFF) {
                    ESP_LOGW(TAG, "TWAI bus-off detected, initiating recovery");
                    twai_initiate_recovery();
                }
                if (alerts & TWAI_ALERT_BUS_RECOVERED) {
                    ESP_LOGI(TAG, "TWAI bus recovered");
                    twai_start();
                }
            }

            if (!raw_passthrough_active) {
                unit_offline_tick();

                // Retry identification for unidentified units
                for (int i = 0; i < UNIT_TABLE_SIZE; i++) {
                    if (unit_table[i].status == UNIT_ONLINE &&
                        !unit_table[i].id_complete &&
                        now_bo - unit_table[i].id_request_time >= 5000)
                    {
                        can_request_id(unit_table[i].can_addr);
                        unit_table[i].id_request_time = now_bo;
                    }
                }
            }
            last_busoff_check = now_bo;
        }

        // Process staggered request queue
        if (!raw_passthrough_active) {
            uint32_t now_q = now_ms();
            if (req_queue_head < req_queue_count &&
                (now_q - req_queue_last_ms >= REQ_SPACING_MS)) {
                can_req_t *r = &req_queue[req_queue_head];
                can_send_command(r->cmd, r->block, r->id, r->value);
                pending_track_sent(r->cmd, r->block, r->id, r->value);
                req_queue_head++;
                req_queue_last_ms = now_q;
            }
            process_pending_timeouts();
        }

        // UI update at ~20fps — must hold LVGL mutex
        uint32_t now = now_ms();
        if (now - last_ui_update >= 50) {
            if (lvgl_port_lock(10)) {
                // Power management — skip normal UI when display off or splash showing
                if (!pwr_management_tick()) {
                    if (current_page == PAGE_DASHBOARD)
                        update_dashboard();
                    else if (current_page == PAGE_SETTINGS_DETAIL)
                        update_settings_detail();
                    else if (current_page == PAGE_ERRORS)
                        update_error_page();

                    // Error popup logic
                    if (error_popup_visible) {
                        if (check_popup_auto_cleared()) {
                            error_popup_visible = false;
                            lv_obj_add_flag(error_popup, LV_OBJ_FLAG_HIDDEN);
                        }
                    } else {
                        if (check_for_error_popup()) {
                            uint8_t code = get_popup_error_code();
                            if (code != 0) {
                                show_error_popup(code);
                                buzzer_alarm();
                            }
                        }
                    }
                }

                lvgl_port_unlock();
            }
            last_ui_update = now;
        }

        // ---- BLE status update ~1 Hz ----
        uint32_t now_ble = now_ms();
        if (now_ble - last_ble_status_update >= 1000) {
            if (lvgl_port_lock(5)) {
                if (ble_gateway_is_connected()) {
                    lv_obj_set_style_text_color(ble_status_icon,
                        lv_color_hex(0x2196F3), 0); // blue = connected
                } else {
                    lv_obj_set_style_text_color(ble_status_icon,
                        lv_color_hex(0x555555), 0); // grey = disconnected
                }
                lvgl_port_unlock();
            }
            last_ble_status_update = now_ble;
        }

        // Slow down loop in sleep mode — CAN + touch only
        vTaskDelay(pdMS_TO_TICKS(raw_passthrough_active ? 1 : 5));
    }
}
