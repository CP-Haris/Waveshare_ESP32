/**
 * @file dashboard_ui.c
 * @brief Shared dashboard layout implementation (pure LVGL 8.4).
 *
 * Builds the Clayton Power dashboard and updates it from a plain
 * dashboard_model_t. Used identically by firmware and the PC simulator.
 *
 * The module reads the real display size from LVGL at create-time and picks
 * one of two layouts — the full 1024x600 design or a compact 800x480 variant
 * with the same visual language. Every coordinate comes from the layout table
 * below, so nothing here is hard-wired to one panel.
 */

#include "dashboard_ui.h"
#include "ui_palette.h"
#include "ui_screen.h"
#include <stdio.h>
#include <string.h>

#if !UI_SKIN_CARBON  /* classic skin — the Carbon Blue skin lives in dashboard_carbon.c */

/*==========================================================================
 *  Resolution-dependent layout
 *========================================================================*/
static lv_coord_t W, H;          /* actual screen size (from LVGL)          */
static bool g_compact;           /* true on the 800x480 panel               */

typedef struct {
    lv_coord_t margin;                   /* outer screen margin             */

    /* Header */
    lv_coord_t badge_w, badge_h;         /* error badge                     */
    lv_coord_t gear;                     /* settings button (square)        */
    lv_coord_t pill_w, pill_h;           /* device selector pill            */
    lv_coord_t pill_btn_w, pill_btn_h;   /* prev/next arrows inside pill    */
    lv_coord_t pill_dot_x;               /* online dot x inside pill        */

    /* SOC hero card */
    lv_coord_t hero_w, hero_h, hero_y;
    lv_coord_t arc, arc_stroke;          /* SOC ring diameter + stroke      */
    lv_coord_t soc_dy, time_dy;          /* label offsets vs. arc centre    */
    lv_coord_t batt_dy;                  /* V/A line above hero bottom      */
    lv_coord_t chip_h, chip_pad, chip_gap;

    /* System side cards */
    lv_coord_t card_w, card_h;
    lv_coord_t card_y0, card_y1;
    lv_coord_t card_pad, card_lbl_x;

    /* Bottom toggle buttons */
    lv_coord_t btn_w, btn_h;

    /* Fonts */
    const lv_font_t *f_soc;              /* big SOC percentage              */
    const lv_font_t *f_time;             /* "time left"                     */
    const lv_font_t *f_batt;             /* "V / A" battery info            */
    const lv_font_t *f_card_val;         /* system card main value          */
    const lv_font_t *f_card_sm;          /* card label/state/detail         */
    const lv_font_t *f_icon;             /* header + card icons             */
    const lv_font_t *f_btn;              /* bottom toggle buttons           */
} layout_t;
static layout_t L;

/* Full 1024x600 design. */
static const layout_t LAYOUT_FULL = {
    .margin      = 14,
    .badge_w     = 56,  .badge_h = 46,
    .gear        = 50,
    .pill_w      = 360, .pill_h  = 46,
    .pill_btn_w  = 40,  .pill_btn_h = 38,
    .pill_dot_x  = 46,
    .hero_w      = 372, .hero_h  = 392, .hero_y = 80,
    .arc         = 300, .arc_stroke = 18,
    .soc_dy      = -18, .time_dy = 34,
    .batt_dy     = -46,
    .chip_h      = 38,  .chip_pad = 14, .chip_gap = 8,
    .card_w      = 286, .card_h  = 132,
    .card_y0     = 80,  .card_y1 = 226,
    .card_pad    = 14,  .card_lbl_x = 28,
    .btn_w       = 430, .btn_h   = 60,
    .f_soc       = &lv_font_montserrat_48,
    .f_time      = &lv_font_montserrat_24,
    .f_batt      = &lv_font_montserrat_20,
    .f_card_val  = &lv_font_montserrat_28,
    .f_card_sm   = &lv_font_montserrat_14,
    .f_icon      = &lv_font_montserrat_16,
    .f_btn       = &lv_font_montserrat_20,
};

/* Compact 800x480 design — same language, tighter grid.
 *   header  10..52      hero 58..394      buttons 420..470
 *   columns 10..242  |  254..546  |  558..790
 */
static const layout_t LAYOUT_COMPACT = {
    .margin      = 10,
    .badge_w     = 48,  .badge_h = 40,
    .gear        = 44,
    .pill_w      = 300, .pill_h  = 40,
    .pill_btn_w  = 36,  .pill_btn_h = 34,
    .pill_dot_x  = 40,
    .hero_w      = 292, .hero_h  = 336, .hero_y = 58,
    .arc         = 208, .arc_stroke = 14,
    .soc_dy      = -14, .time_dy = 26,
    .batt_dy     = -38,
    .chip_h      = 32,  .chip_pad = 10, .chip_gap = 6,
    .card_w      = 232, .card_h  = 108,
    .card_y0     = 58,  .card_y1 = 174,
    .card_pad    = 12,  .card_lbl_x = 24,
    .btn_w       = 384, .btn_h   = 50,
    .f_soc       = &lv_font_montserrat_48,
    .f_time      = &lv_font_montserrat_16,
    .f_batt      = &lv_font_montserrat_16,
    .f_card_val  = &lv_font_montserrat_24,
    .f_card_sm   = &lv_font_montserrat_14,
    .f_icon      = &lv_font_montserrat_14,
    .f_btn       = &lv_font_montserrat_16,
};

