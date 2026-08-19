/**
 * @file ui_doc.c
 * @brief Static replicas of every firmware screen, for documentation shots.
 *
 * The simulator normally only renders the shared dashboard (dashboard_ui.c).
 * The remaining pages live in can_hmi.c and are inseparable from CAN/BLE, so
 * for GUI documentation this file re-creates their exact visuals — same
 * dimensions, colors and fonts, populated with representative static data.
 *
 * Usage:  clayton_sim [800|1024] <screen>
 *   dash         LPS dashboard, charging, all systems on, multi-unit selector
 *   dash-bms     Battery (BMS) dashboard — cards hidden, full-width DC OUT
 *   dash-fault   Dashboard with an inverter fault + error badge
 *   splash       Standby splash ("CLAYTON POWER")
 *   settings     Settings category grid (LPS: 9 tiles)
 *   detail       Category detail page (AC Output: info rows + settings)
 *   errors       Active-error list page
 *   popup-error  Error popup over the dashboard
 *   popup-pin    BLE pairing PIN popup over the dashboard
 *   editor       Numeric setting editor over the detail page
 *
 * Keep in sync with can_hmi.c when the real pages change. This file is
 * sim-only and never compiled into firmware.
 */

#include "lvgl.h"
#include "dashboard_ui.h"
#include "ui_palette.h"
#include <string.h>

/*==========================================================================
 *  Shared scaffolding (mirrors can_hmi.c styling)
 *========================================================================*/
static lv_coord_t W, H;

static lv_obj_t *page_fullscreen(void)
{
    lv_obj_t *page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page, W, H);
    lv_obj_set_style_bg_color(page, COL_BG_DARK, 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 0, 0);
    lv_obj_set_style_radius(page, 0, 0);
    lv_obj_align(page, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    return page;
}

static void page_header(lv_obj_t *page, const char *title, lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(page);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 16, 12);

    lv_obj_t *btn = lv_btn_create(page);
    lv_obj_set_size(btn, 100, 40);
    lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -12, 8);
    lv_obj_set_style_bg_color(btn, COL_BG_CARD, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_t *lb = lv_label_create(btn);
    lv_label_set_text(lb, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_14, 0);
    lv_obj_center(lb);
}

/*==========================================================================
 *  Dashboard variants (real shared module + static model)
 *========================================================================*/
static dashboard_model_t base_model(void)
{
    dashboard_model_t m = {0};
    m.soc_percent       = 76.0f;
    m.battery_current_a = -8.4f;         /* charging */
    m.soc_time_min      = 134;
    m.battery_voltage_v = 52.4f;
    m.inverter_state = 5;  m.dc_output_state = 5;
    m.charger_state  = 5;  m.dc_input_state  = 0;  m.solar_state = 5;
    m.dc_output_voltage_v = 12.46f; m.dc_output_current_a = 25.0f;
    m.solar_current_a     = 6.2f;
    m.ac_output_power_w = 683; m.ac_output_voltage_v = 230.1f; m.ac_output_current_a = 2.97f;
    m.ac_input_power_w  = 486; m.ac_input_voltage_v  = 230.0f; m.ac_input_current_a  = 2.11f;
    m.dev_sel_visible = true; m.dev_online = true;
    m.dev_name = "1/3 CL2 (1F-AB12)";
    return m;
}

static void build_dashboard(const dashboard_model_t *m)
{
    dashboard_ui_create(lv_scr_act(), NULL);
    dashboard_ui_set_ble(true);
    dashboard_ui_update(m);
}

static void screen_dash(void)
{
    dashboard_model_t m = base_model();
    build_dashboard(&m);
}

static void screen_dash_bms(void)
{
    dashboard_model_t m = {0};
    m.is_bms            = true;
    m.soc_percent       = 63.0f;
    m.battery_current_a = 12.5f;         /* discharging */
    m.soc_time_min      = 210;
    m.battery_voltage_v = 51.8f;
    m.dc_output_state   = 5;
    m.dev_sel_visible = true; m.dev_online = true;
    m.dev_name = "2/3 CB1 (7C-33F1)";
    build_dashboard(&m);
}

