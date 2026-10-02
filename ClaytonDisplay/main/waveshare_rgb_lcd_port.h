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
#include "lvgl_port.h"
#include "ui_screen.h"   /* BOARD_CP_DISPLAY board switch */


#define I2C_MASTER_SCL_IO           9       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           8       /*!< GPIO number used for I2C master data  */
/* 100 kHz on purpose: the bus spans the touch FPC, and at 400 kHz the CP
 * board showed phantom ACKs in bus scans. GT911 driver runs 100 kHz anyway. */
#define I2C_MASTER_FREQ_HZ          100000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS       1000

#define GPIO_INPUT_IO_4    4            // CTP_IRQ — GT911 interrupt line
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_4)

/* ---------------------------------------------------------------------------
 * IO expander — board dependent (BOARD_CP_DISPLAY in ui_screen.h):
 *
 *  Waveshare board: CH422G (mode write to 0x24, output byte to 0x38)
 *   IO0 (0x01) = DI0      — opto-isolated digital INPUT, 4.7K pull-up to 3V3
 *   IO1 (0x02) = CTP_RST  — GT911 reset, HIGH = released
 *   IO2 (0x04) = DISP     — backlight boost enable, HIGH = backlight on
 *   IO3 (0x08) = LCD_RST  — ST7262 reset, HIGH = released
 *   IO4 (0x10) = SDCS     — SD card chip select, HIGH = deselected
 *   IO5 (0x20) = DI1      — opto-isolated digital INPUT, 4.7K pull-up to 3V3
 *
 *  Clayton "New Display" PCB: PCA9554 @ 0x20 (reg 0x03 = config, 0x01 = out)
 *   P0 (0x01) = NVM_RST   — design leftover, not connected; keep LOW
 *   P1 (0x02) = TP_RST    — GT911 reset, HIGH = released
 *   P2 (0x04) = DISP_ON   — panel enable (display FPC pin 31)
 *   P3 (0x08) = LCD_RST   — panel reset (display FPC pin 35), HIGH = released
 *   P4 (0x10) = SDCS      — SD card chip select, HIGH = deselected
 *   P5 (0x20) = BUZZ_ON   — buzzer gate (tone = RTC CLKOUT via opamp)
 *   P6 (0x40) = BL_ON     — backlight boost (AP3032 CTRL), HIGH = on
 *   P7 (0x80) = CAN_S     — CAN transceiver standby; pulled HIGH by R1, so
 *                           the transceiver is in STANDBY until firmware
 *                           drives this LOW (waveshare_can_transceiver_enable)
 *
 * Bits P1..P4 happen to match the CH422G positions, so the shared control
 * bits are board-independent; only the access protocol and the extra
 * function bits differ.
 * ------------------------------------------------------------------------- */
#if BOARD_CP_DISPLAY

#define EXP_PCA9554_ADDR (0x20)
#define EXP_NVM_RST      (0x01)
#define EXP_TP_RST       (0x02)
#define EXP_DISP         (0x04)
#define EXP_LCD_RST      (0x08)
#define EXP_SDCS         (0x10)
#define EXP_BUZZ_ON      (0x20)
#define EXP_BL_ON        (0x40)
#define EXP_CAN_S        (0x80)
#define EXP_IDLE_BITS    (0x00)

#else /* Waveshare / CH422G */

#ifndef BOARD_DI_INPUTS_UNUSED
#define BOARD_DI_INPUTS_UNUSED  1
#endif
/* The CH422G's output-enable is a single global bit, so IO0/IO5 are driven
 * even though the board wires them as inputs. Driving them HIGH matches the
 * idle level of the pull-ups and costs nothing (LOW sinks ~0.7 mA each). */
#if BOARD_DI_INPUTS_UNUSED
#define CH422G_DI_IDLE   (0x21)   // IO0 + IO5 held high — no pull-up sink
#else
#define CH422G_DI_IDLE   (0x00)
#endif

#define CH422G_CTP_RST   (0x02)
#define CH422G_DISP      (0x04)
#define CH422G_LCD_RST   (0x08)
#define CH422G_SDCS      (0x10)

#define EXP_TP_RST       CH422G_CTP_RST
#define EXP_DISP         CH422G_DISP
#define EXP_LCD_RST      CH422G_LCD_RST
#define EXP_SDCS         CH422G_SDCS
#define EXP_IDLE_BITS    CH422G_DI_IDLE

#endif /* BOARD_CP_DISPLAY */
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
 * IO43/IO44 (UART0 TXD0/RXD0) — leave alone on BOTH boards:
 *
 * CP "New Display" board: TXD0/RXD0 drive the single-wire bus (DATA_SW on
 * J1 pin 1) through Q3/Q2. The console is on USB-Serial-JTAG, so nothing
 * touches UART0 in normal operation; do not enable a UART0 driver here
 * until the single-wire protocol is actually implemented.
 *
 * Waveshare board — RS485 (IO44 = RS485_TXD): DO NOT drive this pin high.
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

esp_err_t waveshare_esp32_s3_rgb_lcd_init();

/**
 * @brief The shared I2C bus (touch, CH422G, PCF85063 RTC). Valid after
 *        waveshare_esp32_s3_rgb_lcd_init().
 */
i2c_master_bus_handle_t waveshare_i2c_bus(void);

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
esp_err_t waveshare_ch422g_all_low(void);
esp_err_t waveshare_ch422g_wake(void);

/**
 * @brief CAN transceiver normal/standby. On the CP board the transceiver's
 *        S pin is pulled HIGH (standby) at power-on and must be driven low
 *        via the expander before any CAN traffic works. No-op on Waveshare.
 */
esp_err_t waveshare_can_transceiver_enable(bool enable);

/**
 * @brief Short UI click on the buzzer (CP board: RTC CLKOUT 4096 Hz gated by
 *        the expander's BUZZ_ON bit for ~40 ms). No-op on Waveshare.
 *        Non-blocking — the tone is stopped by an esp_timer callback.
 */
esp_err_t waveshare_buzzer_click(void);

typedef enum {
    BUZZ_WARNING,    /* two mid beeps (2048 Hz)                 */
    BUZZ_FAILURE,    /* falling 4096 -> 2048 -> 1024 Hz         */
    BUZZ_CRITICAL,   /* siren, 4096/1024 Hz alternating x3      */
} buzz_alarm_t;

/**
 * @brief Play an alarm melody (CP board; no-op on Waveshare). Non-blocking;
 *        restarts any running sequence. UI clicks are ignored while an
 *        alarm plays.
 */
esp_err_t waveshare_buzzer_alarm(buzz_alarm_t level);

/**
 * @brief Bring-up diagnostics on demand (debug-port command "SC"):
 *        scans the I2C bus and dumps the IO-expander registers/shadow.
 */
void waveshare_i2c_diag(void);

#endif