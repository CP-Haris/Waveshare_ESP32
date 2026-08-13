/**
 * @file ui_screen.h
 * @brief Single source of truth for the panel resolution.
 *
 * Flip UI_PANEL_1024_600 below to switch the whole project between the two
 * Waveshare panels. Everything else follows automatically:
 *
 *   - lvgl_port.h      -> LVGL_PORT_H_RES / V_RES + ESP_PANEL_USE_1024_600_LCD
 *   - waveshare_*.c/h  -> pixel clock, porch timings, GT911 x_max/y_max
 *   - can_hmi.c        -> SCREEN_W / SCREEN_H, settings grid, row widths, splash
 *   - dashboard_ui.c   -> picks the full or the compact dashboard layout
 *
 * The PC simulator includes this header too (../main is on its include path)
 * and uses it as its default window size; it can be overridden at runtime with
 * `clayton_sim 800` / `clayton_sim 1024`.
 *
 * Pure macros - safe to include from any translation unit, firmware or PC.
 */

#ifndef UI_SCREEN_H
#define UI_SCREEN_H

/* ===== THE switch: 1 = 5" 1024x600, 0 = 5" 800x480 ===== */
#ifndef UI_PANEL_1024_600
#define UI_PANEL_1024_600   (1)
#endif

#if UI_PANEL_1024_600
#define SCREEN_W    1024
#define SCREEN_H    600
#else
#define SCREEN_W    800
#define SCREEN_H    480
#endif

/** 1 on the small panel - selects the compact UI variants. */
#define UI_COMPACT  (!UI_PANEL_1024_600)

#endif /* UI_SCREEN_H */
