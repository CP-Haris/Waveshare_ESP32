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
#define UI_PANEL_1024_600   (0)
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

/* ===== Board switch: 0 = Waveshare ESP32-S3-Touch-LCD-5 (CH422G expander),
 *                     1 = Clayton "New Display" PCB (PCA9554 expander, CAN_S,
 *                         buzzer via RTC CLKOUT, single-wire on UART0).
 * The ESP32 pin map (RGB bus, sync, I2C, touch IRQ, SPI, CAN, USB) is
 * IDENTICAL on both boards — only the IO-expander world differs.
 * See docs/"New Display - Netlist" and memory note new-display-pcb. */
#ifndef BOARD_CP_DISPLAY
#define BOARD_CP_DISPLAY  (1)
#endif

/* ===== Skin switch: 0 = classic "Midnight" dashboard (dashboard_ui.c),
 *                    1 = "Carbon Blue" instrument skin (dashboard_carbon.c).
 * Both files implement the same dashboard_ui.h API; exactly one is compiled
 * in (the other compiles to an empty translation unit), so callers and the
 * PC simulator need no changes when switching. */
#ifndef UI_SKIN_CARBON
#define UI_SKIN_CARBON  (1)
#endif

#endif /* UI_SCREEN_H */
