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
 * @brief Helper to write a single byte via I2C to a device (replaces legacy i2c_master_write_to_device)
 */
static esp_err_t i2c_write_byte(uint8_t dev_addr, uint8_t data)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    i2c_master_dev_handle_t dev;
    esp_err_t ret = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &dev);
    if (ret != ESP_OK) return ret;
    ret = i2c_master_transmit(dev, &data, 1, I2C_MASTER_TIMEOUT_MS);
    i2c_master_bus_rm_device(dev);
    return ret;
}

/**
 * @brief Write a register on a device using [reg_addr, data] I2C frame.
 *        Used for ST7262 (I2C addr 0x3C) which expects [command_addr][param].
 */
static esp_err_t i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = dev_addr,
        .scl_speed_hz    = I2C_MASTER_FREQ_HZ,
    };
    i2c_master_dev_handle_t dev;
    esp_err_t ret = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &dev);
    if (ret != ESP_OK) return ret;
    uint8_t buf[2] = { reg_addr, data };
    ret = i2c_master_transmit(dev, buf, 2, I2C_MASTER_TIMEOUT_MS);
    i2c_master_bus_rm_device(dev);
    return ret;
}

// GPIO initialization
void gpio_init(void)
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
void waveshare_esp32_s3_touch_reset()
{
    i2c_write_byte(0x24, 0x01);

    // Reset the touch screen. It is recommended to reset the touch screen before using it.
    i2c_write_byte(0x38, 0x2C);
    esp_rom_delay_us(100 * 1000);
    gpio_set_level(GPIO_INPUT_IO_4, 0);
    esp_rom_delay_us(100 * 1000);
    i2c_write_byte(0x38, 0x2E);
    esp_rom_delay_us(200 * 1000);
}

#endif

static esp_lcd_panel_handle_t s_panel_handle = NULL;

// Initialize RGB LCD
esp_err_t waveshare_esp32_s3_rgb_lcd_init()
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
    s_panel_handle = panel_handle;  // save for restart after light sleep

    ESP_LOGI(TAG, "Initialize RGB LCD panel");         // Log the initialization of the RGB LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle)); // Initialize the LCD panel

    esp_lcd_touch_handle_t tp_handle = NULL; // Declare a handle for the touch panel
#if CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911
    ESP_LOGI(TAG, "Initialize I2C bus");   // Log the initialization of the I2C bus
    i2c_master_init();                     // Initialize the I2C master
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
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp_handle)); // Create new I2C GT911 touch controller
#endif                                                                               // CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911

    ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle)); // Initialize LVGL with the panel and touch handles

    // Register callbacks for RGB panel events
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = rgb_lcd_on_vsync_event, // Callback for vertical sync
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, NULL)); // Register event callbacks

    return ESP_OK; // Return success
}

/******************************* Turn on the screen backlight **************************************/
esp_err_t wavesahre_rgb_lcd_bl_on()
{
    // Configure CH422G to output mode
    i2c_write_byte(0x24, 0x01);

    // Pull the backlight pin high to light the screen backlight
    i2c_write_byte(0x38, 0x1E);
    return ESP_OK;
}

/******************************* Turn off the screen backlight **************************************/
esp_err_t wavesahre_rgb_lcd_bl_off()
{
    // Configure CH422G to output mode
    i2c_write_byte(0x24, 0x01);

    // Turn off the screen backlight by pulling the backlight pin low
    i2c_write_byte(0x38, 0x1A);
    return ESP_OK;
}

/******************************* Restart LCD panel DMA (after light sleep) ********/
esp_err_t waveshare_lcd_restart(void)
{
    if (s_panel_handle == NULL) return ESP_ERR_INVALID_STATE;
    return esp_lcd_rgb_panel_restart(s_panel_handle);
}

/******************************* LCD hardware reset via CH422G **********************
 * CH422G IO3 (0x08) = LCD_RST on this board.
 * Asserting reset puts the ST7262 LCD driver IC into hardware reset,
 * which drastically reduces its current draw through VCC.
 * GT911 remains operational (IO1 = TP_RST stays deasserted).
 */
esp_err_t waveshare_lcd_reset_assert(void)
{
    i2c_write_byte(0x24, 0x01);   // CH422G output mode
    // IO1=1(TP_RST released) + IO4=1(SD/INT) → IO3=0 means LCD_RST asserted
    i2c_write_byte(0x38, 0x12);   // 0x12 = IO1 + IO4, LCD_RST=0, BL=0
    ESP_LOGI(TAG, "LCD RST asserted (ST7262 in HW reset)");
    return ESP_OK;
}

