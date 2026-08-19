/**
 * @file dashboard_ui.h
 * @brief Shared, data-driven dashboard layout for the Clayton Power HMI.
 *
 * This is the SINGLE source of truth for the dashboard's visual layout. Both
 * the ESP32-S3 firmware (can_hmi.c) and the PC simulator build the dashboard
 * through this module, so the on-device screen and the simulator can never
 * visually diverge.
 *
 * The module is pure LVGL 8.4 — it has NO dependency on CAN, BLE, NVS or any
 * firmware service. The caller:
 *   1. calls dashboard_ui_create() once with a set of button callbacks,
 *   2. feeds live data each frame via dashboard_ui_update(&model),
 *   3. (optionally) reports BLE state via dashboard_ui_set_ble().
 *
 * The layout is resolution-aware: dashboard_ui_create() reads the display size
 * from LVGL and builds either the full 1024x600 design or a compact 800x480
 * variant. Callers do not need to know which panel is fitted.
 *
 * Target: 1024x600 / 800x480, LVGL 8.4.
 */

#ifndef DASHBOARD_UI_H
#define DASHBOARD_UI_H

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Plain data snapshot the dashboard renders. Filled by the caller each frame.
 *  Source state/failure values use the same semantics as the firmware:
 *    state:  <1 = off, 1..4 = starting/active, >=5 = on
 *    failure: 0 = ok, >=2 = fault (>=3 = critical/red, else orange)
 */
typedef struct {
    bool     is_bms;              /* BMS units hide inverter + source cards     */

    /* SOC hero */
    float    soc_percent;         /* 0..100                                     */
    float    battery_current_a;   /* + discharging, - charging                  */
    int      soc_time_min;        /* signed minutes remaining                   */
    float    battery_voltage_v;

    /* Toggle + source states (raw) */
    int8_t   inverter_state;      uint8_t inverter_failure;
    int8_t   dc_output_state;     uint8_t dc_output_failure;
    int8_t   charger_state;       uint8_t charger_failure;   /* AC charge chip + charger card */
    int8_t   dc_input_state;      uint8_t dc_input_failure;  /* DC charge chip               */
    int8_t   solar_state;         uint8_t solar_failure;     /* solar chip + solar card      */

    /* System card live values */
    float    dc_output_voltage_v, dc_output_current_a;
    float    solar_current_a;
    uint16_t ac_output_power_w;   float ac_output_voltage_v, ac_output_current_a;
    uint16_t ac_input_power_w;    float ac_input_voltage_v, ac_input_current_a;

    /* Error badge */
    int      error_count;
    bool     error_critical;      /* failure_level >= simple-failure threshold  */

    /* Device selector */
    bool        dev_sel_visible;  /* show only when >1 unit online              */
    bool        dev_online;       /* selected unit online (dot color)           */
    const char *dev_name;         /* pre-formatted "1/3 CL2 (AB-12)" string     */
} dashboard_model_t;

/** Button callbacks (invoked on click). Any field may be NULL. */
typedef struct {
    void (*on_inverter)(void);
    void (*on_dcout)(void);
    void (*on_settings)(void);
    void (*on_error_badge)(void);
    void (*on_dev_prev)(void);
    void (*on_dev_next)(void);
} dashboard_callbacks_t;

/** Build the dashboard as a child of @p parent. Call once. */
void dashboard_ui_create(lv_obj_t *parent, const dashboard_callbacks_t *cb);

/** Root container of the dashboard (for show/hide page switching). */
lv_obj_t *dashboard_ui_root(void);

/** Apply a fresh data snapshot to all widgets. Call each frame. */
void dashboard_ui_update(const dashboard_model_t *m);

/** Update only the BLE status icon (separate, low-rate path). */
void dashboard_ui_set_ble(bool connected);

/** USB link state shown by the header USB icon. */
typedef enum {
    DASH_USB_NONE = 0,   /* no host attached — icon hidden            */
    DASH_USB_IDLE,       /* cable attached, no port open — faint icon */
    DASH_USB_DEBUG,      /* debug/console COM port open — "DBG"       */
    DASH_USB_MODEM,      /* CAN modem COM port open — "CAN"           */
} dash_usb_status_t;

/** Update only the USB status icon (separate, low-rate path). */
void dashboard_ui_set_usb(dash_usb_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* DASHBOARD_UI_H */
