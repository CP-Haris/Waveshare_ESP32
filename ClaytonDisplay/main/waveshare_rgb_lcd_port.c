/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "waveshare_rgb_lcd_port.h"
#include "soc/gpio_reg.h"

static const char *TAG = "waveshare_lcd";

// VSYNC event callback function
IRAM_ATTR static bool rgb_lcd_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    return lvgl_port_notify_rgb_vsync();
}

#if CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911

static i2c_master_bus_handle_t i2c_bus = NULL;

/**
 * @brief I2C master initialization (new driver API for ESP-IDF v6.0)
 */
static esp_err_t i2c_master_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    return i2c_new_master_bus(&bus_config, &i2c_bus);
}

/**
 * @brief Helper to write bytes via I2C to a device (replaces legacy i2c_master_write_to_device)
 */
static esp_err_t i2c_write_bytes(uint8_t dev_addr, const uint8_t *data, size_t len)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    i2c_master_dev_handle_t dev;
    esp_err_t ret = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &dev);
    if (ret != ESP_OK) return ret;
    ret = i2c_master_transmit(dev, data, len, I2C_MASTER_TIMEOUT_MS);
    i2c_master_bus_rm_device(dev);
    return ret;
}

static esp_err_t i2c_write_byte(uint8_t dev_addr, uint8_t data)
{
    return i2c_write_bytes(dev_addr, &data, 1);
}

/* ---------------------------------------------------------------------------
 * IO-expander access layer. All expander outputs go through a shadow byte so
 * every write carries the full, current pin state — the two chips just take
 * different protocols:
 *   CH422G (Waveshare):  mode byte to addr 0x24, output byte to addr 0x38
 *   PCA9554 (CP board):  reg 0x03 = direction (0 = output), reg 0x01 = output
 * ------------------------------------------------------------------------- */
#if BOARD_CP_DISPLAY
/* Boot state: panel enabled + resets released + SD deselected; backlight and
 * buzzer off; NVM_RST (design leftover) low; CAN_S high = transceiver stays
 * in standby until can_hmi enables it. TP_RST starts low — the GT911 reset
 * sequence releases it. */
static uint8_t s_exp_shadow = EXP_DISP | EXP_LCD_RST | EXP_SDCS | EXP_CAN_S;
static bool s_exp_dir_set = false;
#else
static uint8_t s_exp_shadow = EXP_DISP | EXP_LCD_RST | EXP_SDCS | EXP_IDLE_BITS;
#endif

static esp_err_t exp_flush(void)
{
#if BOARD_CP_DISPLAY
    static esp_err_t s_last_err = ESP_OK;
    if (!s_exp_dir_set) {
        const uint8_t cfg[2] = { 0x03, 0x00 };   /* all pins outputs */
        if (i2c_write_bytes(EXP_PCA9554_ADDR, cfg, 2) == ESP_OK)
            s_exp_dir_set = true;
    }
    const uint8_t out[2] = { 0x01, s_exp_shadow };
    esp_err_t err = i2c_write_bytes(EXP_PCA9554_ADDR, out, 2);
    if (err != s_last_err) {                     /* log on state change only */
        if (err != ESP_OK)
            ESP_LOGW(TAG, "PCA9554 write FAILED: %s (addr 0x%02X, out=0x%02X)",
                     esp_err_to_name(err), EXP_PCA9554_ADDR, s_exp_shadow);
        else
            ESP_LOGI(TAG, "PCA9554 write ok (out=0x%02X)", s_exp_shadow);
        s_last_err = err;
    }
    return err;
#else
    i2c_write_byte(0x24, 0x01);                  /* CH422G output mode */
    return i2c_write_byte(0x38, s_exp_shadow);
#endif
}

static esp_err_t exp_update(uint8_t set_bits, uint8_t clear_bits)
{
    s_exp_shadow = (uint8_t)((s_exp_shadow | set_bits) & ~clear_bits);
    return exp_flush();
}

/* Recover a jammed I2C bus before the driver takes the pins. The GT911 powers
 * up with TP_RST floating (PCA9554 pins are hi-Z until first config write) and
 * can come up mid-transaction, holding SDA low — which then blocks EVERY bus
 * device, including the expander itself. Standard cure: clock SCL manually
 * until the slave releases SDA, then issue a STOP. Harmless when the bus is
 * already idle. */
