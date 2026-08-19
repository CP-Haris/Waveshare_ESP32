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

#ifdef __cplusplus
}
#endif
