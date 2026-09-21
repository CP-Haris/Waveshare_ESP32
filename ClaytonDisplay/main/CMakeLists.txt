idf_component_register(
    SRCS "waveshare_rgb_lcd_port.c" "main.c" "lvgl_port.c" "can_hmi.c" "ble_gateway.c" "dashboard_ui.c" "dashboard_carbon.c" "icons_carbon.c" "usb_modem.c" "rtc_pcf85063.c"
         "fonts/cb_font_84.c" "fonts/cb_font_42.c" "fonts/cb_font_22.c" "fonts/cb_font_16.c" "fonts/cb_font_14.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES bt nvs_flash)

# The lv_font_conv output includes "lvgl.h" only when this is defined.
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
