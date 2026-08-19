/**
 * main.c — Entry point for CAN HMI on ESP32-S3-Touch-LCD-5
 *
 * Initializes the RGB LCD, touch, LVGL, TWAI CAN bus, and launches the HMI task.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define CONFIG_TWAI_SUPPRESS_DEPRECATE_WARN 1
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"

#include "waveshare_rgb_lcd_port.h"
#include "lvgl_port.h"
#include "can_hmi.h"
#include "usb_modem.h"

static const char *TAG = "main";

void app_main(void)
{
    if (esp_sleep_get_wakeup_causes() & BIT(ESP_SLEEP_WAKEUP_EXT0))
        ESP_LOGI(TAG, "=== Wake from deep sleep (CAN activity) ===");
    else
        ESP_LOGI(TAG, "=== Clayton Power CAN HMI (cold boot) ===");
    // 1=POWERON 3=SW 4=PANIC 5=INT_WDT 6=TASK_WDT 7=OTHER_WDT 9=BROWNOUT
    ESP_LOGW(TAG, "Reset reason: %d", (int)esp_reset_reason());
    ESP_LOGI(TAG, "Display: ESP32-S3-Touch-LCD-5 (%dx%d)", SCREEN_W, SCREEN_H);

    // Initialize display, touch, and LVGL
    waveshare_esp32_s3_rgb_lcd_init();
    wavesahre_rgb_lcd_bl_on();

    // USB CAN modem (TinyUSB takes over the USB-C port: CDC0 = modem
    // protocol, CDC1 = console). Flashing now needs download mode — use
    // flash.ps1 (sends the 'BT' command) or hold BOOT during reset.
    usb_modem_init();

    // Crash diagnostics (reset reason + wake-step breadcrumb) are captured
    // and logged by can_hmi_task's heartbeat — a one-shot log here would be
    // lost when the USB host attaches and the CDC buffer is cleared.

    // NOTE: IO44 (RS485_TXD) is deliberately left untouched — see the warning
    // in waveshare_rgb_lcd_port.h before adding any code that drives it.

    // Bring up TWAI (CAN bus) at 125 Kbps. can_hmi owns the driver lifecycle:
    // standby uninstalls it so the power manager can enter light sleep.
    if (can_hmi_bus_start() == ESP_OK) {
        ESP_LOGI(TAG, "TWAI started at 125 Kbps (TX=%d, RX=%d)",
                 CONFIG_EXAMPLE_TX_GPIO_NUM, CONFIG_EXAMPLE_RX_GPIO_NUM);
    }

    // Create HMI UI (must be done under LVGL lock)
    if (lvgl_port_lock(1000)) {
        can_hmi_init();
        lvgl_port_unlock();
    }

    // Launch CAN HMI task. 16 KB stack: the standby wake path runs the whole
    // TinyUSB (re)install inside this task's deepest call chain.
    xTaskCreatePinnedToCore(can_hmi_task, "can_hmi", 16384, NULL, 5, NULL, 1);
    ESP_LOGI(TAG, "CAN HMI task launched");
}
