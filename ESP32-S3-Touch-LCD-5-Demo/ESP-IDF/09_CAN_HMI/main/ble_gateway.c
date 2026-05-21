/**
 * BLE Gateway — NimBLE GATT server for CAN-over-BLE bridge.
 *
 * Service UUID:  0x1000 (custom short — expanded to 128-bit by NimBLE)
 * TX Char UUID:  0x1001 (Notify)
 * RX Char UUID:  0x1002 (Write)
 */

#include "ble_gateway.h"

#include <string.h>
#include "esp_log.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_store.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "ble_gw";

// ---------------------------------------------------------------------------
//  UUIDs (128-bit, base: 0000xxxx-0000-1000-8000-00805f9b34fb)
// ---------------------------------------------------------------------------
static const ble_uuid128_t svc_uuid =
    BLE_UUID128_INIT(0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00,
                     0x00, 0x80, 0x00, 0x10, 0x00, 0x00,
                     0x00, 0x10, 0x00, 0x00);

static const ble_uuid128_t tx_chr_uuid =
    BLE_UUID128_INIT(0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00,
                     0x00, 0x80, 0x00, 0x10, 0x00, 0x00,
                     0x01, 0x10, 0x00, 0x00);

static const ble_uuid128_t rx_chr_uuid =
    BLE_UUID128_INIT(0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00,
                     0x00, 0x80, 0x00, 0x10, 0x00, 0x00,
                     0x02, 0x10, 0x00, 0x00);

// ---------------------------------------------------------------------------
//  State
// ---------------------------------------------------------------------------
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t tx_attr_handle;
static bool     client_subscribed = false;
static uint32_t passkey = 0;
static ble_cmd_callback_t cmd_callback = NULL;
static ble_pairing_callback_t pairing_callback = NULL;
static bool ble_ready = false;

void ble_store_config_init(void);

#define BLE_RX_MAX_LEN 256

// ---------------------------------------------------------------------------
//  BLE notification helper
// ---------------------------------------------------------------------------
static void notify(const uint8_t *data, uint16_t len)
{
    if (conn_handle == BLE_HS_CONN_HANDLE_NONE || !client_subscribed) return;

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (om) {
        int rc = ble_gatts_notify_custom(conn_handle, tx_attr_handle, om);
        if (rc != 0) {
            ESP_LOGW(TAG, "notify failed rc=%d", rc);
        }
    }
}

// ---------------------------------------------------------------------------
//  GATT Access Callbacks
// ---------------------------------------------------------------------------
static int gatt_tx_access(uint16_t conn, uint16_t attr,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    // TX is notify-only — read returns empty
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_rx_access(uint16_t conn, uint16_t attr,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_UNLIKELY;

    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    if (len < 1) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;

    uint8_t buf[BLE_RX_MAX_LEN];
    if (len > sizeof(buf)) {
        ESP_LOGW(TAG, "RX cmd too large len=%u max=%u", (unsigned)len, (unsigned)sizeof(buf));
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    uint16_t copy_len = len;
    os_mbuf_copydata(ctxt->om, 0, copy_len, buf);

    uint8_t cmd = buf[0];
    if (cmd != BLE_CMD_SEND_CAN_FRAME && cmd != BLE_CMD_SEND_CAN_FRAMES) {
        ESP_LOGI(TAG, "RX cmd=0x%02X len=%d", cmd, len);
    }

    if (cmd_callback) {
        cmd_callback(cmd, buf + 1, copy_len - 1);
    }

    return 0;
}

// ---------------------------------------------------------------------------
//  GATT Service Definition
// ---------------------------------------------------------------------------
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                // TX — Notify (ESP32 → Phone)
                .uuid = &tx_chr_uuid.u,
                .access_cb = gatt_tx_access,
                .val_handle = &tx_attr_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_AUTHEN |
                         BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_NOTIFY_INDICATE_AUTHEN,
            },
            {
                // RX — Write (Phone → ESP32)
                .uuid = &rx_chr_uuid.u,
                .access_cb = gatt_rx_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP |
                         BLE_GATT_CHR_F_WRITE_AUTHEN,
            },
            { 0 }, // terminator
        },
    },
    { 0 }, // terminator
};

// ---------------------------------------------------------------------------
//  GAP Event Handler
// ---------------------------------------------------------------------------
static void start_advertise(void);