static void i2c_bus_clear(void)
{
    gpio_config_t od = {
        .pin_bit_mask = (1ULL << I2C_MASTER_SCL_IO) | (1ULL << I2C_MASTER_SDA_IO),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&od);
    gpio_set_level(I2C_MASTER_SCL_IO, 1);
    gpio_set_level(I2C_MASTER_SDA_IO, 1);
    esp_rom_delay_us(10);

    if (gpio_get_level(I2C_MASTER_SDA_IO) == 0) {
        ESP_LOGW(TAG, "I2C: SDA stuck low at boot — clocking bus free");
        for (int i = 0; i < 16 && gpio_get_level(I2C_MASTER_SDA_IO) == 0; i++) {
            gpio_set_level(I2C_MASTER_SCL_IO, 0);
            esp_rom_delay_us(10);
            gpio_set_level(I2C_MASTER_SCL_IO, 1);
            esp_rom_delay_us(10);
        }
        /* STOP: SDA low->high while SCL high */
        gpio_set_level(I2C_MASTER_SDA_IO, 0);
        esp_rom_delay_us(10);
        gpio_set_level(I2C_MASTER_SDA_IO, 1);
        esp_rom_delay_us(10);
        ESP_LOGW(TAG, "I2C: bus clear done, SDA=%d", gpio_get_level(I2C_MASTER_SDA_IO));
    }
    gpio_reset_pin(I2C_MASTER_SCL_IO);
    gpio_reset_pin(I2C_MASTER_SDA_IO);
}

// GPIO initialization
static void gpio_init(void)
{
    // Zero-initialize the config structure
    gpio_config_t io_conf = {};
    // Disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // Bit mask of the pins, use GPIO4 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // Set as input mode
    io_conf.mode = GPIO_MODE_OUTPUT;

    gpio_config(&io_conf);
}

// Reset the touch screen
static void waveshare_esp32_s3_touch_reset(void)
{
    // Reset the touch screen. It is recommended to reset the touch screen before using it.
    exp_update(0, EXP_TP_RST);            // TP_RST low (reset asserted)
    esp_rom_delay_us(100 * 1000);
    gpio_set_level(GPIO_INPUT_IO_4, 0);   // INT low during reset -> I2C addr 0x5D
    esp_rom_delay_us(100 * 1000);
    exp_update(EXP_TP_RST, 0);            // release reset
    esp_rom_delay_us(200 * 1000);

    // Release CTP_IRQ. From here on it is GT911's OUTPUT (data-ready signal);
    // leaving the ESP32 driving it push-pull LOW fights the GT911's driver and
    // burns current continuously. As an input the line is also usable as a
    // light-sleep wake source.
    gpio_set_direction(GPIO_INPUT_IO_4, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_INPUT_IO_4, GPIO_FLOATING);
}

#endif

static esp_lcd_panel_handle_t s_panel_handle = NULL;

i2c_master_bus_handle_t waveshare_i2c_bus(void)
{
    return i2c_bus;
}

/**
 * Create + init the RGB panel. Shared between cold boot and wake-from-standby
 * (the panel is deleted in standby to release its NO_LIGHT_SLEEP PM lock).
 */
