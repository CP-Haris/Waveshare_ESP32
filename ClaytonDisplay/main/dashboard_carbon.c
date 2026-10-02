/**
 * @file dashboard_carbon.c
 * @brief "Carbon Blue" dashboard skin (pure LVGL 8.4).
 *
 * Implements the same dashboard_ui.h API as the classic skin, selected with
 * UI_SKIN_CARBON in ui_screen.h. Design reference: the Carbon Blue design
 * specification (docs/design artifacts).
 *
 * Layout (800x480 reference, scaled proportionally on 1024x600):
 *
 *   +------------------------------------------------------------------+
 *   | [! n]  [BT] [USB tag]      < unit >              14:48  [ gear ] |  36
 *   +--------------------------+---------------------------------------+
 *   | BATTERI                  | OPLADNING <<<                  2170 W |
 *   |                          |  (o AC)    (o VEH)    (o SOL)         |
 *   |          76%             |  1240 W     620 W      310 W          | 222
 *   |   [ SoC history +        +---------------------------------------+
 *   |     projection chart ]   | AFLADNING >>>                   916 W |
 *   |                          |  ((o INV))     ((o DCOUT))            |
 *   |  FULD KL. 18:01 - 3H 13M |    683 W          233 W               | 222
 *   +--------------------------+---------------------------------------+
 *
 * State language (from the design spec): the ICON COLOR is the state —
 * grey = off, blue = on, red = blocked/fault, yellow = overload. All dial
 * arcs are ink-white; blue is reserved for the battery prognosis, on-state
 * icons and the connected-BLE icon. Controllable functions (AC out, DC out)
 * sit on circular button plates; everything else is display-only.
 */

#include "dashboard_ui.h"
#include "ui_screen.h"
#include <stdio.h>
#include <string.h>

#if UI_SKIN_CARBON

/* Barlow Semi Condensed (OFL) at the spec sizes — generated with lv_font_conv
 * into main/fonts/. Symbol glyphs (chevrons, header icons) still come from
 * Montserrat, which carries LVGL's FontAwesome range. */
LV_FONT_DECLARE(cb_font_84);
LV_FONT_DECLARE(cb_font_42);
LV_FONT_DECLARE(cb_font_22);
LV_FONT_DECLARE(cb_font_16);
LV_FONT_DECLARE(cb_font_14);

/* Carbon pictograms (A8 alpha, recolorable) — generated into icons_carbon.c */
extern const lv_img_dsc_t cb_ic_plug, cb_ic_car, cb_ic_sun, cb_ic_socket, cb_ic_dc;

/*==========================================================================
 *  Palette (Carbon Blue tokens — see design spec §2)
 *========================================================================*/
#define CB_BG       lv_color_hex(0x0B0C0E)   /* near-black ground           */
#define CB_PANEL    lv_color_hex(0x26292E)   /* grey plates: buttons, popups */
#define CB_PANEL_PR lv_color_hex(0x34383F)   /* plate while pressed         */
#define CB_LINE     lv_color_hex(0x26282C)   /* hairline dividers           */
#define CB_TRACK    lv_color_hex(0x1F2125)   /* empty arc/track             */
#define CB_INK      lv_color_hex(0xEFEDE8)   /* text, arcs, chevrons        */
#define CB_DIM      lv_color_hex(0x82868C)   /* secondary text, off-icons   */
#define CB_FAINT    lv_color_hex(0x4A4E54)   /* micro text, idle sys icons  */
#define CB_BLUE     lv_color_hex(0x4E9EEB)   /* Clayton blue                */
#define CB_BLUE_DIM lv_color_hex(0x2F5F8C)   /* prognosis projection line   */
#define CB_YELLOW   lv_color_hex(0xE6C84A)   /* overload / warning          */
#define CB_RED      lv_color_hex(0xE25454)   /* blocked / fault / critical  */
#define CB_BTN_EDGE lv_color_hex(0x3E434A)   /* circular button border      */

/*==========================================================================
 *  Capacities the dial arcs measure against (design spec §12 — adjust to
 *  the real unit ratings; CAN-provided limits can replace these later).
 *========================================================================*/
#define CAP_AC_IN_W    2000
#define CAP_DC_IN_W    1000
#define CAP_SOLAR_W     800
#define CAP_AC_OUT_W   3000
#define CAP_DC_OUT_W   1200

/*==========================================================================
 *  Geometry (800x480 reference, scaled by the real display size)
 *========================================================================*/
static lv_coord_t W, H;
#define SX(px) ((lv_coord_t)((px) * (int32_t)W / 800))
#define SY(px) ((lv_coord_t)((px) * (int32_t)H / 480))

#define HDR_H      SY(36)
#define LEFT_W     SX(360)
#define DIAL_SIZE  SX(100)
#define BTN_SIZE   SX(114)     /* circular button plate around ctrl dials  */

/* Prognosis chart: time-symmetric x axis. "NU" sits in the middle; the
 * left half is history (-window/2 .. 0), the right half is the future
 * (0 .. +window/2) with tick labels at the quarters (e.g. 4 h window:
 * -2h -1h NU +1h +2h). The window is user-selectable (Graph Window in the
 * menu); the sample period follows: (window/2) / NOW_IDX samples. */
#define CHART_PTS  61
#define NOW_IDX    30
#define WIN_HOURS_DEFAULT 2
static int      g_win_hours = WIN_HOURS_DEFAULT;
static uint32_t g_sample_ms = (uint32_t)WIN_HOURS_DEFAULT * 3600000u / (2u * NOW_IDX);

/*==========================================================================
 *  Widget handles
 *========================================================================*/
static lv_obj_t *g_root;

/* Header */
static lv_obj_t *g_btn_error, *g_err_icon, *g_err_cnt;
static lv_obj_t *g_ble_icon;
static lv_obj_t *g_usb_icon, *g_usb_lbl;
static lv_obj_t *g_clock;
static lv_obj_t *g_btn_settings;
static lv_obj_t *g_dev_sel, *g_dev_name, *g_dev_dot;

/* Battery zone */
static lv_obj_t *g_soc_pct, *g_soc_unit;
static lv_obj_t *g_chart;
static lv_chart_series_t *g_ser_hist;
static lv_obj_t *g_until;
static lv_obj_t *g_proj_line;            /* dashed projection overlay        */
static lv_point_t g_proj_pts[3];         /* must outlive the lv_line         */
static lv_obj_t *g_now_dot, *g_tgt_ring; /* "now" dot + projection target    */
static lv_obj_t *g_tax[5];               /* time axis: -H/2 -H/4 NU +H/4 +H/2 */
static lv_coord_t g_ch_x0, g_ch_y0, g_ch_w, g_ch_h; /* chart geometry (lz)   */