static int gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {

    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "BLE %s (handle=%d)",
                 event->connect.status == 0 ? "connected" : "connect failed",
                 event->connect.conn_handle);
        if (event->connect.status == 0) {
            conn_handle = event->connect.conn_handle;
            // Phone will initiate MTU exchange — we just set preferred MTU
            ble_att_set_preferred_mtu(256);
            int rc = ble_gap_security_initiate(conn_handle);
            if (rc != 0) {
                ESP_LOGW(TAG, "security initiate failed rc=%d", rc);
            }
        } else {
            start_advertise();
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "BLE disconnected reason=%d", event->disconnect.reason);
        conn_handle = BLE_HS_CONN_HANDLE_NONE;
        client_subscribed = false;
        start_advertise();
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        if (event->subscribe.attr_handle == tx_attr_handle) {
            client_subscribed = event->subscribe.cur_notify;
            ESP_LOGI(TAG, "Notifications %s",
                     client_subscribed ? "enabled" : "disabled");
        }
        break;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU updated: %d", event->mtu.value);
        break;

    case BLE_GAP_EVENT_PASSKEY_ACTION:
        if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
            struct ble_sm_io pk = {
                .action = BLE_SM_IOACT_DISP,
                .passkey = passkey,
            };
            if (pairing_callback) {
                pairing_callback(passkey);
            }
            ble_sm_inject_io(event->passkey.conn_handle, &pk);
            ESP_LOGI(TAG, "Passkey display: %06lu", (unsigned long)passkey);
        }
        break;

    case BLE_GAP_EVENT_ENC_CHANGE:
        ESP_LOGI(TAG, "Encryption %s",
                 event->enc_change.status == 0 ? "enabled" : "failed");
        break;

    case BLE_GAP_EVENT_REPEAT_PAIRING: {
        struct ble_gap_conn_desc desc;
        int rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        if (rc == 0) {
            ble_store_util_delete_peer(&desc.peer_id_addr);
        }
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    }

    default:
        break;
    }
    return 0;
}

// ---------------------------------------------------------------------------
//  Advertising
// ---------------------------------------------------------------------------
static void start_advertise(void)
{
    int rc;

    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
        .itvl_min = 160,   // 100ms
        .itvl_max = 240,   // 150ms
        .channel_map = 7,  // All 3 advertising channels (37, 38, 39)
    };

    // Advertising data: flags + name (keep it small & reliable)
    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)"Clayton Power";
    fields.name_len = strlen("Clayton Power");
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_set_fields failed rc=%d", rc);
        return;
    }

    // Scan response with service UUID
    struct ble_hs_adv_fields rsp = {0};
    static const ble_uuid128_t rsp_uuid = BLE_UUID128_INIT(
        0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00,
        0x00, 0x80, 0x00, 0x10, 0x00, 0x00,
        0x00, 0x10, 0x00, 0x00);
    rsp.uuids128 = &rsp_uuid;
    rsp.num_uuids128 = 1;
    rsp.uuids128_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_rsp_set_fields failed rc=%d", rc);
    }

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                            &adv_params, gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_start failed rc=%d", rc);
    } else {
        ESP_LOGI(TAG, "Advertising started (Clayton Power)");
    }
}

// ---------------------------------------------------------------------------
//  NimBLE host task + sync callback
// ---------------------------------------------------------------------------
static void on_sync(void)
{
    // Use public address
    uint8_t own_addr_type;
    int rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_hs_id_infer_auto failed: %d", rc);
        return;
    }

    // Configure security manager: display-only IO, require bonding
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_DISP_ONLY;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 1;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

    // Passkey is delivered via BLE_SM_IOACT_DISP in gap_event handler

    start_advertise();
    ble_ready = true;
}

static void on_reset(int reason)
{
    ESP_LOGW(TAG, "NimBLE reset reason=%d", reason);
    ble_ready = false;
}

static void nimble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// ---------------------------------------------------------------------------
//  Public API
// ---------------------------------------------------------------------------
uint32_t ble_gateway_init(void)
{
    // Generate random 6-digit passkey
    passkey = esp_random() % 1000000;
    ESP_LOGI(TAG, "BLE passkey: %06lu", (unsigned long)passkey);

    // Initialize NVS (required by NimBLE for bonding + PHY cal)
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %d", (int)err);
    }

    // Initialize NimBLE
    int rc = nimble_port_init();
    if (rc != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init failed: %d", rc);
        return passkey;
    }

    // Register GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svcs);
    assert(rc == 0);
    rc = ble_gatts_add_svcs(gatt_svcs);
    assert(rc == 0);

    // Set device name
    ble_svc_gap_device_name_set("Clayton Power");

    // Host callbacks
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_store_config_init();

    // Start NimBLE host task
    nimble_port_freertos_init(nimble_host_task);

    return passkey;
}

void ble_gateway_send_can_frame(uint32_t can_id, const uint8_t *data, uint8_t dlc)
{
    uint8_t pkt[1 + 4 + 1 + 8] = {0};
    if (dlc > 8) dlc = 8;

    pkt[0] = BLE_MSG_CAN_FRAME;
    pkt[1] = (uint8_t)(can_id & 0xFF);
    pkt[2] = (uint8_t)((can_id >> 8) & 0xFF);
    pkt[3] = (uint8_t)((can_id >> 16) & 0xFF);
    pkt[4] = (uint8_t)((can_id >> 24) & 0xFF);
    pkt[5] = dlc;

    if (data && dlc > 0) {
        memcpy(&pkt[6], data, dlc);
    }

    notify(pkt, sizeof(pkt));
}

bool ble_gateway_is_connected(void)
{
    return ble_ready && conn_handle != BLE_HS_CONN_HANDLE_NONE && client_subscribed;
}

uint32_t ble_gateway_get_passkey(void)
{
    return passkey;
}

void ble_gateway_set_cmd_callback(ble_cmd_callback_t cb)
{
    cmd_callback = cb;
}

void ble_gateway_set_pairing_callback(ble_pairing_callback_t cb)
{
    pairing_callback = cb;
}