static esp_err_t create_rgb_panel(void)
{
    ESP_LOGI(TAG, "Install RGB LCD panel driver"); // Log the start of the RGB LCD panel driver installation
    esp_lcd_panel_handle_t panel_handle = NULL;    // Declare a handle for the LCD panel
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT, // Set the clock source for the panel
        .timings = {
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ, // Pixel clock frequency
            .h_res = EXAMPLE_LCD_H_RES,            // Horizontal resolution
            .v_res = EXAMPLE_LCD_V_RES,            // Vertical resolution
#if ESP_PANEL_USE_1024_600_LCD
            .hsync_pulse_width = 24,  // Horizontal sync pulse width (official 5B)
            .hsync_back_porch = 160,  // Horizontal back porch (official 5B)
            .hsync_front_porch = 160, // Horizontal front porch (official 5B)
            .vsync_pulse_width = 2,   // Vertical sync pulse width
            .vsync_back_porch = 23,   // Vertical back porch
            .vsync_front_porch = 12,  // Vertical front porch
#else
            .hsync_pulse_width = 4, // Horizontal sync pulse width
            .hsync_back_porch = 8,  // Horizontal back porch
            .hsync_front_porch = 8, // Horizontal front porch
            .vsync_pulse_width = 4, // Vertical sync pulse width
            .vsync_back_porch = 8,  // Vertical back porch
            .vsync_front_porch = 8, // Vertical front porch
#endif
            .flags = {
                .pclk_active_neg = 1, // Active low pixel clock
            },
        },
        .data_width = EXAMPLE_RGB_DATA_WIDTH,                    // Data width for RGB
        .in_color_format = LCD_COLOR_FMT_RGB565,                 // Input color format (replaces bits_per_pixel)
        .out_color_format = LCD_COLOR_FMT_RGB565,                // Output color format
        .num_fbs = LVGL_PORT_LCD_RGB_BUFFER_NUMS,                // Number of frame buffers
        .bounce_buffer_size_px = EXAMPLE_RGB_BOUNCE_BUFFER_SIZE, // Bounce buffer size in pixels
        .dma_burst_size = 64,                                    // DMA burst size (replaces sram/psram_trans_align)
        .hsync_gpio_num = EXAMPLE_LCD_IO_RGB_HSYNC,              // GPIO number for horizontal sync
        .vsync_gpio_num = EXAMPLE_LCD_IO_RGB_VSYNC,              // GPIO number for vertical sync
        .de_gpio_num = EXAMPLE_LCD_IO_RGB_DE,                    // GPIO number for data enable
        .pclk_gpio_num = EXAMPLE_LCD_IO_RGB_PCLK,                // GPIO number for pixel clock
        .disp_gpio_num = EXAMPLE_LCD_IO_RGB_DISP,                // GPIO number for display
        .data_gpio_nums = {
            EXAMPLE_LCD_IO_RGB_DATA0,
            EXAMPLE_LCD_IO_RGB_DATA1,
            EXAMPLE_LCD_IO_RGB_DATA2,
            EXAMPLE_LCD_IO_RGB_DATA3,
            EXAMPLE_LCD_IO_RGB_DATA4,
            EXAMPLE_LCD_IO_RGB_DATA5,
            EXAMPLE_LCD_IO_RGB_DATA6,
            EXAMPLE_LCD_IO_RGB_DATA7,
            EXAMPLE_LCD_IO_RGB_DATA8,
            EXAMPLE_LCD_IO_RGB_DATA9,
            EXAMPLE_LCD_IO_RGB_DATA10,
            EXAMPLE_LCD_IO_RGB_DATA11,
            EXAMPLE_LCD_IO_RGB_DATA12,
            EXAMPLE_LCD_IO_RGB_DATA13,
            EXAMPLE_LCD_IO_RGB_DATA14,
            EXAMPLE_LCD_IO_RGB_DATA15,
        },
        .flags = {
            .fb_in_psram = 1, // Use PSRAM for framebuffer
        },
    };

    // Create a new RGB panel with the specified configuration
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));
    s_panel_handle = panel_handle;

    ESP_LOGI(TAG, "Initialize RGB LCD panel");         // Log the initialization of the RGB LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle)); // Initialize the LCD panel
    return ESP_OK;
}

