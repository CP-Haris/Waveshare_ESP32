/**
 * PCF85063A RTC driver — see rtc_pcf85063.h.
 *
 * Register map (PCF85063A datasheet §7):
 *   0x00 Control_1   0x04 Seconds (bit7 = OS, clock integrity lost)
 *   0x01 Control_2   0x05 Minutes   0x08 Weekdays
 *   0x02 Offset      0x06 Hours     0x09 Months
 *   0x03 RAM byte    0x07 Days      0x0A Years (00-99 -> 2000-2099)
 * Time/date registers are BCD.
 */

#include "rtc_pcf85063.h"

#include <string.h>
#include <sys/time.h>
#include "esp_log.h"

static const char *TAG = "rtc";

#define PCF85063_ADDR      0x51
#define REG_CONTROL_1      0x00
#define REG_SECONDS        0x04
#define SECONDS_OS_BIT     0x80
#define I2C_TIMEOUT_MS     100

static i2c_master_dev_handle_t s_dev = NULL;

static uint8_t bcd_to_dec(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 0x0F)); }
static uint8_t dec_to_bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

esp_err_t rtc_pcf85063_init(i2c_master_bus_handle_t bus)
{
    if (s_dev) return ESP_OK;

    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = PCF85063_ADDR,
        .scl_speed_hz    = 400000,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &cfg, &s_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "add_device failed: %s", esp_err_to_name(err));
        return err;
    }

    // Normal mode, 24h format, oscillator running
    uint8_t ctrl[2] = { REG_CONTROL_1, 0x00 };
    err = i2c_master_transmit(s_dev, ctrl, sizeof(ctrl), I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "PCF85063 not responding: %s", esp_err_to_name(err));
    }
    return err;
}

bool rtc_pcf85063_get_time(struct tm *out)
{
    if (!s_dev || !out) return false;

    uint8_t reg = REG_SECONDS;
    uint8_t b[7];
    if (i2c_master_transmit_receive(s_dev, &reg, 1, b, sizeof(b),
                                    I2C_TIMEOUT_MS) != ESP_OK) {
        return false;
    }
    if (b[0] & SECONDS_OS_BIT) return false;   // clock integrity lost

    memset(out, 0, sizeof(*out));
    out->tm_sec  = bcd_to_dec(b[0] & 0x7F);
    out->tm_min  = bcd_to_dec(b[1] & 0x7F);
    out->tm_hour = bcd_to_dec(b[2] & 0x3F);
    out->tm_mday = bcd_to_dec(b[3] & 0x3F);
    out->tm_wday = b[4] & 0x07;
    out->tm_mon  = bcd_to_dec(b[5] & 0x1F) - 1;
    out->tm_year = bcd_to_dec(b[6]) + 100;     // years since 1900, RTC = 2000+
    out->tm_isdst = -1;
    return true;
}

esp_err_t rtc_pcf85063_set_time(const struct tm *t)
{
    if (!s_dev || !t) return ESP_ERR_INVALID_STATE;

    uint8_t buf[8] = {
        REG_SECONDS,
        dec_to_bcd((uint8_t)t->tm_sec),        // write clears the OS bit
        dec_to_bcd((uint8_t)t->tm_min),
        dec_to_bcd((uint8_t)t->tm_hour),
        dec_to_bcd((uint8_t)t->tm_mday),
        (uint8_t)(t->tm_wday & 0x07),
        dec_to_bcd((uint8_t)(t->tm_mon + 1)),
        dec_to_bcd((uint8_t)(t->tm_year - 100)),
    };
    return i2c_master_transmit(s_dev, buf, sizeof(buf), I2C_TIMEOUT_MS);
}

esp_err_t rtc_pcf85063_set_datetime(int year, int month, int day,
                                    int hour, int min, int sec)
{
    if (year < 2000 || year > 2099 || month < 1 || month > 12 ||
        day < 1 || day > 31 || hour > 23 || min > 59 || sec > 59) {
        return ESP_ERR_INVALID_ARG;
    }

    struct tm t = {
        .tm_year = year - 1900,
        .tm_mon  = month - 1,
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min  = min,
        .tm_sec  = sec,
        .tm_isdst = -1,
    };
    time_t unix_time = mktime(&t);             // also fills tm_wday
    esp_err_t err = rtc_pcf85063_set_time(&t);
    if (err == ESP_OK) {
        struct timeval tv = { .tv_sec = unix_time };
        settimeofday(&tv, NULL);
        ESP_LOGI(TAG, "Time set: %04d-%02d-%02d %02d:%02d:%02d",
                 year, month, day, hour, min, sec);
    }
    return err;
}
