#ifndef _RGB_LCD_H_
#define _RGB_LCD_H_

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch_gt911.h"
#include "lv_demos.h"
#include "lvgl_port.h"


#define I2C_MASTER_SCL_IO           9       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           8       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS       1000

#define GPIO_INPUT_IO_4    4            // CTP_IRQ — GT911 interrupt line
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_4)

/* ---------------------------------------------------------------------------
 * CH422G IO expander bit map (write to register 0x38)
 *
 *   IO0 (0x01) = DI0      — opto-isolated digital INPUT, 4.7K pull-up to 3V3
 *   IO1 (0x02) = CTP_RST  — GT911 reset, HIGH = released
 *   IO2 (0x04) = DISP     — backlight boost enable, HIGH = backlight on
 *   IO3 (0x08) = LCD_RST  — ST7262 reset, HIGH = released
 *   IO4 (0x10) = SDCS     — SD card chip select, HIGH = deselected
 *   IO5 (0x20) = DI1      — opto-isolated digital INPUT, 4.7K pull-up to 3V3
 *   IO6/IO7             — not connected
 *
 * The CH422G's output-enable is a single global bit, so IO0/IO5 are driven
 * even though the board wires them as inputs. Driving them LOW sinks
 * 3V3/(4.7K+100R) ~= 0.7 mA each, continuously. Driving them HIGH instead
 * matches the idle level of the pull-ups and costs nothing.
 *
 * Set BOARD_DI_INPUTS_UNUSED to 0 if the DI0/DI1 terminals are actually
 * wired: an active opto would then fight the expander output (limited to
 * ~33 mA by the 100R series resistors R19/R23).
 * ------------------------------------------------------------------------- */
#ifndef BOARD_DI_INPUTS_UNUSED
#define BOARD_DI_INPUTS_UNUSED  1
#endif

#if BOARD_DI_INPUTS_UNUSED
#define CH422G_DI_IDLE   (0x21)   // IO0 + IO5 held high — no pull-up sink
#else
#define CH422G_DI_IDLE   (0x00)
#endif

#define CH422G_CTP_RST   (0x02)
#define CH422G_DISP      (0x04)
#define CH422G_LCD_RST   (0x08)
#define CH422G_SDCS      (0x10)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_H_RES               (LVGL_PORT_H_RES)
#define EXAMPLE_LCD_V_RES               (LVGL_PORT_V_RES)

#if ESP_PANEL_USE_1024_600_LCD
    #define EXAMPLE_LCD_PIXEL_CLOCK_HZ      (21 * 1000 * 1000)
#else
    #define EXAMPLE_LCD_PIXEL_CLOCK_HZ      (16 * 1000 * 1000)
#endif

#define EXAMPLE_LCD_BIT_PER_PIXEL       (16)
#define EXAMPLE_RGB_BIT_PER_PIXEL       (16)
#define EXAMPLE_RGB_DATA_WIDTH          (16)
#define EXAMPLE_RGB_BOUNCE_BUFFER_SIZE  (EXAMPLE_LCD_H_RES * CONFIG_EXAMPLE_LCD_RGB_BOUNCE_BUFFER_HEIGHT)
#define EXAMPLE_LCD_IO_RGB_DISP         (-1)             // -1 if not used
#define EXAMPLE_LCD_IO_RGB_VSYNC        (GPIO_NUM_3)
#define EXAMPLE_LCD_IO_RGB_HSYNC        (GPIO_NUM_46)
#define EXAMPLE_LCD_IO_RGB_DE           (GPIO_NUM_5)
#define EXAMPLE_LCD_IO_RGB_PCLK         (GPIO_NUM_7)
#define EXAMPLE_LCD_IO_RGB_DATA0        (GPIO_NUM_14)
#define EXAMPLE_LCD_IO_RGB_DATA1        (GPIO_NUM_38)
#define EXAMPLE_LCD_IO_RGB_DATA2        (GPIO_NUM_18)
#define EXAMPLE_LCD_IO_RGB_DATA3        (GPIO_NUM_17)
#define EXAMPLE_LCD_IO_RGB_DATA4        (GPIO_NUM_10)
#define EXAMPLE_LCD_IO_RGB_DATA5        (GPIO_NUM_39)
#define EXAMPLE_LCD_IO_RGB_DATA6        (GPIO_NUM_0)
#define EXAMPLE_LCD_IO_RGB_DATA7        (GPIO_NUM_45)
#define EXAMPLE_LCD_IO_RGB_DATA8        (GPIO_NUM_48)
#define EXAMPLE_LCD_IO_RGB_DATA9        (GPIO_NUM_47)
#define EXAMPLE_LCD_IO_RGB_DATA10       (GPIO_NUM_21)
#define EXAMPLE_LCD_IO_RGB_DATA11       (GPIO_NUM_1)
#define EXAMPLE_LCD_IO_RGB_DATA12       (GPIO_NUM_2)
#define EXAMPLE_LCD_IO_RGB_DATA13       (GPIO_NUM_42)
#define EXAMPLE_LCD_IO_RGB_DATA14       (GPIO_NUM_41)
#define EXAMPLE_LCD_IO_RGB_DATA15       (GPIO_NUM_40)