// Initialize RGB LCD + touch + LVGL (cold boot)
esp_err_t waveshare_esp32_s3_rgb_lcd_init()
{
    ESP_ERROR_CHECK(create_rgb_panel());

    esp_lcd_touch_handle_t tp_handle = NULL; // Declare a handle for the touch panel
#if CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911
    ESP_LOGI(TAG, "Initialize I2C bus");   // Log the initialization of the I2C bus
    i2c_bus_clear();                       // Free the bus if a slave holds SDA low
    i2c_master_init();                     // Initialize the I2C master

    // One-shot bus scan — bring-up diagnostics (expander/RTC/GT911 presence)
    ESP_LOGI(TAG, "I2C bus scan:");
    for (uint8_t a = 0x08; a <= 0x77; a++) {
        if (i2c_master_probe(i2c_bus, a, 50) == ESP_OK)
            ESP_LOGI(TAG, "  I2C device @ 0x%02X", a);
    }

    ESP_LOGI(TAG, "Initialize GPIO");      // Log GPIO initialization
    gpio_init();                           // Initialize GPIO pins
    ESP_LOGI(TAG, "Initialize Touch LCD"); // Log touch LCD initialization
    waveshare_esp32_s3_touch_reset();      // Reset the touch panel

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;                                          // Declare a handle for touch panel I/O
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG(); // Configure I2C for GT911 touch controller

    ESP_LOGI(TAG, "Initialize I2C panel IO");                                                                          // Log I2C panel I/O initialization
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &tp_io_config, &tp_io_handle)); // Create new I2C panel I/O

    ESP_LOGI(TAG, "Initialize touch controller GT911"); // Log touch controller initialization
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES,                // Set maximum X coordinate
        .y_max = EXAMPLE_LCD_V_RES,                // Set maximum Y coordinate
        .rst_gpio_num = EXAMPLE_PIN_NUM_TOUCH_RST, // GPIO number for reset
        .int_gpio_num = EXAMPLE_PIN_NUM_TOUCH_INT, // GPIO number for interrupt
        .levels = {
            .reset = 0,     // Reset level
            .interrupt = 0, // Interrupt level
        },
        .flags = {
            .swap_xy = 0,  // No swap of X and Y
            .mirror_x = 0, // No mirroring of X
            .mirror_y = 0, // No mirroring of Y
        },
    };
    // Touch is OPTIONAL during bring-up: a missing/unplugged touch FPC must
    // not brick the boot (the whole UI still runs, just without touch input;
    // lvgl_port_init handles tp_handle == NULL).
    esp_err_t tp_err = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp_handle);
    if (tp_err != ESP_OK) {
        ESP_LOGW(TAG, "GT911 not responding (%s) — continuing WITHOUT touch",
                 esp_err_to_name(tp_err));
        tp_handle = NULL;
    }
#endif                                                                               // CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911

    ESP_ERROR_CHECK(lvgl_port_init(s_panel_handle, tp_handle)); // Initialize LVGL with the panel and touch handles

    // Register callbacks for RGB panel events (after lvgl_port_init so the
    // vsync callback never fires before the LVGL task handle exists)
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = rgb_lcd_on_vsync_event, // Callback for vertical sync
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(s_panel_handle, &cbs, NULL)); // Register event callbacks

    return ESP_OK; // Return success
}

/******************************* Turn on the screen backlight **************************************/
esp_err_t wavesahre_rgb_lcd_bl_on()
{
#if BOARD_CP_DISPLAY
    // Panel enable (DISP_ON) + backlight boost (BL_ON) are separate pins here
    return exp_update(EXP_DISP | EXP_BL_ON, 0);
#else
    // On the Waveshare board the DISP bit IS the backlight boost enable
    return exp_update(EXP_DISP | EXP_TP_RST | EXP_LCD_RST | EXP_SDCS, 0);
#endif
}

/******************************* Turn off the screen backlight **************************************/
esp_err_t wavesahre_rgb_lcd_bl_off()
{
#if BOARD_CP_DISPLAY
    return exp_update(0, EXP_BL_ON);      // panel stays enabled, just dark
#else
    return exp_update(0, EXP_DISP);
#endif
}

/* All LCD output pins (used for sleep isolation + wake restore) */
static const int lcd_output_pins[] = {
    EXAMPLE_LCD_IO_RGB_PCLK,   EXAMPLE_LCD_IO_RGB_HSYNC,
    EXAMPLE_LCD_IO_RGB_VSYNC,  EXAMPLE_LCD_IO_RGB_DE,
    EXAMPLE_LCD_IO_RGB_DATA0,  EXAMPLE_LCD_IO_RGB_DATA1,
    EXAMPLE_LCD_IO_RGB_DATA2,  EXAMPLE_LCD_IO_RGB_DATA3,
    EXAMPLE_LCD_IO_RGB_DATA4,  EXAMPLE_LCD_IO_RGB_DATA5,
    EXAMPLE_LCD_IO_RGB_DATA6,  EXAMPLE_LCD_IO_RGB_DATA7,
    EXAMPLE_LCD_IO_RGB_DATA8,  EXAMPLE_LCD_IO_RGB_DATA9,
    EXAMPLE_LCD_IO_RGB_DATA10, EXAMPLE_LCD_IO_RGB_DATA11,
    EXAMPLE_LCD_IO_RGB_DATA12, EXAMPLE_LCD_IO_RGB_DATA13,
    EXAMPLE_LCD_IO_RGB_DATA14, EXAMPLE_LCD_IO_RGB_DATA15,
};
#define LCD_PIN_COUNT (sizeof(lcd_output_pins) / sizeof(lcd_output_pins[0]))