/* Power zones */
static lv_obj_t *g_zone_chg, *g_zone_dis;
static lv_obj_t *g_chg_title, *g_dis_title;
static lv_obj_t *g_chg_total, *g_dis_total;
static lv_obj_t *g_chg_chev[3], *g_dis_chev[3];

/* Dials: charge 0=AC in(charger) 1=vehicle(dc input) 2=solar;
 *        discharge 0=AC out(inverter) 1=DC out. */
static lv_obj_t *g_cd_cell[3], *g_cd_arc[3], *g_cd_icon[3], *g_cd_val[3], *g_cd_unit[3];
static lv_obj_t *g_dd_btn[2], *g_dd_arc[2], *g_dd_icon[2], *g_dd_val[2], *g_dd_unit[2];
static lv_obj_t *g_chg_unit, *g_dis_unit;
static bool g_dd_pulse[2];

static dashboard_callbacks_t g_cb;
static bool g_layout_bms = false;
static bool g_layout_init = false;

/* SoC history ring (values 0..100). Only g_hist_count samples are real —
 * the rest of the chart stays LV_CHART_POINT_NONE so the curve grows
 * leftward from "now" as time actually passes (no fabricated history). */
static int16_t  g_hist[NOW_IDX + 1];
static int      g_hist_count = 0;
static bool     g_hist_init = false;
static uint32_t g_last_sample;

/* Cached clock, parsed for the prognosis target time (-1 = invalid) */
static int g_clock_min = -1;

/*==========================================================================
 *  Helpers
 *========================================================================*/
static void set_text_if_changed(lv_obj_t *label, const char *text)
{
    const char *cur = lv_label_get_text(label);
    if (!cur || strcmp(cur, text) != 0) lv_label_set_text(label, text);
}

/* Icon color IS the state: grey=off, blue=on, red=blocked/fault, yellow=overload. */
static lv_color_t func_color(int8_t st, uint8_t fail)
{
    if (fail >= 3) return CB_RED;          /* fault / blocked               */
    if (fail == 2) return CB_YELLOW;       /* warning-level -> overload     */
    if (st   >= 1) return CB_BLUE;         /* starting or on                */
    return CB_DIM;                         /* off                           */
}

static void ev_inv(lv_event_t *e)     { (void)e; if (g_cb.on_inverter)    g_cb.on_inverter(); }
static void ev_dcout(lv_event_t *e)   { (void)e; if (g_cb.on_dcout)       g_cb.on_dcout(); }
static void ev_settings(lv_event_t *e){ (void)e; if (g_cb.on_settings)    g_cb.on_settings(); }
static void ev_error(lv_event_t *e)   { (void)e; if (g_cb.on_error_badge) g_cb.on_error_badge(); }
static void ev_prev(lv_event_t *e)    { (void)e; if (g_cb.on_dev_prev)    g_cb.on_dev_prev(); }
static void ev_next(lv_event_t *e)    { (void)e; if (g_cb.on_dev_next)    g_cb.on_dev_next(); }

/* Opacity pulse animations (marching chevrons + overload arc) */
static void anim_obj_opa(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}
static void anim_arc_opa(void *var, int32_t v)
{
    lv_obj_set_style_arc_opa((lv_obj_t *)var, (lv_opa_t)v, LV_PART_INDICATOR);
}

static void start_march(lv_obj_t *obj, uint32_t delay_ms)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, anim_obj_opa);
    lv_anim_set_values(&a, LV_OPA_30, LV_OPA_COVER);
    lv_anim_set_time(&a, 720);
    lv_anim_set_playback_time(&a, 1080);
    lv_anim_set_delay(&a, delay_ms);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

static void set_overload_pulse(int idx, bool on)
{
    if (g_dd_pulse[idx] == on) return;
    g_dd_pulse[idx] = on;
    if (on) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, g_dd_arc[idx]);
        lv_anim_set_exec_cb(&a, anim_arc_opa);
        lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_40);
        lv_anim_set_time(&a, 500);
        lv_anim_set_playback_time(&a, 500);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&a);
    } else {
        lv_anim_del(g_dd_arc[idx], anim_arc_opa);
        lv_obj_set_style_arc_opa(g_dd_arc[idx], LV_OPA_COVER, LV_PART_INDICATOR);
    }
}

/*==========================================================================
 *  Builders
 *========================================================================*/
static lv_obj_t *plain_container(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
    return o;
}

/* 270-degree dial arc, gap facing down (matches the design's round gauges). */
static lv_obj_t *make_dial_arc(lv_obj_t *parent, lv_coord_t size)
{
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc, CB_TRACK, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, SX(9), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, CB_INK, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, SX(9), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_INDICATOR);
    return arc;
}

static lv_obj_t *make_dial_icon(lv_obj_t *arc, const lv_img_dsc_t *src)
{
    lv_obj_t *icon = lv_img_create(arc);
    lv_img_set_src(icon, src);
    lv_obj_set_style_img_recolor(icon, CB_DIM, 0);
    lv_obj_set_style_img_recolor_opa(icon, LV_OPA_COVER, 0);
    lv_obj_center(icon);
    return icon;
}

/* Value + dimmed unit pair ("1240" 22px ink + "W" 14px dim). The unit is
 * re-anchored after every text change (see place_value_pair). */
static lv_obj_t *make_value_label(lv_obj_t *parent, lv_obj_t **unit_out)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "--");
    lv_obj_set_style_text_color(lbl, CB_INK, 0);
    lv_obj_set_style_text_font(lbl, &cb_font_22, 0);

    lv_obj_t *unit = lv_label_create(parent);
    lv_label_set_text(unit, "W");
    lv_obj_set_style_text_color(unit, CB_DIM, 0);
    lv_obj_set_style_text_font(unit, &cb_font_14, 0);
    *unit_out = unit;
    return lbl;
}

/* Center the value under its anchor (dial cell / button plate) and hang the
 * dimmed unit off its right edge. Must be re-run after text changes. */
static void place_value_pair(lv_obj_t *val, lv_obj_t *unit, lv_obj_t *anchor,
                             lv_coord_t dy)
{
    lv_obj_update_layout(val);
    lv_obj_align_to(val, anchor, LV_ALIGN_OUT_BOTTOM_MID, -SX(7), dy);
    lv_obj_align_to(unit, val, LV_ALIGN_OUT_RIGHT_BOTTOM, SX(3), -SY(3));
}