#define EXAMPLE_LCD_IO_RST              (-1)             // -1 if not used
#define EXAMPLE_PIN_NUM_BK_LIGHT        (-1)    // -1 if not used
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL   (1)
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL  !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL

#define EXAMPLE_PIN_NUM_TOUCH_RST       (-1)            // -1 if not used
#define EXAMPLE_PIN_NUM_TOUCH_INT       (-1)            // -1 if not used

/* ---------------------------------------------------------------------------
 * RS485 (IO44 = RS485_TXD): DO NOT drive this pin high.
 *
 * U7 (SP3485EN) has DE and /RE tied together, pulled high by R66 (4.7K) and
 * pulled low by S1 (8050 NPN, emitter to GND). S1's base hangs on IO44 through
 * R68 = 100R only. Driving IO44 high therefore sinks (3V3 - Vbe)/100R, i.e.
 * ~26 mA of base current, continuously — measured as a ~5 mA increase on a
 * 12 V supply rail.
 *
 * Putting the transceiver in receive-only costs more than leaving its driver
 * enabled into an idle bus, so the cheapest state is to leave IO44 alone.
 * ------------------------------------------------------------------------- */

bool example_lvgl_lock(int timeout_ms);
void example_lvgl_unlock(void);

esp_err_t waveshare_esp32_s3_rgb_lcd_init();

esp_err_t wavesahre_rgb_lcd_bl_on();
esp_err_t wavesahre_rgb_lcd_bl_off();

/**
 * @brief Direct GT911 touch poll — reads touch count register via I2C.
 *        Works even when LVGL task is suspended.
 * @return true if a finger is currently touching the panel.
 */
bool waveshare_touch_is_pressed(void);

/**
 * @brief Delete the RGB panel for standby: stops the LCD DMA, frees the PSRAM
 *        framebuffers and — critically — releases the esp_lcd driver's
 *        NO_LIGHT_SLEEP PM lock so automatic light sleep can engage.
 */
esp_err_t waveshare_lcd_panel_sleep(void);

/**
 * @brief Recreate the RGB panel after standby and rebind LVGL to the new
 *        framebuffers (forces a full redraw).
 */
esp_err_t waveshare_lcd_panel_wake(void);

void waveshare_lcd_pins_float(void);
esp_err_t waveshare_lcd_reset_assert(void);
esp_err_t waveshare_lcd_reset_release(void);
esp_err_t waveshare_gt911_sleep(void);
esp_err_t waveshare_gt911_wake(void);
esp_err_t waveshare_ch422g_all_low(void);
esp_err_t waveshare_ch422g_sleep(void);
esp_err_t waveshare_ch422g_wake(void);

void example_lvgl_demo_ui();

#endif