/******************************* Panel delete / recreate for standby ***************
 * The esp_lcd RGB driver holds an ESP_PM_NO_LIGHT_SLEEP power-management lock
 * for the panel's entire lifecycle, so automatic light sleep can never engage
 * while the panel exists. In standby we therefore DELETE the panel (stops the
 * DMA, frees the PSRAM framebuffers, releases the PM lock) and recreate it on
 * wake, rebinding LVGL to the new framebuffers.
 */
esp_err_t waveshare_lcd_panel_sleep(void)
{
    if (s_panel_handle == NULL) return ESP_ERR_INVALID_STATE;
    esp_err_t ret = esp_lcd_panel_del(s_panel_handle);
    s_panel_handle = NULL;
    ESP_LOGI(TAG, "RGB panel deleted (DMA stopped, PM lock released)");
    return ret;
}

esp_err_t waveshare_lcd_panel_wake(void)
{
    if (s_panel_handle != NULL) return ESP_OK;   // already alive

    // Return output-enable control of the LCD pins to the peripheral
    // (undo the software-OE hijack from waveshare_lcd_pins_float)
    for (int i = 0; i < LCD_PIN_COUNT; i++) {
        uint32_t reg = GPIO_FUNC0_OUT_SEL_CFG_REG + lcd_output_pins[i] * 4;
        REG_CLR_BIT(reg, BIT(10));
    }

    esp_err_t ret = create_rgb_panel();
    if (ret != ESP_OK) return ret;

    // Re-register vsync callback (LVGL task already exists on wake)
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = rgb_lcd_on_vsync_event,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(s_panel_handle, &cbs, NULL));

    // Point LVGL at the new framebuffers and force a full redraw
    lvgl_port_rebind_panel(s_panel_handle);
    ESP_LOGI(TAG, "RGB panel recreated + LVGL rebound");
    return ESP_OK;
}

/******************************* LCD hardware reset via IO expander ****************
 * Asserting reset puts the ST7262 LCD driver IC into hardware reset,
 * which drastically reduces its current draw through VCC.
 * GT911 remains operational (TP_RST stays deasserted).
 */
esp_err_t waveshare_lcd_reset_assert(void)
{
    exp_update(0, EXP_LCD_RST);
    ESP_LOGI(TAG, "LCD RST asserted (ST7262 in HW reset)");
    return ESP_OK;
}

esp_err_t waveshare_lcd_reset_release(void)
{
    exp_update(EXP_LCD_RST, 0);
    ESP_LOGI(TAG, "LCD RST released");
    vTaskDelay(pdMS_TO_TICKS(20)); // Let ST7262 come out of reset
    return ESP_OK;
}

/******************************* LCD pin isolation for sleep ************************/
void waveshare_lcd_pins_float(void)
{
    for (int i = 0; i < LCD_PIN_COUNT; i++) {
        int pin = lcd_output_pins[i];
        // Switch OE from LCD_CAM peripheral to software control (bit 10)
        uint32_t reg = GPIO_FUNC0_OUT_SEL_CFG_REG + pin * 4;
        REG_SET_BIT(reg, BIT(10));
        // Drive pin LOW — prevents CMOS shoot-through in ST7262 input buffers.
        // Floating inputs sit at undefined voltage → both P/N FETs partially ON
        // → each pin draws ~1 mA of shoot-through current.
        gpio_set_level(pin, 0);
        // Enable output driver (driving LOW, not hi-Z)
        if (pin < 32)
            REG_WRITE(GPIO_ENABLE_W1TS_REG, 1U << pin);
        else
            REG_WRITE(GPIO_ENABLE1_W1TS_REG, 1U << (pin - 32));
    }
    ESP_LOGI(TAG, "LCD pins driven LOW -- no shoot-through");
}