/*==========================================================================
 *  Widget handles (module-private)
 *========================================================================*/
static lv_obj_t *g_root;

static lv_obj_t *g_arc;
static lv_obj_t *g_soc_pct;
static lv_obj_t *g_time_left;
static lv_obj_t *g_batt_info;

static lv_obj_t *g_chip[3];       /* 0=AC(charger) 1=DC(dc_input) 2=Solar */
static lv_obj_t *g_chip_lbl[3];
static lv_obj_t *g_chip_icon[3];

static lv_obj_t *g_btn_inv,   *g_lbl_inv;
static lv_obj_t *g_btn_dcout, *g_lbl_dcout;

static lv_obj_t *g_ble_icon;
static lv_obj_t *g_usb_icon, *g_usb_lbl;
static lv_obj_t *g_clock;
static lv_obj_t *g_btn_settings;
static lv_obj_t *g_btn_error,  *g_lbl_error;

static lv_obj_t *g_dev_sel, *g_dev_name, *g_dev_dot;

/* System cards: 0=DC out, 1=Solar, 2=Inverter, 3=Charger */
static lv_obj_t *g_sys_card[4];
static lv_obj_t *g_sys_val[4];
static lv_obj_t *g_sys_unit[4];
static lv_obj_t *g_sys_state[4];
static lv_obj_t *g_sys_dot[4];
static lv_obj_t *g_sys_detail[4];

static dashboard_callbacks_t g_cb;
static bool g_layout_bms = false;
static bool g_layout_init = false;

/*==========================================================================
 *  Helpers
 *========================================================================*/
static lv_color_t soc_color(float soc)
{
    if (soc > 50.0f) return COL_GREEN;
    if (soc > 20.0f) return COL_ORANGE;
    return COL_RED;
}

static void set_text_if_changed(lv_obj_t *label, const char *text)
{
    const char *cur = lv_label_get_text(label);
    if (!cur || strcmp(cur, text) != 0) lv_label_set_text(label, text);
}

/* Map a raw source state+failure to a short status word + color. */
static void src_status(int8_t st, uint8_t fail, const char **txt, lv_color_t *col)
{
    if (fail >= 2 && st >= 1) { *txt = "Fault";  *col = (fail >= 3) ? COL_RED : COL_ORANGE; }
    else if (st >= 5)         { *txt = "On";     *col = COL_GREEN; }
    else if (st >= 1)         { *txt = "Active"; *col = COL_ORANGE; }
    else                      { *txt = "Off";    *col = COL_TEXT_DIM; }
}

/* Trampolines so we can store plain function pointers in the model. */
static void ev_inv(lv_event_t *e)     { (void)e; if (g_cb.on_inverter)    g_cb.on_inverter(); }
static void ev_dcout(lv_event_t *e)   { (void)e; if (g_cb.on_dcout)       g_cb.on_dcout(); }
static void ev_settings(lv_event_t *e){ (void)e; if (g_cb.on_settings)    g_cb.on_settings(); }
static void ev_error(lv_event_t *e)   { (void)e; if (g_cb.on_error_badge) g_cb.on_error_badge(); }
static void ev_prev(lv_event_t *e)    { (void)e; if (g_cb.on_dev_prev)    g_cb.on_dev_prev(); }
static void ev_next(lv_event_t *e)    { (void)e; if (g_cb.on_dev_next)    g_cb.on_dev_next(); }

/*==========================================================================
 *  Builders
 *========================================================================*/
static void style_card(lv_obj_t *card, lv_coord_t radius)
{
    lv_obj_set_style_bg_color(card, COL_BG_CARD, 0);
    lv_obj_set_style_bg_grad_color(card, COL_CARD_GRAD, 0);
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, radius, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, COL_CARD_BORDER, 0);
    lv_obj_set_style_border_opa(card, LV_OPA_60, 0);
    lv_obj_set_style_shadow_width(card, 22, 0);
    lv_obj_set_style_shadow_ofs_y(card, 7, 0);
    lv_obj_set_style_shadow_color(card, COL_SHADOW, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_40, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_CLICKABLE);
}

static lv_obj_t *make_dot(lv_obj_t *parent, lv_color_t color)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 10, 10);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(dot, color, 0);
    return dot;
}

