/**
 * @file ui_palette.h
 * @brief Single source of truth for the Clayton Power HMI color palette.
 *
 * Shared by the firmware (can_hmi.c) and the PC simulator so colors can never
 * diverge between them. Luxury "midnight" dark theme with an app-blue accent
 * (#4b8eff) matching the ClaytonPowerApp mobile app.
 *
 * Pure macros over lv_color_hex() — include from any LVGL 8.4 translation unit.
 */

#ifndef UI_PALETTE_H
#define UI_PALETTE_H

#include "lvgl.h"

/* Surfaces — deeper near-black anthracite for maximum punch on the panel */
#define COL_BG_DARK     lv_color_hex(0x070810)
#define COL_BG_PANEL    lv_color_hex(0x10131F)
#define COL_BG_CARD     lv_color_hex(0x161A28)
#define COL_CARD_GRAD   lv_color_hex(0x10131F)
#define COL_CARD_BORDER lv_color_hex(0x39405A)
#define COL_SHADOW      lv_color_hex(0x000000)

/* Accent — brighter, more saturated app blue */
#define COL_ACCENT      lv_color_hex(0x5C9BFF)
#define COL_ACCENT_DEEP lv_color_hex(0x1E63DA)
#define COL_ACCENT_GLOW lv_color_hex(0x3A82F0)

/* Status — brighter, more saturated */
#define COL_GREEN       lv_color_hex(0x46E07A)
#define COL_ORANGE      lv_color_hex(0xFFA526)
#define COL_RED         lv_color_hex(0xFF4D52)
#define COL_SOLAR       lv_color_hex(0xFFC61A)

/* Text — pure white hero, brighter secondary/tertiary for readability */
#define COL_TEXT        lv_color_hex(0xFFFFFF)
#define COL_TEXT_DIM    lv_color_hex(0xA8B0C0)
#define COL_TEXT_FAINT  lv_color_hex(0x767E92)

/* SOC arc — bright accent over a darker, higher-contrast track */
#define COL_SOC_ARC     lv_color_hex(0x5C9BFF)
#define COL_SOC_BG      lv_color_hex(0x232838)

#endif /* UI_PALETTE_H */