/******************************* Expander standby state ****************************/
esp_err_t waveshare_ch422g_all_low(void)
{
    // TP_RST stays HIGH so the GT911 remains operational (touch wake).
    // Panel enable, LCD reset, SD CS, backlight and buzzer all go low.
    // CAN_S is deliberately left alone — it is owned by
    // waveshare_can_transceiver_enable() (called from the TWAI lifecycle).
#if BOARD_CP_DISPLAY
    exp_update(EXP_TP_RST, EXP_DISP | EXP_LCD_RST | EXP_SDCS |
                           EXP_BL_ON | EXP_BUZZ_ON | EXP_NVM_RST);
#else
    exp_update(EXP_TP_RST | EXP_IDLE_BITS,
               EXP_DISP | EXP_LCD_RST | EXP_SDCS);
#endif
    ESP_LOGI(TAG, "Expander IOs low (TP_RST kept HIGH)");
    return ESP_OK;
}

/******************************* Expander wake *************************************/
esp_err_t waveshare_ch422g_wake(void)
{
    // CH422G: any I2C write wakes it (SLEEP bit auto-clears).
    // PCA9554 has no sleep state — re-flushing the shadow is harmless and
    // restores the outputs in case the chip lost power in between.
    return exp_flush();
}

/******************************* CAN transceiver standby ***************************/
esp_err_t waveshare_can_transceiver_enable(bool enable)
{
#if BOARD_CP_DISPLAY
    // S pin is pulled HIGH by R1 = standby; drive LOW for normal operation.
    esp_err_t ret = enable ? exp_update(0, EXP_CAN_S)
                           : exp_update(EXP_CAN_S, 0);
    ESP_LOGI(TAG, "CAN transceiver %s", enable ? "normal mode" : "standby");
    return ret;
#else
    (void)enable;   // Waveshare: S pin is hard-wired, nothing to do
    return ESP_OK;
#endif
}

/******************************* Buzzer click **************************************/
#if BOARD_CP_DISPLAY
#include "esp_timer.h"
#include "rtc_pcf85063.h"

/* Tone = RTC CLKOUT (PCF85063: 1024/2048/4096 Hz used here) gated by the
 * expander's BUZZ_ON bit. A sequence is a chain of one-shot esp_timer steps:
 * each note sets the CLKOUT frequency while the gate is closed, then opens
 * the gate for its duration. CLKOUT is switched off after the last note.
 * The buzzer (PKLCS1212E4001) resonates at 4 kHz, so lower notes are
 * noticeably quieter. */
typedef struct { uint16_t hz, on_ms, off_ms; } buzz_note_t;

static const buzz_note_t k_click[]    = { {4096,  40,   0} };
static const buzz_note_t k_warning[]  = { {2048, 150, 120}, {2048, 150,   0} };
static const buzz_note_t k_failure[]  = { {4096, 200,  80}, {2048, 200,  80},
                                          {1024, 300,   0} };            /* falling */
static const buzz_note_t k_critical[] = { {4096, 250,  50}, {1024, 250,  50},
                                          {4096, 250,  50}, {1024, 250,  50},
                                          {4096, 250,  50}, {1024, 400,   0} }; /* siren */

static esp_timer_handle_t  s_buzz_timer = NULL;
static const buzz_note_t  *s_seq = NULL;
static volatile int        s_seq_len = 0, s_seq_idx = 0;
static volatile bool       s_tone_on = false;
static volatile bool       s_alarm_busy = false;   /* multi-note alarm playing */

static void buzz_note_start(const buzz_note_t *n)
{
    rtc_pcf85063_clkout_set_hz(n->hz);
    exp_update(EXP_BUZZ_ON, 0);
    s_tone_on = true;
    esp_timer_start_once(s_buzz_timer, (uint64_t)n->on_ms * 1000);
}

static void buzzer_step_cb(void *arg)
{
    (void)arg;
    if (s_tone_on) {                                    /* end of a note */
        exp_update(0, EXP_BUZZ_ON);
        s_tone_on = false;
        const buzz_note_t *n = &s_seq[s_seq_idx];
        if (++s_seq_idx < s_seq_len) {
            esp_timer_start_once(s_buzz_timer, (uint64_t)(n->off_ms ? n->off_ms : 1) * 1000);
            return;
        }
        rtc_pcf85063_clkout_set_hz(0);
        s_alarm_busy = false;
    } else if (s_seq_idx < s_seq_len) {                 /* next note */
        buzz_note_start(&s_seq[s_seq_idx]);
    }
}

