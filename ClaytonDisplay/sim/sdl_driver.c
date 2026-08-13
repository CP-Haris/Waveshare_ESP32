/**
 * @file sdl_driver.c
 * @brief Minimal SDL2 display + pointer driver implementation (LVGL 8.4).
 */

#include "sdl_driver.h"
#include <SDL2/SDL.h>
#include <stdlib.h>

/* ---- SDL objects ---------------------------------------------------------*/
static SDL_Window   *s_window  = NULL;
static SDL_Renderer *s_renderer = NULL;
static SDL_Texture  *s_texture  = NULL;
static int           s_hor = 0;
static int           s_ver = 0;

/* ---- LVGL draw buffers ---------------------------------------------------*/
static lv_disp_draw_buf_t s_draw_buf;
static lv_color_t        *s_buf1 = NULL;   /* partial render buffer (1/10 screen) */

/* ---- Pointer (mouse) state ----------------------------------------------*/
static int  s_mouse_x = 0;
static int  s_mouse_y = 0;
static bool s_mouse_pressed = false;
static bool s_quit = false;

/* Flush a rendered area to the SDL texture and present it. */
static void sdl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    const int w = lv_area_get_width(area);
    const int h = lv_area_get_height(area);

    SDL_Rect rect = { area->x1, area->y1, w, h };
    /* lv_color_t is 16-bit here (RGB565); pitch = width * 2 bytes. */
    SDL_UpdateTexture(s_texture, &rect, color_p, w * (int)sizeof(lv_color_t));

    /* Present the whole texture; untouched regions keep their previous pixels. */
    SDL_RenderClear(s_renderer);
    SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
    SDL_RenderPresent(s_renderer);

    lv_disp_flush_ready(drv);
}

/* Report the latest mouse position/state to LVGL. */
static void sdl_pointer_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    data->point.x = s_mouse_x;
    data->point.y = s_mouse_y;
    data->state   = s_mouse_pressed ? LV_INDEV_STATE_PRESSED
                                    : LV_INDEV_STATE_RELEASED;
}

bool sdl_sim_init(int hor_res, int ver_res, const char *title)
{
    s_hor = hor_res;
    s_ver = ver_res;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    s_window = SDL_CreateWindow(title,
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                hor_res, ver_res, SDL_WINDOW_SHOWN);
    if (!s_window) { SDL_Log("CreateWindow failed: %s", SDL_GetError()); return false; }

    s_renderer = SDL_CreateRenderer(s_window, -1,
                                    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!s_renderer) { SDL_Log("CreateRenderer failed: %s", SDL_GetError()); return false; }

    s_texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_RGB565,
                                  SDL_TEXTUREACCESS_STREAMING, hor_res, ver_res);
    if (!s_texture) { SDL_Log("CreateTexture failed: %s", SDL_GetError()); return false; }

    /* Partial buffer: 1/10 of the screen is plenty and keeps RAM low. */
    const size_t buf_px = (size_t)hor_res * ver_res / 10;
    s_buf1 = (lv_color_t *)malloc(buf_px * sizeof(lv_color_t));
    if (!s_buf1) { SDL_Log("draw buffer alloc failed"); return false; }

    lv_disp_draw_buf_init(&s_draw_buf, s_buf1, NULL, (uint32_t)buf_px);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &s_draw_buf;
    disp_drv.flush_cb = sdl_flush_cb;
    disp_drv.hor_res  = hor_res;
    disp_drv.ver_res  = ver_res;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_pointer_read_cb;
    lv_indev_drv_register(&indev_drv);

    return true;
}

bool sdl_sim_pump(void)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                s_quit = true;
                break;
            case SDL_MOUSEMOTION:
                s_mouse_x = e.motion.x;
                s_mouse_y = e.motion.y;
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    s_mouse_pressed = true;
                    s_mouse_x = e.button.x;
                    s_mouse_y = e.button.y;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    s_mouse_pressed = false;
                }
                break;
            default:
                break;
        }
    }
    return !s_quit;
}

void sdl_sim_deinit(void)
{
    if (s_texture)  SDL_DestroyTexture(s_texture);
    if (s_renderer) SDL_DestroyRenderer(s_renderer);
    if (s_window)   SDL_DestroyWindow(s_window);
    free(s_buf1);
    SDL_Quit();
}
