# Clayton Power HMI - LVGL 8.4 PC Simulator (SDL2)
#
# Reuses the project's own LVGL 8.4 source (managed_components) and the real
# ui_theme.c from ../main, so what you see here matches the ESP32-S3 1:1.
#
# Build (from this folder), using the MSYS2 ucrt64 toolchain:
#   cmake -G Ninja -B build .
#   cmake --build build
#   ./build/clayton_sim

cmake_minimum_required(VERSION 3.16)
project(clayton_sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Paths -----------------------------------------------------------------
set(REPO_MAIN  "${CMAKE_CURRENT_SOURCE_DIR}/../main")
set(LVGL_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl")

# --- LVGL 8.4 sources ------------------------------------------------------
file(GLOB_RECURSE LVGL_SOURCES "${LVGL_DIR}/src/*.c")

# --- Simulator + reused project sources ------------------------------------
set(SIM_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdl_driver.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui_demo.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui_doc.c"
    "${REPO_MAIN}/dashboard_ui.c"      # the REAL dashboard, shared with firmware
    "${REPO_MAIN}/dashboard_carbon.c"  # Carbon Blue skin (UI_SKIN_CARBON in ui_screen.h)
    "${REPO_MAIN}/icons_carbon.c"      # Carbon pictograms (generated)
    "${REPO_MAIN}/fonts/cb_font_84.c"
    "${REPO_MAIN}/fonts/cb_font_42.c"
    "${REPO_MAIN}/fonts/cb_font_22.c"
    "${REPO_MAIN}/fonts/cb_font_16.c"
    "${REPO_MAIN}/fonts/cb_font_14.c"
)

add_executable(clayton_sim ${SIM_SOURCES} ${LVGL_SOURCES})

# lv_conf.h lives in this folder; LV_CONF_INCLUDE_SIMPLE makes LVGL find it.
# LV_LVGL_H_INCLUDE_SIMPLE makes the lv_font_conv output include "lvgl.h".
target_compile_definitions(clayton_sim PRIVATE LV_CONF_INCLUDE_SIMPLE LV_LVGL_H_INCLUDE_SIMPLE)

target_include_directories(clayton_sim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"      # lv_conf.h, sdl_driver.h
    "${REPO_MAIN}"                     # ui_theme.h
    "${LVGL_DIR}"                      # lvgl.h
)

# --- SDL2 ------------------------------------------------------------------
find_package(SDL2 REQUIRED)
target_include_directories(clayton_sim PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(clayton_sim PRIVATE ${SDL2_LIBRARIES})

if(WIN32)
    # Keep a console for logs; switch to -mwindows for a windowed-only build.
    target_link_libraries(clayton_sim PRIVATE mingw32)
endif()