static lv_obj_t *make_zone_title(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, CB_INK, 0);
    lv_obj_set_style_text_font(lbl, &cb_font_16, 0);
    lv_obj_set_style_text_letter_space(lbl, 3, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, SX(24), SY(14));
    return lbl;
}

/* Three marching chevrons right of a zone title. dir_in points toward the
 * battery (left) for charging; away (right) for discharging. The march
 * order follows the energy direction. */
static void make_chevrons(lv_obj_t *parent, lv_obj_t *title, bool dir_in,
                          lv_obj_t **out)
{
    for (int i = 0; i < 3; i++) {
        lv_obj_t *c = lv_label_create(parent);
        lv_label_set_text(c, dir_in ? LV_SYMBOL_LEFT : LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(c, CB_INK, 0);
        lv_obj_set_style_text_font(c, &lv_font_montserrat_14, 0);
        lv_obj_align_to(c, title, LV_ALIGN_OUT_RIGHT_MID, SX(14) + i * SX(11), 0);
        out[i] = c;
        /* march toward the battery when charging: rightmost chevron first */
        uint32_t delay = dir_in ? (uint32_t)(2 - i) * 250u : (uint32_t)i * 250u;
        start_march(c, delay);
    }
}

/* Zone total: 42px ink value with a small dimmed "W" fixed at the zone's
 * right edge; the value hangs off the unit's left side and grows leftward. */
static lv_obj_t *make_zone_total(lv_obj_t *parent, lv_obj_t **unit_out)
{
    lv_obj_t *unit = lv_label_create(parent);
    lv_label_set_text(unit, "W");
    lv_obj_set_style_text_color(unit, CB_DIM, 0);
    lv_obj_set_style_text_font(unit, &cb_font_16, 0);
    lv_obj_align(unit, LV_ALIGN_TOP_RIGHT, -SX(24), SY(16));
    *unit_out = unit;

    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "--");
    lv_obj_set_style_text_color(lbl, CB_INK, 0);
    lv_obj_set_style_text_font(lbl, &cb_font_42, 0);
    lv_obj_align_to(lbl, unit, LV_ALIGN_OUT_LEFT_TOP, -SX(6), -SY(8));
    return lbl;
}

static void place_total(lv_obj_t *val, lv_obj_t *unit)
{
    lv_obj_update_layout(val);
    lv_obj_align_to(val, unit, LV_ALIGN_OUT_LEFT_TOP, -SX(6), -SY(8));
}

static void chart_fill_cb(lv_event_t *e);   /* defined with the update code */
static void update_time_axis(void);

/*==========================================================================
 *  Public: create
 *========================================================================*/