esp_err_t waveshare_lcd_reset_release(void)
{
    i2c_write_byte(0x24, 0x01);   // CH422G output mode
    // IO1=1 + IO3=1 + IO4=1, BL still off
    i2c_write_byte(0x38, 0x1A);   // 0x1A = IO1 + IO3 + IO4
    ESP_LOGI(TAG, "LCD RST released");
    vTaskDelay(pdMS_TO_TICKS(20)); // Let ST7262 come out of reset
    return ESP_OK;
}

/******************************* LCD pin isolation for sleep ************************/
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

void waveshare_lcd_pins_drive(void)
{
    for (int i = 0; i < LCD_PIN_COUNT; i++) {
        int pin = lcd_output_pins[i];
        // Enable output driver
        if (pin < 32)
            REG_WRITE(GPIO_ENABLE_W1TS_REG, 1U << pin);
        else
            REG_WRITE(GPIO_ENABLE1_W1TS_REG, 1U << (pin - 32));
        // Return OE to LCD_CAM peripheral (clear bit 10)
        uint32_t reg = GPIO_FUNC0_OUT_SEL_CFG_REG + pin * 4;
        REG_CLR_BIT(reg, BIT(10));
    }
    // Restart DMA to re-sync LCD timing
    if (s_panel_handle)
        esp_lcd_rgb_panel_restart(s_panel_handle);
    ESP_LOGI(TAG, "LCD pins restored + panel restarted");
}

/******************************* GT911 sleep / wake ********************************/

/* Helper: send one I2C command to GT911 (address 0x5D) */
static esp_err_t gt911_write_reg(uint8_t reg_h, uint8_t reg_l, uint8_t val)
{
    if (!i2c_bus) return ESP_ERR_INVALID_STATE;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = 0x5D,
        .scl_speed_hz    = I2C_MASTER_FREQ_HZ,
    };
    i2c_master_dev_handle_t dev;
    esp_err_t ret = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &dev);
    if (ret != ESP_OK) return ret;
    uint8_t cmd[3] = { reg_h, reg_l, val };
    ret = i2c_master_transmit(dev, cmd, 3, I2C_MASTER_TIMEOUT_MS);
    i2c_master_bus_rm_device(dev);
    return ret;
}

esp_err_t waveshare_gt911_sleep(void)
{
    // GT911 sleep entry: hold INT LOW, then write 0x05 to 0x8040
    gpio_set_direction(GPIO_INPUT_IO_4, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_INPUT_IO_4, 0);   // INT LOW
    esp_rom_delay_us(200);                 // hold >100µs
    esp_err_t ret = gt911_write_reg(0x80, 0x40, 0x05);
    // Leave GPIO4 LOW — GT911 is in sleep, no conflict
    return ret;
}

esp_err_t waveshare_gt911_wake(void)
{
    // GT911 wake: pulse INT HIGH via ESP32 GPIO4 (the actual INT pin)
    // Match Espressif BSP: OUTPUT HIGH → wait → OUTPUT_OD (release)
    gpio_set_direction(GPIO_INPUT_IO_4, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_INPUT_IO_4, 1);   // INT HIGH — wakes GT911
    vTaskDelay(pdMS_TO_TICKS(5));          // hold 5ms
    // Release INT pin (open-drain = GT911 can drive it again)
    gpio_set_direction(GPIO_INPUT_IO_4, GPIO_MODE_OUTPUT_OD);
    vTaskDelay(pdMS_TO_TICKS(55));         // GT911 needs ~50ms to initialize
    return ESP_OK;
}

/******************************* CH422G all IOs LOW ********************************/
esp_err_t waveshare_ch422g_all_low(void)
{
    i2c_write_byte(0x24, 0x01);   // Push-pull output mode
    // IO1(CTP_RST)=HIGH so GT911 stays in SW sleep (not HW reset).
    // IO2(DISP)=LOW, IO3(LCD_RST)=LOW, IO4(SDCS)=LOW, rest LOW.
    i2c_write_byte(0x38, 0x02);   // 0x02 = only IO1 HIGH
    ESP_LOGI(TAG, "CH422G IOs low (CTP_RST kept HIGH)");
    return ESP_OK;
}