static void build_sys_card(int idx, lv_align_t align, lv_coord_t x, lv_coord_t y,
                           const char *icon, const char *label)
{
    lv_obj_t *card = lv_obj_create(g_root);
    g_sys_card[idx] = card;
    lv_obj_set_size(card, L.card_w, L.card_h);
    lv_obj_align(card, align, x, y);
    style_card(card, 16);
    lv_obj_set_style_pad_all(card, L.card_pad, 0);

    lv_obj_t *gi = lv_label_create(card);
    lv_label_set_text(gi, icon);
    lv_obj_set_style_text_color(gi, COL_ACCENT, 0);
    lv_obj_set_style_text_font(gi, L.f_icon, 0);
    lv_obj_align(gi, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(lbl, L.f_card_sm, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, L.card_lbl_x, 1);

    /* Dot + status word live in a right-aligned flex row so the dot follows the
     * label when the status text changes width ("Off" -> "Active"). */
    lv_obj_t *srow = lv_obj_create(card);
    lv_obj_remove_style_all(srow);
    lv_obj_set_size(srow, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(srow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(srow, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(srow, 6, 0);
    lv_obj_align(srow, LV_ALIGN_TOP_RIGHT, 0, 0);

    g_sys_dot[idx] = make_dot(srow, COL_TEXT_DIM);

    g_sys_state[idx] = lv_label_create(srow);
    lv_label_set_text(g_sys_state[idx], "Off");
    lv_obj_set_style_text_color(g_sys_state[idx], COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(g_sys_state[idx], L.f_card_sm, 0);

    /* Value + unit likewise, bottom-aligned so the unit sits on the baseline
     * and slides right as the value grows ("--" -> "312"). */
    lv_obj_t *vrow = lv_obj_create(card);
    lv_obj_remove_style_all(vrow);
    lv_obj_set_size(vrow, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(vrow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vrow, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_gap(vrow, 6, 0);
    lv_obj_set_style_pad_bottom(vrow, 2, 0);
    lv_obj_align(vrow, LV_ALIGN_LEFT_MID, 0, g_compact ? 4 : 6);

    g_sys_val[idx] = lv_label_create(vrow);
    lv_label_set_text(g_sys_val[idx], "--");
    lv_obj_set_style_text_color(g_sys_val[idx], COL_TEXT, 0);
    lv_obj_set_style_text_font(g_sys_val[idx], L.f_card_val, 0);

    g_sys_unit[idx] = lv_label_create(vrow);
    lv_label_set_text(g_sys_unit[idx], "W");
    lv_obj_set_style_text_color(g_sys_unit[idx], COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(g_sys_unit[idx], L.f_icon, 0);

    g_sys_detail[idx] = lv_label_create(card);
    lv_label_set_text(g_sys_detail[idx], "--");
    lv_obj_set_style_text_color(g_sys_detail[idx], COL_TEXT_FAINT, 0);
    lv_obj_set_style_text_font(g_sys_detail[idx], L.f_card_sm, 0);
    lv_obj_align(g_sys_detail[idx], LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

static void build_chip(int idx, lv_obj_t *parent, const char *icon, const char *text)
{
    lv_obj_t *chip = lv_obj_create(parent);
    g_chip[idx] = chip;
    lv_obj_remove_style_all(chip);
    lv_obj_set_size(chip, LV_SIZE_CONTENT, L.chip_h);
    lv_obj_set_style_radius(chip, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(chip, COL_BG_CARD, 0);
    lv_obj_set_style_border_width(chip, 1, 0);
    lv_obj_set_style_border_color(chip, COL_CARD_BORDER, 0);
    lv_obj_set_style_border_opa(chip, LV_OPA_50, 0);
    lv_obj_set_style_pad_hor(chip, L.chip_pad, 0);
    lv_obj_set_flex_flow(chip, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(chip, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(chip, g_compact ? 5 : 7, 0);

    g_chip_icon[idx] = lv_label_create(chip);
    lv_label_set_text(g_chip_icon[idx], icon);
    lv_obj_set_style_text_color(g_chip_icon[idx], COL_TEXT_FAINT, 0);
    lv_obj_set_style_text_font(g_chip_icon[idx], L.f_icon, 0);

    g_chip_lbl[idx] = lv_label_create(chip);
    lv_label_set_text(g_chip_lbl[idx], text);
    lv_obj_set_style_text_color(g_chip_lbl[idx], COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(g_chip_lbl[idx], L.f_card_sm, 0);
}

static lv_obj_t *build_toggle_btn(lv_align_t align, lv_coord_t x_ofs,
                                  const char *text, lv_event_cb_t cb, lv_obj_t **lbl_out)
{
    lv_obj_t *btn = lv_btn_create(g_root);
    lv_obj_set_size(btn, L.btn_w, L.btn_h);
    lv_obj_align(btn, align, x_ofs, -L.margin);
    lv_obj_set_style_bg_color(btn, COL_BG_CARD, 0);
    lv_obj_set_style_radius(btn, 16, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, COL_CARD_BORDER, 0);
    lv_obj_set_style_shadow_width(btn, 16, 0);
    lv_obj_set_style_shadow_ofs_y(btn, 5, 0);
    lv_obj_set_style_shadow_color(btn, COL_SHADOW, 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_40, 0);
    lv_obj_set_style_translate_y(btn, 1, LV_STATE_PRESSED);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, L.f_btn, 0);
    lv_obj_center(lbl);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    *lbl_out = lbl;
    return btn;
}

/*==========================================================================
 *  Public: create
 *========================================================================*/
void dashboard_ui_create(lv_obj_t *parent, const dashboard_callbacks_t *cb)
{
    if (cb) g_cb = *cb;
    g_layout_init = false;

    /* Pick the layout from the real display size. */
    lv_disp_t *disp = lv_obj_get_disp(parent ? parent : lv_scr_act());
    W = lv_disp_get_hor_res(disp);
    H = lv_disp_get_ver_res(disp);
    g_compact = (W < 900) || (H < 540);
    L = g_compact ? LAYOUT_COMPACT : LAYOUT_FULL;

    /* Root page */
    g_root = lv_obj_create(parent);
    lv_obj_set_size(g_root, W, H);
    lv_obj_set_style_bg_color(g_root, COL_BG_DARK, 0);
    lv_obj_set_style_bg_grad_color(g_root, lv_color_hex(0x0C0D14), 0);
    lv_obj_set_style_bg_grad_dir(g_root, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(g_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_root, 0, 0);
    lv_obj_set_style_radius(g_root, 0, 0);
    lv_obj_set_style_pad_all(g_root, 0, 0);
    lv_obj_align(g_root, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_clear_flag(g_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(g_root, LV_OBJ_FLAG_CLICKABLE);

    /*------------------ Hero card (frames the SOC ring) ------------------*/
    lv_obj_t *hero = lv_obj_create(g_root);
    lv_obj_set_size(hero, L.hero_w, L.hero_h);
    lv_obj_align(hero, LV_ALIGN_TOP_MID, 0, L.hero_y);
    style_card(hero, 20);
    lv_obj_set_style_shadow_width(hero, 28, 0);
    lv_obj_set_style_shadow_opa(hero, LV_OPA_50, 0);

    /* SOC arc */
    g_arc = lv_arc_create(hero);
    lv_obj_set_size(g_arc, L.arc, L.arc);
    lv_obj_align(g_arc, LV_ALIGN_TOP_MID, 0, 6);
    lv_arc_set_rotation(g_arc, 135);
    lv_arc_set_bg_angles(g_arc, 0, 270);
    lv_arc_set_range(g_arc, 0, 100);
    lv_arc_set_value(g_arc, 0);
    lv_obj_remove_style(g_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(g_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(g_arc, COL_SOC_BG, LV_PART_MAIN);
    lv_obj_set_style_arc_width(g_arc, L.arc_stroke, LV_PART_MAIN);
    lv_obj_set_style_arc_color(g_arc, COL_SOC_ARC, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(g_arc, L.arc_stroke, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(g_arc, true, LV_PART_INDICATOR);

    g_soc_pct = lv_label_create(hero);
    lv_label_set_text(g_soc_pct, "--%");
    lv_obj_set_style_text_color(g_soc_pct, COL_TEXT, 0);
    lv_obj_set_style_text_font(g_soc_pct, L.f_soc, 0);
    lv_obj_align_to(g_soc_pct, g_arc, LV_ALIGN_CENTER, 0, L.soc_dy);

    g_time_left = lv_label_create(hero);
    lv_label_set_text(g_time_left, "-- min");
    lv_obj_set_style_text_color(g_time_left, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(g_time_left, L.f_time, 0);
    lv_obj_align_to(g_time_left, g_arc, LV_ALIGN_CENTER, 0, L.time_dy);

    g_batt_info = lv_label_create(hero);
    lv_label_set_text(g_batt_info, "--.- V   --.- A");
    lv_obj_set_style_text_color(g_batt_info, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(g_batt_info, L.f_batt, 0);
    lv_obj_align(g_batt_info, LV_ALIGN_BOTTOM_MID, 0, L.batt_dy);

    /* Charging-source chips inside the hero */
    lv_obj_t *chips = lv_obj_create(hero);
    lv_obj_remove_style_all(chips);
    lv_obj_set_size(chips, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(chips, LV_ALIGN_BOTTOM_MID, 0, 2);
    lv_obj_set_flex_flow(chips, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(chips, L.chip_gap, 0);
    build_chip(0, chips, LV_SYMBOL_CHARGE, "AC");
    build_chip(1, chips, LV_SYMBOL_UPLOAD, "DC");
    build_chip(2, chips, LV_SYMBOL_IMAGE,  "Solar");

    /*------------------ Header ------------------*/
    /* Error badge (top-left) */
    g_btn_error = lv_btn_create(g_root);
    lv_obj_set_size(g_btn_error, L.badge_w, L.badge_h);
    lv_obj_align(g_btn_error, LV_ALIGN_TOP_LEFT, L.margin, L.margin);
    lv_obj_set_style_bg_color(g_btn_error, COL_RED, 0);
    lv_obj_set_style_radius(g_btn_error, 14, 0);
    lv_obj_set_style_border_width(g_btn_error, 0, 0);
    lv_obj_set_style_shadow_width(g_btn_error, 16, 0);
    lv_obj_set_style_shadow_color(g_btn_error, COL_RED, 0);
    lv_obj_set_style_shadow_opa(g_btn_error, LV_OPA_30, 0);
    g_lbl_error = lv_label_create(g_btn_error);
    lv_label_set_text(g_lbl_error, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(g_lbl_error, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(g_lbl_error, &lv_font_montserrat_24, 0);
    lv_obj_center(g_lbl_error);
    lv_obj_add_event_cb(g_btn_error, ev_error, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(g_btn_error, LV_OBJ_FLAG_HIDDEN);

    /* BLE icon — just right of the error badge */
    g_ble_icon = lv_label_create(g_root);
    lv_label_set_text(g_ble_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(g_ble_icon, COL_TEXT_FAINT, 0);
    lv_obj_set_style_text_font(g_ble_icon, &lv_font_montserrat_24, 0);
    lv_obj_align(g_ble_icon, LV_ALIGN_TOP_LEFT,
                 L.margin + L.badge_w + 14, L.margin + 10);

    /* USB icon + port-mode tag ("CAN"/"DBG") — right of the BLE icon.
     * Hidden until a USB host attaches (see dashboard_ui_set_usb). */
    g_usb_icon = lv_label_create(g_root);
    lv_label_set_text(g_usb_icon, LV_SYMBOL_USB);
    lv_obj_set_style_text_color(g_usb_icon, COL_TEXT_FAINT, 0);
    lv_obj_set_style_text_font(g_usb_icon, &lv_font_montserrat_24, 0);
    lv_obj_align(g_usb_icon, LV_ALIGN_TOP_LEFT,
                 L.margin + L.badge_w + 14 + 34, L.margin + 10);
    lv_obj_add_flag(g_usb_icon, LV_OBJ_FLAG_HIDDEN);

    g_usb_lbl = lv_label_create(g_root);
    lv_label_set_text(g_usb_lbl, "");
    lv_obj_set_style_text_color(g_usb_lbl, COL_TEXT_FAINT, 0);
    lv_obj_set_style_text_font(g_usb_lbl, L.f_card_sm, 0);
    lv_obj_align(g_usb_lbl, LV_ALIGN_TOP_LEFT,
                 L.margin + L.badge_w + 14 + 34 + 28, L.margin + 16);
    lv_obj_add_flag(g_usb_lbl, LV_OBJ_FLAG_HIDDEN);

    /* Device selector pill (center) */
    g_dev_sel = lv_obj_create(g_root);
    lv_obj_set_size(g_dev_sel, L.pill_w, L.pill_h);
    lv_obj_align(g_dev_sel, LV_ALIGN_TOP_MID, 0, L.margin);
    lv_obj_set_style_bg_color(g_dev_sel, COL_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(g_dev_sel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(g_dev_sel, L.pill_h / 2, 0);
    lv_obj_set_style_border_width(g_dev_sel, 1, 0);
    lv_obj_set_style_border_color(g_dev_sel, COL_CARD_BORDER, 0);
    lv_obj_set_style_pad_all(g_dev_sel, 0, 0);
    lv_obj_clear_flag(g_dev_sel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g_dev_sel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *bp = lv_btn_create(g_dev_sel);
    lv_obj_set_size(bp, L.pill_btn_w, L.pill_btn_h);
    lv_obj_align(bp, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_set_style_bg_opa(bp, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(bp, 0, 0);
    lv_obj_t *bpl = lv_label_create(bp);
    lv_label_set_text(bpl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(bpl, COL_ACCENT, 0);
    lv_obj_center(bpl);
    lv_obj_add_event_cb(bp, ev_prev, LV_EVENT_CLICKED, NULL);

    g_dev_dot = make_dot(g_dev_sel, COL_GREEN);
    lv_obj_align(g_dev_dot, LV_ALIGN_LEFT_MID, L.pill_dot_x, 0);

    g_dev_name = lv_label_create(g_dev_sel);
    lv_label_set_text(g_dev_name, "Unit 1/1");
    lv_obj_set_style_text_color(g_dev_name, COL_TEXT, 0);
    lv_obj_set_style_text_font(g_dev_name, &lv_font_montserrat_16, 0);
    lv_obj_align(g_dev_name, LV_ALIGN_CENTER, 4, 0);

    lv_obj_t *bn = lv_btn_create(g_dev_sel);
    lv_obj_set_size(bn, L.pill_btn_w, L.pill_btn_h);
    lv_obj_align(bn, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_obj_set_style_bg_opa(bn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(bn, 0, 0);
    lv_obj_t *bnl = lv_label_create(bn);
    lv_label_set_text(bnl, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(bnl, COL_ACCENT, 0);
    lv_obj_center(bnl);
    lv_obj_add_event_cb(bn, ev_next, LV_EVENT_CLICKED, NULL);

    /* Clock — left of the settings gear (RTC time, "--:--" until valid) */
    g_clock = lv_label_create(g_root);
    lv_label_set_text(g_clock, "--:--");
    lv_obj_set_style_text_color(g_clock, COL_TEXT_DIM, 0);
    lv_obj_set_style_text_font(g_clock, &lv_font_montserrat_20, 0);
    lv_obj_align(g_clock, LV_ALIGN_TOP_RIGHT,
                 -(L.margin + L.gear + 16), L.margin + (L.gear - 20) / 2);

    /* Settings gear (top-right) */
    g_btn_settings = lv_btn_create(g_root);
    lv_obj_set_size(g_btn_settings, L.gear, L.gear);
    lv_obj_align(g_btn_settings, LV_ALIGN_TOP_RIGHT, -L.margin, L.margin - 2);
    lv_obj_set_style_bg_color(g_btn_settings, COL_ACCENT, 0);
    lv_obj_set_style_bg_grad_color(g_btn_settings, COL_ACCENT_DEEP, 0);
    lv_obj_set_style_bg_grad_dir(g_btn_settings, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_radius(g_btn_settings, 14, 0);
    lv_obj_set_style_shadow_width(g_btn_settings, 18, 0);
    lv_obj_set_style_shadow_color(g_btn_settings, COL_ACCENT_GLOW, 0);
    lv_obj_set_style_shadow_opa(g_btn_settings, LV_OPA_40, 0);
    lv_obj_t *gear = lv_label_create(g_btn_settings);
    lv_label_set_text(gear, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(gear, COL_BG_DARK, 0);
    lv_obj_set_style_text_font(gear, &lv_font_montserrat_24, 0);
    lv_obj_center(gear);
    lv_obj_add_event_cb(g_btn_settings, ev_settings, LV_EVENT_CLICKED, NULL);

    /*------------------ System cards (side margins) ------------------*/
    build_sys_card(0, LV_ALIGN_TOP_LEFT,   L.margin, L.card_y0, LV_SYMBOL_DOWNLOAD, "DC OUTPUT");
    build_sys_card(1, LV_ALIGN_TOP_LEFT,   L.margin, L.card_y1, LV_SYMBOL_IMAGE,    "SOLAR");
    build_sys_card(2, LV_ALIGN_TOP_RIGHT, -L.margin, L.card_y0, LV_SYMBOL_POWER,    "INVERTER");
    build_sys_card(3, LV_ALIGN_TOP_RIGHT, -L.margin, L.card_y1, LV_SYMBOL_CHARGE,   "CHARGER");

    /*------------------ Bottom toggle buttons ------------------*/
    g_btn_inv   = build_toggle_btn(LV_ALIGN_BOTTOM_LEFT,  L.margin,
                                   LV_SYMBOL_POWER " INVERTER", ev_inv, &g_lbl_inv);
    g_btn_dcout = build_toggle_btn(LV_ALIGN_BOTTOM_RIGHT, -L.margin,
                                   LV_SYMBOL_DOWNLOAD " DC OUT", ev_dcout, &g_lbl_dcout);
}

lv_obj_t *dashboard_ui_root(void) { return g_root; }

void dashboard_ui_set_chart_window(int hours) { (void)hours; /* no chart */ }

void dashboard_ui_set_clock(const char *hhmm)
{
    if (!g_clock || !hhmm) return;
    set_text_if_changed(g_clock, hhmm);
}

void dashboard_ui_set_ble(bool connected)
{
    if (!g_ble_icon) return;
    lv_obj_set_style_text_color(g_ble_icon,
        connected ? COL_ACCENT : COL_TEXT_FAINT, 0);
}

void dashboard_ui_set_usb(dash_usb_status_t status)
{
    if (!g_usb_icon) return;

    if (status == DASH_USB_NONE) {
        lv_obj_add_flag(g_usb_icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(g_usb_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_clear_flag(g_usb_icon, LV_OBJ_FLAG_HIDDEN);

    lv_color_t col;
    const char *tag;
    switch (status) {
    case DASH_USB_MODEM: col = COL_GREEN;      tag = "CAN"; break;
    case DASH_USB_DEBUG: col = COL_ORANGE;     tag = "DBG"; break;
    default:             col = COL_TEXT_FAINT; tag = "";    break;
    }
    lv_obj_set_style_text_color(g_usb_icon, col, 0);

    if (tag[0]) {
        set_text_if_changed(g_usb_lbl, tag);
        lv_obj_set_style_text_color(g_usb_lbl, col, 0);
        lv_obj_clear_flag(g_usb_lbl, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(g_usb_lbl, LV_OBJ_FLAG_HIDDEN);
    }
}

/*==========================================================================
 *  Public: update
 *========================================================================*/
static void update_toggle(lv_obj_t *btn, lv_obj_t *lbl, const char *base,
                          int8_t st, uint8_t fl)
{
    lv_color_t bg, border, txt;
    char text[40];
    if (fl >= 2 && st >= 1) {
        bg = (fl >= 3) ? COL_RED : COL_ORANGE; border = bg;
        txt = lv_color_hex(0xffffff);
        snprintf(text, sizeof(text), "%s  !", base);
    } else if (st >= 5) {
        bg = COL_GREEN; border = COL_GREEN; txt = COL_BG_DARK;
        snprintf(text, sizeof(text), "%s ON", base);
    } else if (st >= 1) {
        bg = COL_BG_CARD; border = COL_ORANGE; txt = COL_ORANGE;
        snprintf(text, sizeof(text), "%s ...", base);
    } else {
        bg = COL_BG_CARD; border = COL_TEXT_DIM; txt = COL_TEXT_DIM;
        snprintf(text, sizeof(text), "%s", base);
    }
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_border_color(btn, border, 0);
    lv_obj_set_style_border_width(btn, (st >= 5) ? 0 : 1, 0);
    set_text_if_changed(lbl, text);
    lv_obj_set_style_text_color(lbl, txt, 0);
}

void dashboard_ui_update(const dashboard_model_t *m)
{
    char buf[64];

    /* Device selector */
    if (m->dev_sel_visible) {
        lv_obj_clear_flag(g_dev_sel, LV_OBJ_FLAG_HIDDEN);
        if (m->dev_name) set_text_if_changed(g_dev_name, m->dev_name);
        lv_obj_set_style_bg_color(g_dev_dot, m->dev_online ? COL_GREEN : COL_TEXT_DIM, 0);
    } else {
        lv_obj_add_flag(g_dev_sel, LV_OBJ_FLAG_HIDDEN);
    }

    /* SOC arc + percentage */
    int soc = (int)(m->soc_percent + 0.5f);
    if (soc < 0) soc = 0;
    if (soc > 100) soc = 100;
    lv_arc_set_value(g_arc, soc);
    lv_color_t sc = soc_color(m->soc_percent);
    lv_obj_set_style_arc_color(g_arc, sc, LV_PART_INDICATOR);
    snprintf(buf, sizeof(buf), "%d%%", soc);
    set_text_if_changed(g_soc_pct, buf);
    lv_obj_set_style_text_color(g_soc_pct, sc, 0);

    /* Time left (+ = charging, - = discharging; bench-verified) */
    {
        bool charging = m->battery_current_a > 0.5f;
        bool discharging = m->battery_current_a < -0.5f;
        int tmin = m->soc_time_min < 0 ? -m->soc_time_min : m->soc_time_min;
        if (tmin > 0) {
            int h = tmin / 60, mm = tmin % 60;
            const char *arrow = charging ? LV_SYMBOL_UP : LV_SYMBOL_DOWN;
            if (h > 0) snprintf(buf, sizeof(buf), "%s %dh %dm", arrow, h, mm);
            else       snprintf(buf, sizeof(buf), "%s %d min", arrow, mm);
            lv_obj_set_style_text_color(g_time_left, charging ? COL_GREEN : COL_TEXT_DIM, 0);
        } else if (charging) {
            snprintf(buf, sizeof(buf), LV_SYMBOL_UP " Charging");
            lv_obj_set_style_text_color(g_time_left, COL_GREEN, 0);
        } else if (discharging) {
            snprintf(buf, sizeof(buf), LV_SYMBOL_DOWN " ---");
            lv_obj_set_style_text_color(g_time_left, COL_TEXT_DIM, 0);
        } else {
            snprintf(buf, sizeof(buf), "Standby");
            lv_obj_set_style_text_color(g_time_left, COL_TEXT_DIM, 0);
        }
        set_text_if_changed(g_time_left, buf);
    }

    /* Battery info */
    snprintf(buf, sizeof(buf), "%.1f V   %+.1f A",
             (double)m->battery_voltage_v, (double)m->battery_current_a);
    set_text_if_changed(g_batt_info, buf);

    /* Toggle layout (BMS hides inverter, DC-out spans full width) */
    if (!g_layout_init || g_layout_bms != m->is_bms) {
        if (m->is_bms) {
            lv_obj_add_flag(g_btn_inv, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_width(g_btn_dcout, W - 2 * L.margin);
            lv_obj_align(g_btn_dcout, LV_ALIGN_BOTTOM_MID, 0, -L.margin);
        } else {
            lv_obj_clear_flag(g_btn_inv, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_width(g_btn_dcout, L.btn_w);
            lv_obj_align(g_btn_dcout, LV_ALIGN_BOTTOM_RIGHT, -L.margin, -L.margin);
        }
        g_layout_init = true;
        g_layout_bms = m->is_bms;
    }

    if (!m->is_bms)
        update_toggle(g_btn_inv, g_lbl_inv, LV_SYMBOL_POWER " INVERTER",
                      m->inverter_state, m->inverter_failure);
    update_toggle(g_btn_dcout, g_lbl_dcout, LV_SYMBOL_DOWNLOAD " DC OUT",
                  m->dc_output_state, m->dc_output_failure);

    /* Charging chips */
    {
        int8_t  st[3]  = { m->charger_state, m->dc_input_state, m->solar_state };
        uint8_t fl[3]  = { m->charger_failure, m->dc_input_failure, m->solar_failure };
        lv_color_t okc[3] = { COL_GREEN, COL_GREEN, COL_SOLAR };
        for (int i = 0; i < 3; i++) {
            if (m->is_bms || st[i] < 1) {
                lv_obj_add_flag(g_chip[i], LV_OBJ_FLAG_HIDDEN);
                continue;
            }
            lv_obj_clear_flag(g_chip[i], LV_OBJ_FLAG_HIDDEN);
            lv_color_t c; lv_opa_t border_opa = LV_OPA_90;
            if (fl[i] >= 2)      c = (fl[i] >= 3) ? COL_RED : COL_ORANGE;
            else if (st[i] >= 3) c = okc[i];
            else                 c = COL_ORANGE;
            lv_obj_set_style_border_color(g_chip[i], c, 0);
            lv_obj_set_style_border_opa(g_chip[i], border_opa, 0);
            lv_obj_set_style_bg_color(g_chip[i], COL_BG_PANEL, 0);
            lv_obj_set_style_text_color(g_chip_icon[i], c, 0);
            lv_obj_set_style_text_color(g_chip_lbl[i], COL_TEXT, 0);
        }
    }

    /* Error badge */
    if (m->error_count > 0) {
        lv_obj_clear_flag(g_btn_error, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(g_btn_error, m->error_critical ? COL_RED : COL_ORANGE, 0);
        lv_obj_set_style_shadow_color(g_btn_error, m->error_critical ? COL_RED : COL_ORANGE, 0);
    } else {
        lv_obj_add_flag(g_btn_error, LV_OBJ_FLAG_HIDDEN);
    }

    /* System cards */
    if (m->is_bms) {
        for (int i = 0; i < 4; i++) lv_obj_add_flag(g_sys_card[i], LV_OBJ_FLAG_HIDDEN);
    } else {
        const char *stxt; lv_color_t scol; char vb[24], db[48];
        for (int i = 0; i < 4; i++) lv_obj_clear_flag(g_sys_card[i], LV_OBJ_FLAG_HIDDEN);

        /* 0: DC OUTPUT */
        snprintf(vb, sizeof(vb), "%d",
                 (int)(m->dc_output_voltage_v * m->dc_output_current_a + 0.5f));
        set_text_if_changed(g_sys_val[0], vb);
        set_text_if_changed(g_sys_unit[0], "W");
        snprintf(db, sizeof(db), "%.2f V  |  %.1f A",
                 (double)m->dc_output_voltage_v, (double)m->dc_output_current_a);
        set_text_if_changed(g_sys_detail[0], db);
        src_status(m->dc_output_state, m->dc_output_failure, &stxt, &scol);
        set_text_if_changed(g_sys_state[0], stxt);
        lv_obj_set_style_text_color(g_sys_state[0], scol, 0);
        lv_obj_set_style_bg_color(g_sys_dot[0], scol, 0);

        /* 1: SOLAR */
        snprintf(vb, sizeof(vb), "%.1f", (double)m->solar_current_a);
        set_text_if_changed(g_sys_val[1], vb);
        set_text_if_changed(g_sys_unit[1], "A");
        set_text_if_changed(g_sys_detail[1], m->solar_state >= 1 ? "MPPT active" : "Idle");
        src_status(m->solar_state, m->solar_failure, &stxt, &scol);
        set_text_if_changed(g_sys_state[1], stxt);
        lv_obj_set_style_text_color(g_sys_state[1], scol, 0);
        lv_obj_set_style_bg_color(g_sys_dot[1], scol, 0);

        /* 2: INVERTER */
        snprintf(vb, sizeof(vb), "%u", (unsigned)m->ac_output_power_w);
        set_text_if_changed(g_sys_val[2], vb);
        set_text_if_changed(g_sys_unit[2], "W");
        snprintf(db, sizeof(db), "%.0f V  |  %.2f A",
                 (double)m->ac_output_voltage_v, (double)m->ac_output_current_a);
        set_text_if_changed(g_sys_detail[2], db);
        src_status(m->inverter_state, m->inverter_failure, &stxt, &scol);
        set_text_if_changed(g_sys_state[2], stxt);
        lv_obj_set_style_text_color(g_sys_state[2], scol, 0);
        lv_obj_set_style_bg_color(g_sys_dot[2], scol, 0);

        /* 3: CHARGER */
        snprintf(vb, sizeof(vb), "%u", (unsigned)m->ac_input_power_w);
        set_text_if_changed(g_sys_val[3], vb);
        set_text_if_changed(g_sys_unit[3], "W");
        snprintf(db, sizeof(db), "%.0f V  |  %.2f A",
                 (double)m->ac_input_voltage_v, (double)m->ac_input_current_a);
        set_text_if_changed(g_sys_detail[3], db);
        src_status(m->charger_state, m->charger_failure, &stxt, &scol);
        set_text_if_changed(g_sys_state[3], stxt);
        lv_obj_set_style_text_color(g_sys_state[3], scol, 0);
        lv_obj_set_style_bg_color(g_sys_dot[3], scol, 0);
    }
}

#endif /* !UI_SKIN_CARBON */