static esp_err_t buzz_play(const buzz_note_t *seq, int len)
{
    if (!s_buzz_timer) {
        const esp_timer_create_args_t args = {
            .callback = buzzer_step_cb,
            .name = "buzz",
        };
        if (esp_timer_create(&args, &s_buzz_timer) != ESP_OK) return ESP_FAIL;
    }
    esp_timer_stop(s_buzz_timer);
    exp_update(0, EXP_BUZZ_ON);
    s_seq = seq;
    s_seq_len = len;
    s_seq_idx = 0;
    s_alarm_busy = (len > 1);
    buzz_note_start(&seq[0]);
    return ESP_OK;
}

esp_err_t waveshare_buzzer_alarm(buzz_alarm_t level)
{
    switch (level) {
    case BUZZ_CRITICAL: return buzz_play(k_critical, sizeof(k_critical) / sizeof(k_critical[0]));
    case BUZZ_FAILURE:  return buzz_play(k_failure,  sizeof(k_failure)  / sizeof(k_failure[0]));
    default:            return buzz_play(k_warning,  sizeof(k_warning)  / sizeof(k_warning[0]));
    }
}

esp_err_t waveshare_buzzer_click(void)
{
    if (s_alarm_busy) return ESP_OK;                   /* never cut an alarm short */
    return buzz_play(k_click, 1);
}
#else
esp_err_t waveshare_buzzer_alarm(buzz_alarm_t level)
{
    (void)level;
    return ESP_OK;   // no buzzer on the Waveshare board
}

esp_err_t waveshare_buzzer_click(void)
{
    return ESP_OK;   // no buzzer on the Waveshare board
}
#endif

/******************************* I2C diagnostics (debug-port "SC") *****************/
void waveshare_i2c_diag(void)
{
    if (!i2c_bus) { ESP_LOGW(TAG, "[DIAG] no I2C bus"); return; }

    ESP_LOGI(TAG, "[DIAG] I2C scan:");
    for (uint8_t a = 0x08; a <= 0x77; a++) {
        if (i2c_master_probe(i2c_bus, a, 50) == ESP_OK)
            ESP_LOGI(TAG, "[DIAG]   device @ 0x%02X", a);
    }

#if BOARD_CP_DISPLAY
    // Read back all four PCA9554 registers (0=input, 1=output, 2=polarity,
    // 3=config). Input reg shows the REAL pin levels.
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = EXP_PCA9554_ADDR,
        .scl_speed_hz    = I2C_MASTER_FREQ_HZ,
    };
    i2c_master_dev_handle_t dev;
    if (i2c_master_bus_add_device(i2c_bus, &cfg, &dev) == ESP_OK) {
        for (uint8_t r = 0; r < 4; r++) {
            uint8_t v = 0;
            esp_err_t err = i2c_master_transmit_receive(dev, &r, 1, &v, 1,
                                                        I2C_MASTER_TIMEOUT_MS);
            if (err == ESP_OK)
                ESP_LOGI(TAG, "[DIAG] PCA9554 reg%u = 0x%02X", r, v);
            else
                ESP_LOGW(TAG, "[DIAG] PCA9554 reg%u read FAILED: %s",
                         r, esp_err_to_name(err));
        }
        i2c_master_bus_rm_device(dev);
    }
    ESP_LOGI(TAG, "[DIAG] expander shadow = 0x%02X (TP_RST=%d DISP=%d LCD_RST=%d "
             "SDCS=%d BUZZ=%d BL=%d CAN_S=%d)",
             s_exp_shadow,
             !!(s_exp_shadow & EXP_TP_RST), !!(s_exp_shadow & EXP_DISP),
             !!(s_exp_shadow & EXP_LCD_RST), !!(s_exp_shadow & EXP_SDCS),
             !!(s_exp_shadow & EXP_BUZZ_ON), !!(s_exp_shadow & EXP_BL_ON),
             !!(s_exp_shadow & EXP_CAN_S));