/******************************* CH422G sleep / wake ********************************/
esp_err_t waveshare_ch422g_sleep(void)
{
    // CH422G mode register: IO_OE (0x01) | SLEEP (0x08) = 0x09
    // Outputs maintain their state. Wakes on any I2C command to CH422G.
    return i2c_write_byte(0x24, 0x09);
}

esp_err_t waveshare_ch422g_wake(void)
{
    // Any I2C write to CH422G wakes it; SLEEP bit auto-clears.
    // Set IO_OE mode explicitly to be safe.
    return i2c_write_byte(0x24, 0x01);
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

    static uint32_t diag_cnt = 0;
    if (++diag_cnt % 10 == 1 || val != 0) {
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

/******************************* Example code **************************************/
static void draw_event_cb(lv_event_t *e) // Draw event callback function
{
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e); // Get the draw part descriptor
    if (dsc->part == LV_PART_ITEMS)
    {                                                                 // If drawing chart items
        lv_obj_t *obj = lv_event_get_target(e);                       // Get the target object of the event
        lv_chart_series_t *ser = lv_chart_get_series_next(obj, NULL); // Get the series of the chart
        uint32_t cnt = lv_chart_get_point_count(obj);                 // Get the number of points in the chart
        /* Make older values more transparent */
        dsc->rect_dsc->bg_opa = (LV_OPA_COVER * dsc->id) / (cnt - 1); // Set opacity based on the index

        /* Make smaller values blue, higher values red  */
        lv_coord_t *x_array = lv_chart_get_x_array(obj, ser); // Get the X-axis array
        lv_coord_t *y_array = lv_chart_get_y_array(obj, ser); // Get the Y-axis array
        /* dsc->id is the drawing order, but we need the index of the point being drawn dsc->id  */
        uint32_t start_point = lv_chart_get_x_start_point(obj, ser); // Get the start point of the chart
        uint32_t p_act = (start_point + dsc->id) % cnt;              // Calculate the actual index based on the start point
        lv_opa_t x_opa = (x_array[p_act] * LV_OPA_50) / 200;         // Calculate X-axis opacity
        lv_opa_t y_opa = (y_array[p_act] * LV_OPA_50) / 1000;        // Calculate Y-axis opacity

        dsc->rect_dsc->bg_color = lv_color_mix(lv_palette_main(LV_PALETTE_RED), // Mix colors
                                               lv_palette_main(LV_PALETTE_BLUE),
                                               x_opa + y_opa);
    }
}

static void add_data(lv_timer_t *timer) // Timer callback to add data to the chart
{
    lv_obj_t *chart = timer->user_data;                                                                        // Get the chart associated with the timer
    lv_chart_set_next_value2(chart, lv_chart_get_series_next(chart, NULL), lv_rand(0, 200), lv_rand(0, 1000)); // Add random data to the chart
}

// This demo UI is adapted from LVGL official example: https://docs.lvgl.io/master/examples.html#scatter-chart
void example_lvgl_demo_ui() // LVGL demo UI initialization function
{
    lv_obj_t *scr = lv_scr_act();                                              // Get the current active screen
    lv_obj_t *chart = lv_chart_create(scr);                                    // Create a chart object
    lv_obj_set_size(chart, 200, 150);                                          // Set chart size
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 0);                                // Center the chart on the screen
    lv_obj_add_event_cb(chart, draw_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL); // Add draw event callback
    lv_obj_set_style_line_width(chart, 0, LV_PART_ITEMS);                      /* Remove chart lines  */

    lv_chart_set_type(chart, LV_CHART_TYPE_SCATTER); // Set chart type to scatter

    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 5, 5, 5, 1, true, 30);  // Set X-axis ticks
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 10, 5, 6, 5, true, 50); // Set Y-axis ticks

    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, 200);  // Set X-axis range
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 1000); // Set Y-axis range

    lv_chart_set_point_count(chart, 50); // Set the number of points in the chart

    lv_chart_series_t *ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y); // Add a series to the chart
    for (int i = 0; i < 50; i++)
    {                                                                            // Add random points to the chart
        lv_chart_set_next_value2(chart, ser, lv_rand(0, 200), lv_rand(0, 1000)); // Set X and Y values
    }

    lv_timer_create(add_data, 100, chart); // Create a timer to add new data every 100ms
}
