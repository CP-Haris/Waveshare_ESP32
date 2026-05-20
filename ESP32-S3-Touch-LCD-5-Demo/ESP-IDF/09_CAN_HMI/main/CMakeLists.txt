idf_component_register(
    SRCS "waveshare_rgb_lcd_port.c" "main.c" "lvgl_port.c" "can_hmi.c" "ble_gateway.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES bt nvs_flash)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