void dashboard_ui_create(lv_obj_t *parent, const dashboard_callbacks_t *cb)
{
    if (cb) g_cb = *cb;
    g_layout_init = false;
    g_hist_init = false;
    g_dd_pulse[0] = g_dd_pulse[1] = false;

    lv_disp_t *disp = lv_obj_get_disp(parent ? parent : lv_scr_act());
    W = lv_disp_get_hor_res(disp);
    H = lv_disp_get_ver_res(disp);

    /* Root */
    g_root = lv_obj_create(parent);
    lv_obj_set_size(g_root, W, H);
    lv_obj_set_style_bg_color(g_root, CB_BG, 0);
    lv_obj_set_style_bg_opa(g_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_root, 0, 0);
    lv_obj_set_style_radius(g_root, 0, 0);
    lv_obj_set_style_pad_all(g_root, 0, 0);
    lv_obj_align(g_root, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_clear_flag(g_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(g_root, LV_OBJ_FLAG_CLICKABLE);

    /*---------------- Status bar (36 px) ----------------*/
    lv_obj_t *hdr = plain_container(g_root);
    lv_obj_set_size(hdr, W, HDR_H);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_border_width(hdr, 1, 0);
    lv_obj_set_style_border_color(hdr, CB_LINE, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, 0);

    /* Error badge: dim outline when clear, colored icon + count when active */
    g_btn_error = lv_btn_create(hdr);
    lv_obj_set_size(g_btn_error, SX(64), HDR_H);
    lv_obj_align(g_btn_error, LV_ALIGN_LEFT_MID, SX(10), 0);
    lv_obj_set_style_bg_opa(g_btn_error, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(g_btn_error, 0, 0);
    lv_obj_set_style_border_width(g_btn_error, 0, 0);
    g_err_icon = lv_label_create(g_btn_error);
    lv_label_set_text(g_err_icon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(g_err_icon, CB_FAINT, 0);
    lv_obj_set_style_text_font(g_err_icon, &lv_font_montserrat_16, 0);
    lv_obj_align(g_err_icon, LV_ALIGN_LEFT_MID, SX(4), 0);
    g_err_cnt = lv_label_create(g_btn_error);
    lv_label_set_text(g_err_cnt, "");
    lv_obj_set_style_text_color(g_err_cnt, CB_RED, 0);
    lv_obj_set_style_text_font(g_err_cnt, &cb_font_16, 0);
    lv_obj_align(g_err_cnt, LV_ALIGN_LEFT_MID, SX(28), 0);
    lv_obj_add_event_cb(g_btn_error, ev_error, LV_EVENT_CLICKED, NULL);

    /* BLE — blue when the app is connected */
    g_ble_icon = lv_label_create(hdr);
    lv_label_set_text(g_ble_icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(g_ble_icon, CB_FAINT, 0);
    lv_obj_set_style_text_font(g_ble_icon, &lv_font_montserrat_16, 0);
    lv_obj_align(g_ble_icon, LV_ALIGN_LEFT_MID, SX(84), 0);

    /* USB + port-mode tag — hidden until a host attaches */
    g_usb_icon = lv_label_create(hdr);
    lv_label_set_text(g_usb_icon, LV_SYMBOL_USB);
    lv_obj_set_style_text_color(g_usb_icon, CB_INK, 0);
    lv_obj_set_style_text_font(g_usb_icon, &lv_font_montserrat_16, 0);
    lv_obj_align(g_usb_icon, LV_ALIGN_LEFT_MID, SX(114), 0);
    lv_obj_add_flag(g_usb_icon, LV_OBJ_FLAG_HIDDEN);
    g_usb_lbl = lv_label_create(hdr);
    lv_label_set_text(g_usb_lbl, "");
    lv_obj_set_style_text_color(g_usb_lbl, CB_DIM, 0);
    lv_obj_set_style_text_font(g_usb_lbl, &cb_font_14, 0);
    lv_obj_align(g_usb_lbl, LV_ALIGN_LEFT_MID, SX(140), 0);
    lv_obj_add_flag(g_usb_lbl, LV_OBJ_FLAG_HIDDEN);

    /* Device selector (compact, centered; only shown with >1 unit online) */
    g_dev_sel = plain_container(hdr);
    lv_obj_set_size(g_dev_sel, SX(240), HDR_H);
    lv_obj_align(g_dev_sel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(g_dev_sel, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *bp = lv_btn_create(g_dev_sel);
    lv_obj_set_size(bp, SX(34), HDR_H);
    lv_obj_align(bp, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(bp, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(bp, 0, 0);
    lv_obj_t *bpl = lv_label_create(bp);
    lv_label_set_text(bpl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(bpl, CB_DIM, 0);
    lv_obj_center(bpl);
    lv_obj_add_event_cb(bp, ev_prev, LV_EVENT_CLICKED, NULL);

    g_dev_dot = lv_obj_create(g_dev_sel);
    lv_obj_remove_style_all(g_dev_dot);
    lv_obj_set_size(g_dev_dot, 8, 8);
    lv_obj_set_style_radius(g_dev_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(g_dev_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(g_dev_dot, CB_BLUE, 0);
    lv_obj_align(g_dev_dot, LV_ALIGN_LEFT_MID, SX(40), 0);

    g_dev_name = lv_label_create(g_dev_sel);
    lv_label_set_text(g_dev_name, "");
    lv_obj_set_style_text_color(g_dev_name, CB_INK, 0);
    lv_obj_set_style_text_font(g_dev_name, &cb_font_14, 0);
    lv_obj_align(g_dev_name, LV_ALIGN_CENTER, SX(4), 0);

    lv_obj_t *bn = lv_btn_create(g_dev_sel);
    lv_obj_set_size(bn, SX(34), HDR_H);
    lv_obj_align(bn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(bn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(bn, 0, 0);
    lv_obj_t *bnl = lv_label_create(bn);
    lv_label_set_text(bnl, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(bnl, CB_DIM, 0);
    lv_obj_center(bnl);
    lv_obj_add_event_cb(bn, ev_next, LV_EVENT_CLICKED, NULL);

    /* Settings gear (own field with a left hairline) */
    g_btn_settings = lv_btn_create(hdr);
    lv_obj_set_size(g_btn_settings, SX(48), HDR_H);
    lv_obj_align(g_btn_settings, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(g_btn_settings, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(g_btn_settings, 0, 0);
    lv_obj_set_style_radius(g_btn_settings, 0, 0);
    lv_obj_set_style_border_width(g_btn_settings, 1, 0);
    lv_obj_set_style_border_color(g_btn_settings, CB_LINE, 0);
    lv_obj_set_style_border_side(g_btn_settings, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_t *gear = lv_label_create(g_btn_settings);
    lv_label_set_text(gear, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(gear, CB_DIM, 0);
    lv_obj_set_style_text_font(gear, &lv_font_montserrat_16, 0);
    lv_obj_center(gear);
    lv_obj_add_event_cb(g_btn_settings, ev_settings, LV_EVENT_CLICKED, NULL);

    /* Clock, left of the gear */
    g_clock = lv_label_create(hdr);
    lv_label_set_text(g_clock, "--:--");
    lv_obj_set_style_text_color(g_clock, CB_INK, 0);
    lv_obj_set_style_text_font(g_clock, &cb_font_16, 0);
    lv_obj_align(g_clock, LV_ALIGN_RIGHT_MID, -SX(62), 0);

    /*---------------- Battery zone (left) ----------------*/
    lv_obj_t *lz = plain_container(g_root);
    lv_obj_set_size(lz, LEFT_W, H - HDR_H);
    lv_obj_set_pos(lz, 0, HDR_H);
    lv_obj_set_style_border_width(lz, 1, 0);
    lv_obj_set_style_border_color(lz, CB_LINE, 0);
    lv_obj_set_style_border_side(lz, LV_BORDER_SIDE_RIGHT, 0);

    lv_obj_t *btitle = lv_label_create(lz);
    lv_label_set_text(btitle, "BATTERI");
    lv_obj_set_style_text_color(btitle, CB_INK, 0);
    lv_obj_set_style_text_font(btitle, &cb_font_16, 0);
    lv_obj_set_style_text_letter_space(btitle, 3, 0);
    lv_obj_align(btitle, LV_ALIGN_TOP_LEFT, SX(24), SY(14));

    /* SoC — Barlow Semi Condensed Bold 84 px, the field's hero; the unit
     * sign is a separate, smaller and dimmed label (design spec §3). */
    g_soc_pct = lv_label_create(lz);
    lv_label_set_text(g_soc_pct, "--");
    lv_obj_set_style_text_color(g_soc_pct, CB_INK, 0);
    lv_obj_set_style_text_font(g_soc_pct, &cb_font_84, 0);
    lv_obj_align(g_soc_pct, LV_ALIGN_TOP_MID, -SX(14), SY(34));

    g_soc_unit = lv_label_create(lz);
    lv_label_set_text(g_soc_unit, "%");
    lv_obj_set_style_text_color(g_soc_unit, CB_DIM, 0);
    lv_obj_set_style_text_font(g_soc_unit, &cb_font_42, 0);
    lv_obj_align_to(g_soc_unit, g_soc_pct, LV_ALIGN_OUT_RIGHT_BOTTOM, SX(3), -SY(10));

    /* Prognosis chart: history series; projection drawn as a dashed lv_line
     * overlay with a target ring; "now" marked with hairline + ink dot. */
    g_ch_w  = SX(312);
    g_ch_h  = SY(176);
    g_ch_x0 = (LEFT_W - g_ch_w) / 2;
    g_ch_y0 = SY(138);

    g_chart = lv_chart_create(lz);
    lv_obj_set_size(g_chart, g_ch_w, g_ch_h);
    lv_obj_set_pos(g_chart, g_ch_x0, g_ch_y0);
    lv_chart_set_type(g_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(g_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(g_chart, CHART_PTS);
    lv_chart_set_div_line_count(g_chart, 3, 3);   /* 25/50/75 % + quarter ticks */
    lv_obj_set_style_bg_opa(g_chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_chart, 0, 0);
    lv_obj_set_style_pad_all(g_chart, 0, 0);
    lv_obj_set_style_radius(g_chart, 0, 0);
    lv_obj_set_style_line_color(g_chart, CB_LINE, LV_PART_MAIN);
    lv_obj_set_style_line_width(g_chart, 1, LV_PART_MAIN);
    lv_obj_set_style_line_width(g_chart, 3, LV_PART_ITEMS);
    lv_obj_set_style_size(g_chart, 0, LV_PART_INDICATOR);   /* no point dots */
    /* history occupies the left 55%; the rest of the series stays NONE so
     * the grid spans the full prognosis area but the line stops at "now" */
    g_ser_hist = lv_chart_add_series(g_chart, CB_BLUE, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(g_chart, g_ser_hist, LV_CHART_POINT_NONE);
    lv_obj_add_event_cb(g_chart, chart_fill_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);

    /* % axis labels left of the grid lines */
    static const uint8_t axis_v[4] = { 25, 50, 75, 100 };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *al = lv_label_create(lz);
        lv_label_set_text_fmt(al, "%d", axis_v[i]);
        lv_obj_set_style_text_color(al, CB_DIM, 0);
        lv_obj_set_style_text_font(al, &cb_font_16, 0);
        lv_obj_align(al, LV_ALIGN_TOP_RIGHT,
                     -(LEFT_W - g_ch_x0) - SX(5),
                     g_ch_y0 + (100 - axis_v[i]) * (g_ch_h - 1) / 100 - SY(9));
    }

    /* "now" hairline + dot; dashed projection + target ring */
    lv_coord_t now_x = g_ch_x0 + g_ch_w * NOW_IDX / (CHART_PTS - 1);
    lv_obj_t *nline = plain_container(lz);
    lv_obj_set_size(nline, 1, g_ch_h + SY(10));
    lv_obj_set_pos(nline, now_x, g_ch_y0 - SY(4));
    lv_obj_set_style_bg_color(nline, CB_LINE, 0);
    lv_obj_set_style_bg_opa(nline, LV_OPA_COVER, 0);

    g_proj_line = lv_line_create(lz);
    lv_obj_set_pos(g_proj_line, 0, 0);
    lv_obj_set_style_line_color(g_proj_line, CB_BLUE, 0);
    lv_obj_set_style_line_width(g_proj_line, 2, 0);
    lv_obj_set_style_line_opa(g_proj_line, LV_OPA_70, 0);
    lv_obj_set_style_line_dash_width(g_proj_line, 5, 0);
    lv_obj_set_style_line_dash_gap(g_proj_line, 6, 0);

    g_now_dot = plain_container(lz);
    lv_obj_set_size(g_now_dot, SX(10), SX(10));
    lv_obj_set_style_radius(g_now_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(g_now_dot, CB_INK, 0);
    lv_obj_set_style_bg_opa(g_now_dot, LV_OPA_COVER, 0);

    g_tgt_ring = plain_container(lz);
    lv_obj_set_size(g_tgt_ring, SX(10), SX(10));
    lv_obj_set_style_radius(g_tgt_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(g_tgt_ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_tgt_ring, 2, 0);
    lv_obj_set_style_border_color(g_tgt_ring, CB_BLUE, 0);

    /* time axis: 5 fixed ticks -H/2 -H/4 NU +H/4 +H/2 (texts follow the
     * selected Graph Window — see update_time_axis) */
    for (int i = 0; i < 5; i++) {
        g_tax[i] = lv_label_create(lz);
        lv_label_set_text(g_tax[i], "");
        lv_obj_set_style_text_color(g_tax[i], (i == 2) ? CB_INK : CB_DIM, 0);
        lv_obj_set_style_text_font(g_tax[i], &cb_font_16, 0);
    }
    update_time_axis();

    /* Prognosis line: "FULD KL. 18:01 · OM 3H 13M" (recolored target) */
    g_until = lv_label_create(lz);
    lv_label_set_recolor(g_until, true);
    lv_label_set_text(g_until, "");
    lv_obj_set_style_text_color(g_until, lv_color_hex(0xB9BCB6), 0);
    lv_obj_set_style_text_font(g_until, &cb_font_16, 0);
    lv_obj_align(g_until, LV_ALIGN_TOP_MID, 0, SY(352));

    /*---------------- Charge zone (top right) ----------------*/
    g_zone_chg = plain_container(g_root);
    lv_obj_set_size(g_zone_chg, W - LEFT_W, (H - HDR_H) / 2);
    lv_obj_set_pos(g_zone_chg, LEFT_W, HDR_H);

    g_chg_title = make_zone_title(g_zone_chg, "OPLADNING");
    make_chevrons(g_zone_chg, g_chg_title, true, g_chg_chev);
    g_chg_total = make_zone_total(g_zone_chg, &g_chg_unit);

    static const lv_img_dsc_t *chg_sym[3] = { &cb_ic_plug, &cb_ic_car, &cb_ic_sun };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *cell = plain_container(g_zone_chg);
        g_cd_cell[i] = cell;
        lv_obj_set_size(cell, DIAL_SIZE, DIAL_SIZE);
        lv_obj_set_pos(cell, SX(24) + i * (DIAL_SIZE + SX(46)), SY(58));
        g_cd_arc[i] = make_dial_arc(cell, DIAL_SIZE);
        lv_obj_center(g_cd_arc[i]);
        g_cd_icon[i] = make_dial_icon(g_cd_arc[i], chg_sym[i]);
        g_cd_val[i] = make_value_label(g_zone_chg, &g_cd_unit[i]);
        place_value_pair(g_cd_val[i], g_cd_unit[i], cell, SY(4));
    }

    /*---------------- Discharge zone (bottom right) ----------------*/
    g_zone_dis = plain_container(g_root);
    lv_obj_set_size(g_zone_dis, W - LEFT_W, (H - HDR_H) / 2);
    lv_obj_set_pos(g_zone_dis, LEFT_W, HDR_H + (H - HDR_H) / 2);
    lv_obj_set_style_border_width(g_zone_dis, 1, 0);
    lv_obj_set_style_border_color(g_zone_dis, CB_LINE, 0);
    lv_obj_set_style_border_side(g_zone_dis, LV_BORDER_SIDE_TOP, 0);

    g_dis_title = make_zone_title(g_zone_dis, "AFLADNING");
    make_chevrons(g_zone_dis, g_dis_title, false, g_dis_chev);
    g_dis_total = make_zone_total(g_zone_dis, &g_dis_unit);

    /* Controllable functions sit on circular button plates ("what looks
     * like a button is a button"). */
    static const lv_img_dsc_t *dis_sym[2] = { &cb_ic_socket, &cb_ic_dc };
    lv_event_cb_t dis_cb[2] = { ev_inv, ev_dcout };
    for (int i = 0; i < 2; i++) {
        lv_obj_t *btn = lv_btn_create(g_zone_dis);
        g_dd_btn[i] = btn;
        lv_obj_set_size(btn, BTN_SIZE, BTN_SIZE);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, (i == 0) ? -SX(93) : SX(93), SY(48));
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(btn, CB_PANEL, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, CB_BTN_EDGE, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_bg_color(btn, CB_PANEL_PR, LV_STATE_PRESSED);
        lv_obj_add_event_cb(btn, dis_cb[i], LV_EVENT_CLICKED, NULL);

        g_dd_arc[i] = make_dial_arc(btn, DIAL_SIZE);
        lv_obj_center(g_dd_arc[i]);
        /* clicks land on the plate, not the arc */
        lv_obj_clear_flag(g_dd_arc[i], LV_OBJ_FLAG_CLICKABLE);
        g_dd_icon[i] = make_dial_icon(g_dd_arc[i], dis_sym[i]);

        g_dd_val[i] = make_value_label(g_zone_dis, &g_dd_unit[i]);
        place_value_pair(g_dd_val[i], g_dd_unit[i], btn, SY(2));
    }
}

lv_obj_t *dashboard_ui_root(void) { return g_root; }

/*==========================================================================
 *  Public: low-rate setters
 *========================================================================*/
void dashboard_ui_set_chart_window(int hours)
{
    if (hours < 1) hours = 1;
    if (hours > 24) hours = 24;
    if (hours == g_win_hours) return;
    g_win_hours = hours;
    g_sample_ms = (uint32_t)hours * 3600000u / (2u * NOW_IDX);
    g_hist_init = false;   /* restart the buffer from the next SoC sample */
    update_time_axis();
}

void dashboard_ui_set_clock(const char *hhmm)
{
    if (!g_clock || !hhmm) return;
    set_text_if_changed(g_clock, hhmm);

    int h, m;
    if (sscanf(hhmm, "%d:%d", &h, &m) == 2 &&
        h >= 0 && h < 24 && m >= 0 && m < 60)
        g_clock_min = h * 60 + m;
    else
        g_clock_min = -1;
}

void dashboard_ui_set_ble(bool connected)
{
    if (!g_ble_icon) return;
    lv_obj_set_style_text_color(g_ble_icon, connected ? CB_BLUE : CB_FAINT, 0);
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

    const char *tag = (status == DASH_USB_MODEM) ? "CAN"
                    : (status == DASH_USB_DEBUG) ? "DBG" : "";
    lv_obj_set_style_text_color(g_usb_icon,
        (status == DASH_USB_IDLE) ? CB_FAINT : CB_INK, 0);
    if (tag[0]) {
        set_text_if_changed(g_usb_lbl, tag);
        lv_obj_clear_flag(g_usb_lbl, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(g_usb_lbl, LV_OBJ_FLAG_HIDDEN);
    }
}

/*==========================================================================
 *  Update helpers
 *========================================================================*/
/* Returns true when the function is in overload: running, no hard fault,
 * and the live value exceeds the bar's max (CAN-fetched or default). */
static bool set_dial(lv_obj_t *arc, lv_obj_t *icon,
                     lv_obj_t *val, lv_obj_t *unit, lv_obj_t *anchor,
                     lv_coord_t dy, int power_w, int cap_w,
                     int8_t st, uint8_t fail)
{
    bool overload = (st >= 1 && fail < 3 && cap_w > 0 && power_w > cap_w);
    lv_obj_set_style_img_recolor(icon,
        overload ? CB_YELLOW : func_color(st, fail), 0);

    int pct;
    if (overload)                pct = 100;
    else if (fail >= 3 || st < 1) pct = 0;
    else {
        pct = (cap_w > 0) ? (power_w * 100 + cap_w / 2) / cap_w : 0;
        if (pct > 100) pct = 100;
        if (pct < 0)   pct = 0;
    }
    lv_arc_set_value(arc, pct);
    lv_obj_set_style_arc_color(arc, overload ? CB_YELLOW : CB_INK, LV_PART_INDICATOR);

    char buf[20];
    bool zero = (fail >= 3 || st < 1);
    snprintf(buf, sizeof(buf), "%d", zero ? 0 : power_w);
    set_text_if_changed(val, buf);
    lv_obj_set_style_text_color(val, zero ? CB_DIM : CB_INK, 0);
    place_value_pair(val, unit, anchor, dy);
    return overload;
}

/* Bar full scale: prefer the CAN-fetched max, fall back to the default. */
static int bar_cap(float fetched_w, int fallback_w)
{
    return (fetched_w > 1.0f) ? (int)(fetched_w + 0.5f) : fallback_w;
}

static void set_chevrons(lv_obj_t **chev, bool active)
{
    for (int i = 0; i < 3; i++) {
        if (active) lv_obj_clear_flag(chev[i], LV_OBJ_FLAG_HIDDEN);
        else        lv_obj_add_flag(chev[i], LV_OBJ_FLAG_HIDDEN);
    }
}

/* Faded blue fill under the history line (design spec §5.2). LVGL 8 charts
 * have no native area fill, so each line segment draws a masked rect: a line
 * mask keeps only the area below the segment, a fade mask lets it dissolve
 * toward the chart bottom. Requires LV_DRAW_COMPLEX (already on — the arcs
 * need it too). */
static void chart_fill_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
    if (dsc->part != LV_PART_ITEMS || !dsc->p1 || !dsc->p2) return;

    lv_draw_mask_line_param_t line_mask_param;
    lv_draw_mask_line_points_init(&line_mask_param,
                                  dsc->p1->x, dsc->p1->y,
                                  dsc->p2->x, dsc->p2->y,
                                  LV_DRAW_MASK_LINE_SIDE_BOTTOM);
    int16_t line_mask_id = lv_draw_mask_add(&line_mask_param, NULL);

    lv_draw_mask_fade_param_t fade_mask_param;
    lv_draw_mask_fade_init(&fade_mask_param, &obj->coords,
                           LV_OPA_COVER, obj->coords.y1,
                           LV_OPA_TRANSP, obj->coords.y2);
    int16_t fade_mask_id = lv_draw_mask_add(&fade_mask_param, NULL);

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_opa = LV_OPA_20;
    rect_dsc.bg_color = CB_BLUE;

    lv_area_t a;
    a.x1 = dsc->p1->x;
    a.x2 = dsc->p2->x - 1;
    a.y1 = LV_MIN(dsc->p1->y, dsc->p2->y);
    a.y2 = obj->coords.y2;
    lv_draw_rect(dsc->draw_ctx, &rect_dsc, &a);

    lv_draw_mask_free_param(&line_mask_param);
    lv_draw_mask_free_param(&fade_mask_param);
    lv_draw_mask_remove_id(line_mask_id);
    lv_draw_mask_remove_id(fade_mask_id);
}

/* Chart-local pixel helpers (coordinates inside the battery zone) */
static lv_coord_t ch_x(int idx) { return g_ch_x0 + (lv_coord_t)((int32_t)idx * (g_ch_w - 1) / (CHART_PTS - 1)); }
static lv_coord_t ch_y(int v)   { return g_ch_y0 + (lv_coord_t)((int32_t)(100 - v) * (g_ch_h - 1) / 100); }

/* Time axis ticks: -H/2 -H/4 NU +H/4 +H/2, centered under the quarter
 * positions. Re-run whenever the Graph Window changes. */
static void update_time_axis(void)
{
    if (!g_tax[0]) return;
    int step_min = g_win_hours * 15;   /* one quarter of the window */
    for (int i = 0; i < 5; i++) {
        int off = (i - 2) * step_min;
        char b[12];
        if (off == 0)            snprintf(b, sizeof(b), "NU");
        else if (off % 60 == 0)  snprintf(b, sizeof(b), "%+dh", off / 60);
        else                     snprintf(b, sizeof(b), "%+dm", off);
        lv_label_set_text(g_tax[i], b);
        lv_obj_update_layout(g_tax[i]);
        lv_coord_t x = g_ch_x0 + (lv_coord_t)((int32_t)i * (g_ch_w - 1) / 4);
        lv_obj_set_pos(g_tax[i], x - lv_obj_get_width(g_tax[i]) / 2,
                       g_ch_y0 + g_ch_h + SY(6));
    }
}

/* Push a SoC sample into the history, redraw the series and reposition the
 * dashed projection overlay, the "now" dot and the target ring. The
 * projection is TIME-TRUE on the right half: it reaches 100/0 where the
 * remaining time (tmin) actually lands on the axis, then runs flat. */
static void update_chart(int soc, bool charging, bool discharging, int tmin)
{
    uint32_t now = lv_tick_get();
    if (!g_hist_init) {
        g_hist[NOW_IDX] = (int16_t)soc;
        g_hist_count = 1;
        g_hist_init = true;
        g_last_sample = now;
    } else if (now - g_last_sample >= g_sample_ms) {
        memmove(&g_hist[0], &g_hist[1], NOW_IDX * sizeof(g_hist[0]));
        g_hist[NOW_IDX] = (int16_t)soc;
        if (g_hist_count <= NOW_IDX) g_hist_count++;
        g_last_sample = now;
    } else {
        g_hist[NOW_IDX] = (int16_t)soc;   /* live head */
    }

    /* Real samples sit right-aligned against "now"; older slots stay NONE */
    int first = NOW_IDX + 1 - g_hist_count;
    for (int i = 0; i <= NOW_IDX; i++)
        lv_chart_set_value_by_id(g_chart, g_ser_hist, i,
                                 (i >= first) ? g_hist[i] : LV_CHART_POINT_NONE);
    lv_chart_refresh(g_chart);

    /* Dashed projection, time-true: slope from the remaining time. */
    int target = charging ? 100 : discharging ? 0 : soc;
    int half_min = g_win_hours * 30;                 /* right half in minutes */
    lv_coord_t nx = ch_x(NOW_IDX), rx = ch_x(CHART_PTS - 1);
    g_proj_pts[0].x = nx; g_proj_pts[0].y = ch_y(soc);
    int npts = 2;
    bool ring = false;

    if ((charging || discharging) && tmin > 0) {
        if (tmin <= half_min) {
            /* target reached inside the window: knee, then flat */
            lv_coord_t kx = nx + (lv_coord_t)((int32_t)(rx - nx) * tmin / half_min);
            g_proj_pts[1].x = kx; g_proj_pts[1].y = ch_y(target);
            g_proj_pts[2].x = rx; g_proj_pts[2].y = ch_y(target);
            npts = 3;
            ring = true;
            lv_obj_set_pos(g_tgt_ring, kx - SX(5), ch_y(target) - SX(5));
        } else {
            /* target beyond the window: clip the slope at the right edge */
            int edge_v = soc + (int)((int64_t)(target - soc) * half_min / tmin);
            g_proj_pts[1].x = rx; g_proj_pts[1].y = ch_y(edge_v);
        }
    } else {
        g_proj_pts[1].x = rx; g_proj_pts[1].y = ch_y(soc);   /* idle: flat */
    }
    lv_line_set_points(g_proj_line, g_proj_pts, npts);
    if (ring) lv_obj_clear_flag(g_tgt_ring, LV_OBJ_FLAG_HIDDEN);
    else      lv_obj_add_flag(g_tgt_ring, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_pos(g_now_dot, nx - SX(5), ch_y(soc) - SX(5));
}

/* "FULD KL. 18:01 · OM 3H 13M" — target clock only when the RTC is valid. */
static void update_until(bool charging, bool discharging, int tmin)
{
    char buf[80];
    if (tmin > 0 && (charging || discharging)) {
        const char *word = charging ? "FULD" : "TOM";
        int h = tmin / 60, mm = tmin % 60;
        if (g_clock_min >= 0) {
            int tgt = (g_clock_min + tmin) % (24 * 60);
            snprintf(buf, sizeof(buf), "#4E9EEB %s KL. %02d:%02d#  ·  OM %dH %02dM",
                     word, tgt / 60, tgt % 60, h, mm);
        } else {
            snprintf(buf, sizeof(buf), "#4E9EEB %s#  OM %dH %02dM", word, h, mm);
        }
    } else if (charging) {
        snprintf(buf, sizeof(buf), "#4E9EEB OPLADER#");
    } else if (discharging) {
        snprintf(buf, sizeof(buf), "AFLADER");
    } else {
        snprintf(buf, sizeof(buf), "STANDBY");
    }
    set_text_if_changed(g_until, buf);
    lv_obj_update_layout(g_until);
    lv_obj_align(g_until, LV_ALIGN_TOP_MID, 0, SY(352));
}

/*==========================================================================
 *  Public: update
 *========================================================================*/
void dashboard_ui_update(const dashboard_model_t *m)
{
    char buf[32];

    /* Device selector */
    if (m->dev_sel_visible) {
        lv_obj_clear_flag(g_dev_sel, LV_OBJ_FLAG_HIDDEN);
        if (m->dev_name) set_text_if_changed(g_dev_name, m->dev_name);
        lv_obj_set_style_bg_color(g_dev_dot, m->dev_online ? CB_BLUE : CB_DIM, 0);
    } else {
        lv_obj_add_flag(g_dev_sel, LV_OBJ_FLAG_HIDDEN);
    }

    /* SoC + prognosis (unit sign is its own dimmed label, re-anchored as
     * the number's width changes) */
    int soc = (int)(m->soc_percent + 0.5f);
    if (soc < 0) soc = 0;
    if (soc > 100) soc = 100;
    snprintf(buf, sizeof(buf), "%d", soc);
    set_text_if_changed(g_soc_pct, buf);
    lv_obj_update_layout(g_soc_pct);
    lv_obj_align(g_soc_pct, LV_ALIGN_TOP_MID, -SX(14), SY(34));
    lv_obj_align_to(g_soc_unit, g_soc_pct, LV_ALIGN_OUT_RIGHT_BOTTOM, SX(3), -SY(10));

    /* + = charging (current into the battery), - = discharging */
    bool charging    = m->battery_current_a > 0.5f;
    bool discharging = m->battery_current_a < -0.5f;
    int tmin = m->soc_time_min < 0 ? -m->soc_time_min : m->soc_time_min;
    update_chart(soc, charging, discharging, tmin);
    update_until(charging, discharging, tmin);

    /* Powers */
    int p_ac_in  = m->ac_input_power_w;
    int p_dc_in  = (int)(m->dc_input_voltage_v * m->dc_input_current_a + 0.5f);
    int p_solar  = (int)(m->solar_current_a * m->battery_voltage_v + 0.5f);
    int p_ac_out = m->ac_output_power_w;
    int p_dc_out = (int)(m->dc_output_voltage_v * m->dc_output_current_a + 0.5f);
    if (p_dc_in < 0) p_dc_in = 0;
    if (p_solar < 0) p_solar = 0;
    if (p_dc_out < 0) p_dc_out = 0;

    /* BMS units have no LPS functions: hide the whole charge cluster and
     * the inverter plate. */
    if (!g_layout_init || g_layout_bms != m->is_bms) {
        lv_obj_t *chg_objs[] = { g_chg_title, g_chg_total, g_chg_unit,
                                 g_cd_cell[0], g_cd_cell[1], g_cd_cell[2],
                                 g_cd_val[0], g_cd_val[1], g_cd_val[2],
                                 g_cd_unit[0], g_cd_unit[1], g_cd_unit[2] };
        for (unsigned i = 0; i < sizeof(chg_objs)/sizeof(chg_objs[0]); i++) {
            if (m->is_bms) lv_obj_add_flag(chg_objs[i], LV_OBJ_FLAG_HIDDEN);
            else           lv_obj_clear_flag(chg_objs[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (m->is_bms) {
            lv_obj_add_flag(g_dd_btn[0], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(g_dd_val[0], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(g_dd_unit[0], LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(g_dd_btn[1], LV_ALIGN_TOP_MID, 0, SY(48));
        } else {
            lv_obj_clear_flag(g_dd_btn[0], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(g_dd_val[0], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(g_dd_unit[0], LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(g_dd_btn[1], LV_ALIGN_TOP_MID, SX(93), SY(48));
        }
        place_value_pair(g_dd_val[1], g_dd_unit[1], g_dd_btn[1], SY(2));
        g_layout_init = true;
        g_layout_bms = m->is_bms;
    }

    if (!m->is_bms) {
        /* Charge dials */
        set_dial(g_cd_arc[0], g_cd_icon[0], g_cd_val[0], g_cd_unit[0],
                 g_cd_cell[0], SY(4), p_ac_in,
                 bar_cap(m->ac_in_max_w, CAP_AC_IN_W),
                 m->charger_state,  m->charger_failure);
        set_dial(g_cd_arc[1], g_cd_icon[1], g_cd_val[1], g_cd_unit[1],
                 g_cd_cell[1], SY(4), p_dc_in,
                 bar_cap(m->dc_in_max_w, CAP_DC_IN_W),
                 m->dc_input_state, m->dc_input_failure);
        set_dial(g_cd_arc[2], g_cd_icon[2], g_cd_val[2], g_cd_unit[2],
                 g_cd_cell[2], SY(4), p_solar,
                 bar_cap(m->solar_max_w, CAP_SOLAR_W),
                 m->solar_state,    m->solar_failure);

        int chg_total = p_ac_in + p_dc_in + p_solar;
        snprintf(buf, sizeof(buf), "%d", chg_total);
        set_text_if_changed(g_chg_total, buf);
        place_total(g_chg_total, g_chg_unit);
        set_chevrons(g_chg_chev, chg_total > 5);

        bool inv_ov = set_dial(g_dd_arc[0], g_dd_icon[0], g_dd_val[0], g_dd_unit[0],
                 g_dd_btn[0], SY(2), p_ac_out,
                 bar_cap(m->ac_out_max_w, CAP_AC_OUT_W),
                 m->inverter_state, m->inverter_failure);
        set_overload_pulse(0, inv_ov);
    } else {
        set_chevrons(g_chg_chev, false);
        set_text_if_changed(g_chg_total, "");
        set_overload_pulse(0, false);
    }

    bool dco_ov = set_dial(g_dd_arc[1], g_dd_icon[1], g_dd_val[1], g_dd_unit[1],
             g_dd_btn[1], SY(2), p_dc_out,
             bar_cap(m->dc_out_max_w, CAP_DC_OUT_W),
             m->dc_output_state, m->dc_output_failure);
    set_overload_pulse(1, dco_ov);

    int dis_total = (m->is_bms ? 0 : p_ac_out) + p_dc_out;
    snprintf(buf, sizeof(buf), "%d", dis_total);
    set_text_if_changed(g_dis_total, buf);
    place_total(g_dis_total, g_dis_unit);
    set_chevrons(g_dis_chev, dis_total > 5);

    /* Error badge: dim outline when clear; colored icon + count when active */
    if (m->error_count > 0) {
        lv_color_t c = m->error_critical ? CB_RED : CB_YELLOW;
        lv_obj_set_style_text_color(g_err_icon, c, 0);
        snprintf(buf, sizeof(buf), "%d", m->error_count);
        set_text_if_changed(g_err_cnt, buf);
        lv_obj_set_style_text_color(g_err_cnt, c, 0);
    } else {
        lv_obj_set_style_text_color(g_err_icon, CB_FAINT, 0);
        set_text_if_changed(g_err_cnt, "");
    }
}

#endif /* UI_SKIN_CARBON */
