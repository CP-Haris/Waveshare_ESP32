/**
 * PCF85063A real-time clock — onboard the Waveshare ESP32-S3-Touch-LCD-5,
 * on the shared I2C bus (address 0x51), battery-backed via the RTC header.
 *
 * The system clock is synced from the RTC once at boot (main.c); afterwards
 * the firmware reads normal system time. Setting the time writes both.
 */

#pragma once

#include <time.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Attach to the (already created) I2C bus. Call once from main. */
esp_err_t rtc_pcf85063_init(i2c_master_bus_handle_t bus);

/**
 * Read the RTC. Returns false if the chip is unreachable or reports lost
 * clock integrity (first power-up / battery removed) — the time is then
 * not to be trusted.
 */
bool rtc_pcf85063_get_time(struct tm *out);

/** Write the RTC (also clears the lost-integrity flag). */
esp_err_t rtc_pcf85063_set_time(const struct tm *t);

/**
 * Set RTC AND system clock from broken-down local time.
 * Convenience used by the debug-port TS command.
 */
esp_err_t rtc_pcf85063_set_datetime(int year, int month, int day,
                                    int hour, int min, int sec);

/**
 * CLKOUT pin control. On the Clayton "New Display" PCB the RTC's CLKOUT
 * drives the buzzer, so the tone is made by enabling a 4096 Hz square wave
 * and gating it with the expander's BUZZ_ON bit. Off = COF disabled (also
 * the recommended idle state — the power-on default is 32768 Hz).
 */
esp_err_t rtc_pcf85063_clkout_enable(bool on);

/**
 * CLKOUT at a specific frequency: 1024, 2048, 4096, 8192, 16384 or 32768 Hz;
 * 0 = off. Other values return ESP_ERR_INVALID_ARG.
 */
esp_err_t rtc_pcf85063_clkout_set_hz(uint32_t hz);

#ifdef __cplusplus
}
#endif