static void screen_dash_fault(void)
{
    dashboard_model_t m = base_model();
    m.inverter_state = 5; m.inverter_failure = 3;   /* critical fault */
    m.charger_state  = 2; m.charger_failure  = 2;   /* warning-level  */
    m.error_count    = 2; m.error_critical   = true;
    build_dashboard(&m);
}

/*==========================================================================
 *  Standby splash
 *========================================================================*/
static void screen_splash(void)
{
    lv_obj_t *page = page_fullscreen();

    lv_obj_t *title = lv_label_create(page);
    lv_label_set_text(title, "CLAYTON POWER");
    lv_obj_set_style_text_color(title, COL_ACCENT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *sub = lv_label_create(page);
    lv_label_set_text(sub, "Standby");
    lv_obj_set_style_text_color(sub, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 24);
}

/*==========================================================================
 *  Settings category grid
 *========================================================================*/
typedef struct { const char *icon, *title; } doc_cat_t;
static const doc_cat_t LPS_CATS[] = {
    {LV_SYMBOL_POWER,     "AC Output"},
    {LV_SYMBOL_CHARGE,    "AC Input"},
    {LV_SYMBOL_DOWNLOAD,  "DC Output"},
    {LV_SYMBOL_UPLOAD,    "DC Input"},
    {LV_SYMBOL_BATTERY_3, "Starter Battery"},
    {LV_SYMBOL_IMAGE,     "Solar"},
    {LV_SYMBOL_SETTINGS,  "General"},
    {LV_SYMBOL_EYE_OPEN,  "Status"},
    {LV_SYMBOL_WARNING,   "Temperature"},
};

static void screen_settings(void)
{
    lv_obj_t *page = page_fullscreen();
    page_header(page, LV_SYMBOL_SETTINGS "  SETTINGS", COL_ACCENT);

    const int tile_w = 200, tile_h = 140, gap = 20;
    const int cols = (W >= 900) ? 4 : 3;
    const int x0 = (W - cols * tile_w - (cols - 1) * gap) / 2;
    const int y0 = 56;

    for (int i = 0; i < (int)(sizeof(LPS_CATS) / sizeof(LPS_CATS[0])); i++) {
        int col = i % cols, row = i / cols;
        lv_obj_t *tile = lv_btn_create(page);
        lv_obj_set_pos(tile, x0 + col * (tile_w + gap), y0 + row * (tile_h + gap));
        lv_obj_set_size(tile, tile_w, tile_h);
        lv_obj_set_style_bg_color(tile, COL_BG_CARD, 0);
        lv_obj_set_style_radius(tile, 12, 0);
        lv_obj_set_style_border_width(tile, 1, 0);
        lv_obj_set_style_border_color(tile, COL_BG_PANEL, 0);
        lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(tile, 10, 0);

        lv_obj_t *icon = lv_label_create(tile);
        lv_label_set_text(icon, LPS_CATS[i].icon);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(icon, COL_ACCENT, 0);

        lv_obj_t *name = lv_label_create(tile);
        lv_label_set_text(name, LPS_CATS[i].title);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(name, COL_TEXT, 0);
        lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);
    }
}

/*==========================================================================
 *  Category detail page (AC Output)
 *========================================================================*/
