/**
 * @file main.c
 * @brief PC simulator entry point for the Clayton Power HMI luxury theme.
 *
 * Initialises LVGL 8.4 + the SDL2 driver, builds the demo dashboard, and runs
 * the main loop.
 *
 * The window defaults to whatever panel ui_screen.h selects for the firmware,
 * so the simulator always matches the build you are about to flash. Override it
 * to preview the other panel without recompiling:
 *
 *     ./clayton_sim          # follow ui_screen.h (firmware setting)
 *     ./clayton_sim 800      # force 800x480  (5" panel)
 *     ./clayton_sim 1024     # force 1024x600 (7" panel)
 *
 * A second argument selects a static documentation screen instead of the
 * animated demo (see ui_doc.c for the list):
 *
 *     ./clayton_sim 1024 settings
 *     ./clayton_sim 1024 popup-error
 */

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "sdl_driver.h"
#include "ui_screen.h"

void ui_demo_create(void);              /* from ui_demo.c */
bool ui_doc_create(const char *screen); /* from ui_doc.c  */

int main(int argc, char *argv[])
{
    int hor_res = SCREEN_W;
    int ver_res = SCREEN_H;

    if (argc > 1) {
        if (strcmp(argv[1], "800") == 0)       { hor_res = 800;  ver_res = 480; }
        else if (strcmp(argv[1], "1024") == 0) { hor_res = 1024; ver_res = 600; }
        else {
            SDL_Log("Usage: %s [800|1024]", argv[0]);
            return 1;
        }
    }

    lv_init();

    char title[64];
    snprintf(title, sizeof(title), "Clayton Power HMI - Simulator (%dx%d)",
             hor_res, ver_res);

    if (!sdl_sim_init(hor_res, ver_res, title)) {
        return 1;
    }

    if (argc > 2) {
        if (!ui_doc_create(argv[2])) {
            SDL_Log("Unknown doc screen '%s' (see ui_doc.c)", argv[2]);
            return 1;
        }
    } else {
        ui_demo_create();
    }

    /* Main loop: pump input, run LVGL timers. */
    while (sdl_sim_pump()) {
        lv_timer_handler();
        SDL_Delay(5);
    }

    sdl_sim_deinit();
    return 0;
}