#endif

    /* Bus-health stress test with REAL transactions. i2c_master_probe() is not
     * a reliable health gauge: it always runs at 100 kHz and reports "found"
     * for any status other than an explicit NACK/timeout. Each target is read
     * N times at I2C_MASTER_FREQ_HZ; the first good value is the reference. */
    static const struct { uint8_t addr; uint8_t reg[2]; uint8_t reg_len; const char *name; } tgt[] = {
#if BOARD_CP_DISPLAY
        { EXP_PCA9554_ADDR, { 0x01 },       1, "PCA9554 out" },
#endif
        { 0x51,             { 0x00 },       1, "RTC ctrl1"   },
        { 0x5D,             { 0x81, 0x40 }, 2, "GT911 id"    },
        { 0x30,             { 0x00 },       1, "empty 0x30"  },   /* must NACK */
    };
    const int N = 200;
    esp_log_level_set("i2c.master", ESP_LOG_NONE);   /* NACKs would flood the log */
    for (size_t t = 0; t < sizeof(tgt) / sizeof(tgt[0]); t++) {
        i2c_device_config_t dc = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address  = tgt[t].addr,
            .scl_speed_hz    = I2C_MASTER_FREQ_HZ,
        };
        i2c_master_dev_handle_t d;
        if (i2c_master_bus_add_device(i2c_bus, &dc, &d) != ESP_OK) continue;
        int ok = 0, nack = 0, other = 0, mismatch = 0;
        int ref = -1;
        for (int i = 0; i < N; i++) {
            uint8_t v = 0;
            esp_err_t e = i2c_master_transmit_receive(d, tgt[t].reg, tgt[t].reg_len,
                                                      &v, 1, 50);
            if (e == ESP_OK) {
                ok++;
                if (ref < 0) ref = v;
                else if (v != ref) mismatch++;
            } else if (e == ESP_ERR_NOT_FOUND || e == ESP_ERR_INVALID_STATE) {
                nack++;
            } else {
                other++;
            }
        }
        i2c_master_bus_rm_device(d);
        ESP_LOGI(TAG, "[DIAG] stress %-12s @0x%02X: ok=%d nack=%d err=%d mismatch=%d ref=0x%02X",
                 tgt[t].name, tgt[t].addr, ok, nack, other, mismatch, ref < 0 ? 0 : ref);
    }
    esp_log_level_set("i2c.master", ESP_LOG_INFO);
    ESP_LOGI(TAG, "[DIAG] bus speed %d Hz, N=%d per target", I2C_MASTER_FREQ_HZ, N);
}

/******************************* Direct touch poll (for sleep mode) ****************/
bool waveshare_touch_is_pressed(void)
{
    if (!i2c_bus) return false;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = 0x5D,           // GT911 default I2C address
        .scl_speed_hz    = I2C_MASTER_FREQ_HZ,
    };
    i2c_master_dev_handle_t dev;
    esp_err_t add_ret = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &dev);
    if (add_ret != ESP_OK) {
        ESP_LOGW(TAG, "[TOUCH] i2c add_device err=%s", esp_err_to_name(add_ret));
        return false;
    }

    // GT911 register 0x814E: bit[7]=buffer ready, bit[3:0]=touch points
    uint8_t reg[2] = { 0x81, 0x4E };
    uint8_t val = 0;
    esp_err_t ret = i2c_master_transmit_receive(dev, reg, 2, &val, 1,
                                                 I2C_MASTER_TIMEOUT_MS);

    // Only log real touches and the occasional keep-alive: this runs several
    // times per second in standby, and console traffic is not free there.
    static uint32_t diag_cnt = 0;
    if (val != 0 || ret != ESP_OK || ++diag_cnt % 240 == 1) {
        ESP_LOGI(TAG, "[TOUCH] reg 0x814E=0x%02X ret=%s", val, esp_err_to_name(ret));
    }

    // Clear the buffer status (write 0 to 0x814E) so GT911 updates next read
    if (ret == ESP_OK) {
        uint8_t clr[3] = { 0x81, 0x4E, 0x00 };
        i2c_master_transmit(dev, clr, 3, I2C_MASTER_TIMEOUT_MS);
    }

    i2c_master_bus_rm_device(dev);

    return (ret == ESP_OK) && ((val & 0x0F) > 0);
}