static lv_obj_t *detail_page(void)
{
    lv_obj_t *page = page_fullscreen();
    page_header(page, LV_SYMBOL_POWER "  AC Output", COL_ACCENT);

    lv_obj_t *content = lv_obj_create(page);
    lv_obj_set_size(content, W - 20, H - 60);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 4, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 4, 0);

    const int row_w = W - 64;

    /* Read-only info rows */
    static const char *info[][2] = {
        {"Status", "On"}, {"Power", "683 W"},
        {"Voltage", "230 V"}, {"Current", "2.97 A"},
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *row = lv_obj_create(content);
        lv_obj_set_size(row, row_w, 40);
        lv_obj_set_style_bg_color(row, COL_BG_PANEL, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(row, 6, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_hor(row, 18, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *name = lv_label_create(row);
        lv_label_set_text(name, info[i][0]);
        lv_obj_set_style_text_color(name, COL_TEXT_DIM, 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *val = lv_label_create(row);
        lv_label_set_text(val, info[i][1]);
        lv_obj_set_style_text_color(val, COL_TEXT, 0);
        lv_obj_set_style_text_font(val, &lv_font_montserrat_16, 0);
        lv_obj_align(val, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    /* Separator */
    lv_obj_t *sep = lv_obj_create(content);
    lv_obj_set_size(sep, row_w - 20, 1);
    lv_obj_set_style_bg_color(sep, COL_BG_CARD, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_pad_all(sep, 0, 0);

    /* Editable setting rows */
    static const char *settings[][2] = {
        {"Inverter Cutoff", "10 %"},
        {"Auto Off Delay",  "00:05:00"},
        {"Auto Off Load",   "200 W"},
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *row = lv_btn_create(content);
        lv_obj_set_size(row, row_w, 50);
        lv_obj_set_style_bg_color(row, COL_BG_CARD, 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_pad_hor(row, 18, 0);

        lv_obj_t *name = lv_label_create(row);
        lv_label_set_text(name, settings[i][0]);
        lv_obj_set_style_text_color(name, COL_TEXT, 0);
        lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *val = lv_label_create(row);
        lv_label_set_text(val, settings[i][1]);
        lv_obj_set_style_text_color(val, COL_ACCENT, 0);
        lv_obj_set_style_text_font(val, &lv_font_montserrat_16, 0);
        lv_obj_align(val, LV_ALIGN_RIGHT_MID, 0, 0);
    }
    return page;
}

static void screen_detail(void) { detail_page(); }

/*==========================================================================
 *  Error list page
 *========================================================================*/
typedef struct { const char *title, *desc; bool warning; } doc_err_t;
static const doc_err_t DOC_ERRORS[] = {
    {"E004 High Temp Warning  [WARNING]",
     "Unit is getting too hot. Allow it to cool down.", true},
    {"E011 IO Overload  [FAILURE]",
     "12 VDC aux overloaded. Remove load to avoid shutdown.", false},
    {"E020 230 VAC Overload  [FAILURE]",
     "230 VAC overloaded. Remove load to avoid shutdown.", false},
};

static void screen_errors(void)
{
    lv_obj_t *page = page_fullscreen();
    page_header(page, LV_SYMBOL_WARNING "  ERRORS", COL_RED);

    lv_obj_t *content = lv_obj_create(page);
    lv_obj_set_size(content, W - 20, H - 60);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 4, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, 6, 0);

    const int row_w = W - 64;
    for (int i = 0; i < 3; i++) {
        lv_color_t col = DOC_ERRORS[i].warning ? COL_ORANGE : COL_RED;

        lv_obj_t *row = lv_obj_create(content);
        lv_obj_set_size(row, row_w, 80);
        lv_obj_set_style_bg_color(row, COL_BG_CARD, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, col, 0);
        lv_obj_set_style_pad_all(row, 12, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *dot = lv_obj_create(row);
        lv_obj_set_size(dot, 12, 12);
        lv_obj_set_style_radius(dot, 6, 0);
        lv_obj_set_style_bg_color(dot, col, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_align(dot, LV_ALIGN_TOP_LEFT, 0, 4);

        lv_obj_t *title = lv_label_create(row);
        lv_label_set_text(title, DOC_ERRORS[i].title);
        lv_obj_set_style_text_color(title, COL_TEXT, 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 22, 0);

        lv_obj_t *desc = lv_label_create(row);
        lv_label_set_text(desc, DOC_ERRORS[i].desc);
        lv_obj_set_style_text_color(desc, COL_TEXT_DIM, 0);
        lv_obj_set_style_text_font(desc, &lv_font_montserrat_12, 0);
        lv_obj_set_width(desc, row_w - 50);
        lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);
        lv_obj_align(desc, LV_ALIGN_TOP_LEFT, 22, 24);
    }
}

/*==========================================================================
 *  Popups
 *========================================================================*/
static void screen_popup_error(void)
{
    screen_dash_fault();

    lv_obj_t *pop = lv_obj_create(lv_scr_act());
    lv_obj_set_size(pop, 560, 340);
    lv_obj_center(pop);
    lv_obj_set_style_bg_color(pop, lv_color_hex(0x1a1018), 0);
    lv_obj_set_style_bg_opa(pop, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pop, 2, 0);
    lv_obj_set_style_border_color(pop, COL_RED, 0);
    lv_obj_set_style_radius(pop, 14, 0);
    lv_obj_set_style_pad_all(pop, 20, 0);
    lv_obj_clear_flag(pop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *level = lv_label_create(pop);
    lv_label_set_text(level, LV_SYMBOL_WARNING "  FAILURE");
    lv_obj_set_style_text_color(level, COL_RED, 0);
    lv_obj_set_style_text_font(level, &lv_font_montserrat_20, 0);
    lv_obj_align(level, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *title = lv_label_create(pop);
    lv_label_set_text(title, "E020 230 VAC Overload");
    lv_obj_set_style_text_color(title, COL_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(title, 510);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *desc = lv_label_create(pop);
    lv_label_set_text(desc, "230 VAC overloaded. Remove load to avoid shutdown.");
    lv_obj_set_style_text_color(desc, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(desc, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(desc, 510);
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);
    lv_obj_align(desc, LV_ALIGN_TOP_MID, 0, 90);

    lv_obj_t *code = lv_label_create(pop);
    lv_label_set_text(code, "Error code: 20");
    lv_obj_set_style_text_color(code, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(code, &lv_font_montserrat_14, 0);
    lv_obj_align(code, LV_ALIGN_TOP_MID, 0, 160);

    lv_obj_t *btn_clr = lv_btn_create(pop);
    lv_obj_set_size(btn_clr, 200, 56);
    lv_obj_align(btn_clr, LV_ALIGN_BOTTOM_LEFT, 10, -6);
    lv_obj_set_style_bg_color(btn_clr, COL_RED, 0);
    lv_obj_set_style_radius(btn_clr, 10, 0);
    lv_obj_t *lb1 = lv_label_create(btn_clr);
    lv_label_set_text(lb1, LV_SYMBOL_TRASH "  Clear");
    lv_obj_set_style_text_font(lb1, &lv_font_montserrat_20, 0);
    lv_obj_center(lb1);

    lv_obj_t *btn_ret = lv_btn_create(pop);
    lv_obj_set_size(btn_ret, 200, 56);
    lv_obj_align(btn_ret, LV_ALIGN_BOTTOM_RIGHT, -10, -6);
    lv_obj_set_style_bg_color(btn_ret, COL_BG_CARD, 0);
    lv_obj_set_style_radius(btn_ret, 10, 0);
    lv_obj_t *lb2 = lv_label_create(btn_ret);
    lv_label_set_text(lb2, LV_SYMBOL_LEFT "  Return");
    lv_obj_set_style_text_font(lb2, &lv_font_montserrat_20, 0);
    lv_obj_center(lb2);
}

static void screen_popup_pin(void)
{
    screen_dash();

    lv_obj_t *pop = lv_obj_create(lv_scr_act());
    lv_obj_set_size(pop, 470, 300);
    lv_obj_center(pop);
    lv_obj_set_style_bg_color(pop, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(pop, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pop, 2, 0);
    lv_obj_set_style_border_color(pop, COL_ACCENT, 0);
    lv_obj_set_style_radius(pop, 18, 0);
    lv_obj_set_style_pad_all(pop, 22, 0);
    lv_obj_set_style_shadow_width(pop, 28, 0);
    lv_obj_set_style_shadow_color(pop, lv_color_hex(0x020609), 0);
    lv_obj_clear_flag(pop, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(pop);
    lv_label_set_text(title, LV_SYMBOL_BLUETOOTH "  BLE PIN");
    lv_obj_set_style_text_color(title, COL_ACCENT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *pin = lv_label_create(pop);
    lv_label_set_text(pin, "482913");
    lv_obj_set_style_text_color(pin, COL_TEXT, 0);
    lv_obj_set_style_text_font(pin, &lv_font_montserrat_48, 0);
    lv_obj_align(pin, LV_ALIGN_TOP_MID, 0, 68);

    lv_obj_t *hint = lv_label_create(pop);
    lv_label_set_text(hint, "Enter this PIN on the phone when pairing.");
    lv_obj_set_width(hint, 390);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(hint, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_16, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 132);

    lv_obj_t *btn = lv_btn_create(pop);
    lv_obj_set_size(btn, 180, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(btn, COL_BG_CARD, 0);
    lv_obj_set_style_radius(btn, 14, 0);
    lv_obj_t *lb = lv_label_create(btn);
    lv_label_set_text(lb, LV_SYMBOL_CLOSE "  Close");
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_16, 0);
    lv_obj_center(lb);
}

static void screen_editor(void)
{
    lv_obj_t *page = detail_page();

    lv_obj_t *ed = lv_obj_create(page);
    lv_obj_set_size(ed, 500, 300);
    lv_obj_center(ed);
    lv_obj_set_style_bg_color(ed, lv_color_hex(0x161b22), 0);
    lv_obj_set_style_bg_opa(ed, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ed, 2, 0);
    lv_obj_set_style_border_color(ed, COL_ACCENT, 0);
    lv_obj_set_style_radius(ed, 12, 0);
    lv_obj_set_style_pad_all(ed, 16, 0);
    lv_obj_clear_flag(ed, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *name = lv_label_create(ed);
    lv_label_set_text(name, "Auto Off Load");
    lv_obj_set_style_text_color(name, COL_ACCENT, 0);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_20, 0);
    lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *val = lv_label_create(ed);
    lv_label_set_text(val, "200 W");
    lv_obj_set_style_text_color(val, COL_TEXT, 0);
    lv_obj_set_style_text_font(val, &lv_font_montserrat_28, 0);
    lv_obj_align(val, LV_ALIGN_TOP_MID, 0, 45);

    lv_obj_t *range = lv_label_create(ed);
    lv_label_set_text(range, "Min: 10 W    Max: 1500 W");
    lv_obj_set_style_text_color(range, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(range, &lv_font_montserrat_14, 0);
    lv_obj_align(range, LV_ALIGN_TOP_MID, 0, 95);

    const char *syms[] = {LV_SYMBOL_MINUS, LV_SYMBOL_PLUS};
    int x_pos[] = {-110, 110};
    for (int i = 0; i < 2; i++) {
        lv_obj_t *b = lv_btn_create(ed);
        lv_obj_set_size(b, 100, 56);
        lv_obj_align(b, LV_ALIGN_TOP_MID, x_pos[i], 130);
        lv_obj_set_style_bg_color(b, COL_BG_CARD, 0);
        lv_obj_set_style_radius(b, 8, 0);
        lv_obj_t *lb = lv_label_create(b);
        lv_label_set_text(lb, syms[i]);
        lv_obj_set_style_text_font(lb, &lv_font_montserrat_24, 0);
        lv_obj_center(lb);
    }

    const char *ok_syms[] = {LV_SYMBOL_OK, LV_SYMBOL_CLOSE};
    lv_color_t ok_cols[] = {COL_GREEN, COL_RED};
    int ok_x[] = {-80, 80};
    for (int i = 0; i < 2; i++) {
        lv_obj_t *b = lv_btn_create(ed);
        lv_obj_set_size(b, 130, 50);
        lv_obj_align(b, LV_ALIGN_TOP_MID, ok_x[i], 215);
        lv_obj_set_style_bg_color(b, ok_cols[i], 0);
        lv_obj_set_style_radius(b, 8, 0);
        lv_obj_t *lb = lv_label_create(b);
        lv_label_set_text(lb, ok_syms[i]);
        lv_obj_set_style_text_font(lb, &lv_font_montserrat_20, 0);
        lv_obj_center(lb);
    }
}

/*==========================================================================
 *  Dispatch
 *========================================================================*/
bool ui_doc_create(const char *screen)
{
    W = lv_disp_get_hor_res(NULL);
    H = lv_disp_get_ver_res(NULL);

    lv_obj_set_style_bg_color(lv_scr_act(), COL_BG_DARK, 0);
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);

    if      (!strcmp(screen, "dash"))        screen_dash();
    else if (!strcmp(screen, "dash-bms"))    screen_dash_bms();
    else if (!strcmp(screen, "dash-fault"))  screen_dash_fault();
    else if (!strcmp(screen, "splash"))      screen_splash();
    else if (!strcmp(screen, "settings"))    screen_settings();
    else if (!strcmp(screen, "detail"))      screen_detail();
    else if (!strcmp(screen, "errors"))      screen_errors();
    else if (!strcmp(screen, "popup-error")) screen_popup_error();
    else if (!strcmp(screen, "popup-pin"))   screen_popup_pin();
    else if (!strcmp(screen, "editor"))      screen_editor();
    else return false;
    return true;
}
