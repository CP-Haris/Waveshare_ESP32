/**
 * @file ui_demo.c
 * @brief Simulator harness that renders the SHARED dashboard_ui module.
 *
 * The simulator now builds the exact same dashboard as the firmware (via
 * dashboard_ui_create / dashboard_ui_update), so what you see here matches the
 * ESP32-S3 screen 1:1. A timer animates a demo model so the UI feels live.
 */

#include "lvgl.h"
#include "dashboard_ui.h"
#include <math.h>

/* Demo button callbacks — just log via no-op (simulator has no CAN bus). */
static void on_inverter(void) { /* toggle inverter (no-op in sim) */ }
static void on_dcout(void)    { /* toggle dc-out (no-op in sim)   */ }
static void on_settings(void) { /* open settings (no-op in sim)   */ }
static void on_error(void)    { /* open errors (no-op in sim)     */ }
static void on_prev(void)     { /* prev unit (no-op in sim)       */ }
static void on_next(void)     { /* next unit (no-op in sim)       */ }

static uint32_t s_tick = 0;

/* Build a believable, slowly-changing demo model and push it to the UI. */
static void demo_tick(lv_timer_t *t)
{
    (void)t;
    s_tick++;

    float phase = (float)s_tick * 0.05f;
    float soc   = 70.0f + 17.0f * sinf(phase * 0.3f);     /* 53..87 % */
    float curr  = 12.5f * sinf(phase * 0.5f);             /* +/- charge */

    dashboard_model_t m = {0};
    m.is_bms            = false;
    m.soc_percent       = soc;
    m.battery_current_a = curr;
    m.soc_time_min      = 134;
    m.battery_voltage_v = 52.4f;

    /* Sources: inverter ON, charger ready, solar charging, dc-out on */
    m.inverter_state = 5;  m.inverter_failure = 0;
    m.dc_output_state = 5; m.dc_output_failure = 0;
    m.charger_state = 2;   m.charger_failure = 0;   /* AC chip + charger card */
    m.dc_input_state = 0;  m.dc_input_failure = 0;
    m.solar_state = 5;     m.solar_failure = 0;

    m.dc_output_voltage_v = 12.46f; m.dc_output_current_a = 25.0f;
    m.solar_current_a     = 6.2f;
    m.ac_output_power_w   = 683; m.ac_output_voltage_v = 230.1f; m.ac_output_current_a = 2.97f;
    m.ac_input_power_w    = 0;   m.ac_input_voltage_v  = 0.0f;   m.ac_input_current_a  = 0.0f;

    m.error_count    = 0;
    m.error_critical = false;

    /* Show the multi-unit selector to exercise that path */
    m.dev_sel_visible = true;
    m.dev_online      = true;
    m.dev_name        = "1/3 CL2 (1F-AB12)";

    dashboard_ui_update(&m);
}

void ui_demo_create(void)
{
    static const dashboard_callbacks_t cb = {
        .on_inverter    = on_inverter,
        .on_dcout       = on_dcout,
        .on_settings    = on_settings,
        .on_error_badge = on_error,
        .on_dev_prev    = on_prev,
        .on_dev_next    = on_next,
    };

    dashboard_ui_create(lv_scr_act(), &cb);
    dashboard_ui_set_ble(true);

    demo_tick(NULL);                       /* initial paint */
    lv_timer_create(demo_tick, 100, NULL); /* animate ~10 Hz */
}
