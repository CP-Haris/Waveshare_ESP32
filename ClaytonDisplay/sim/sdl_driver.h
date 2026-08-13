/**
 * @file sdl_driver.h
 * @brief Minimal SDL2 display + pointer driver for the LVGL 8.4 PC simulator.
 *
 * Pinned to the project's own LVGL 8.4 source (no lv_drivers dependency, no
 * version mismatch). Renders a 16-bit (RGB565) framebuffer into an SDL window
 * and feeds mouse input to an LVGL pointer indev.
 */

#ifndef SDL_DRIVER_H
#define SDL_DRIVER_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Create the SDL window + LVGL display/input devices.
 *  @param hor_res  horizontal resolution (e.g. 1024)
 *  @param ver_res  vertical resolution   (e.g. 600)
 *  @param title    window title
 *  @return false on SDL failure
 */
bool sdl_sim_init(int hor_res, int ver_res, const char *title);

/** Pump SDL events into LVGL. Call once per main-loop iteration.
 *  @return false when the user closed the window (quit requested).
 */
bool sdl_sim_pump(void);

/** Release SDL resources. */
void sdl_sim_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* SDL_DRIVER_H */
