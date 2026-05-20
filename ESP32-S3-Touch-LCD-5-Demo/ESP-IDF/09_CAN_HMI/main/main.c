/**
 * main.c — Entry point for CAN HMI on ESP32-S3-Touch-LCD-5
 *
 * Initializes the RGB LCD, touch, LVGL, TWAI CAN bus, and launches the HMI task.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define CONFIG_TWAI_SUPPRESS_DEPRECATE_WARN 1
#include "driver/twai.h"
#include "esp_log.h"

#include "esp_sleep.h"

#include "waveshare_rgb_lcd_port.h"
#include "lvgl_port.h"
#include "can_hmi.h"

static const char *TAG = "main";

void app_main(void)
{
    esp_sleep_wakeup_cause_t wake = esp_sleep_get_wakeup_cause();
    if (wake == ESP_SLEEP_WAKEUP_EXT0)
        ESP_LOGI(TAG, "=== Wake from deep sleep (CAN activity) ===");
    else
        ESP_LOGI(TAG, "=== Clayton Power CAN HMI (cold boot) ===");
    ESP_LOGI(TAG, "Display: ESP32-S3-Touch-LCD-5 (1024x600)");

    // Initialize display, touch, and LVGL
    waveshare_esp32_s3_rgb_lcd_init();
    wavesahre_rgb_lcd_bl_on();

    // Initialize TWAI (CAN bus) at 125 Kbps
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        CONFIG_EXAMPLE_TX_GPIO_NUM, CONFIG_EXAMPLE_RX_GPIO_NUM, TWAI_MODE_NORMAL);
    g_config.alerts_enabled = TWAI_ALERT_BUS_OFF | TWAI_ALERT_BUS_RECOVERED |
                              TWAI_ALERT_ERR_PASS | TWAI_ALERT_ABOVE_ERR_WARN;
    g_config.tx_queue_len = 64;
    g_config.rx_queue_len = 32;

    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "TWAI driver install failed: %s", esp_err_to_name(err));
    } else {
        err = twai_start();
        if (err != ESP_OK)
            ESP_LOGE(TAG, "TWAI start failed: %s", esp_err_to_name(err));
        else
            ESP_LOGI(TAG, "TWAI started at 125 Kbps (TX=%d, RX=%d)",
                     CONFIG_EXAMPLE_TX_GPIO_NUM, CONFIG_EXAMPLE_RX_GPIO_NUM);
    }

    // Create HMI UI (must be done under LVGL lock)
    if (lvgl_port_lock(1000)) {
        can_hmi_init();
        lvgl_port_unlock();
    }

    // Launch CAN HMI task
    xTaskCreatePinnedToCore(can_hmi_task, "can_hmi", 8192, NULL, 5, NULL, 1);
    ESP_LOGI(TAG, "CAN HMI task launched");
}
