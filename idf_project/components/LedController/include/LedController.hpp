#pragma once

#include "pca9955b_hal.h"
#include "ws2812b_hal.h"

typedef enum {
    LED_TYPE_WS2812B = 0,
    LED_TYPE_PCA9955B,
} led_type_t;

typedef struct {
    led_type_t type;

    gpio_num_t gpio_num;
    uint16_t pixel_num;

    uint8_t i2c_addr;
} hw_config_t;

typedef struct {
    uint8_t ch_idx;
    led_type_t type;

    gpio_num_t rmt_gpio;
    uint8_t i2c_addr;
    uint8_t pca_ch;
} ch_config_t;

typedef struct {
    uint8_t hw_num;
    hw_config_t hw_configs[12];

    uint8_t ch_num;
    ch_config_t ch_configs[44];
} LedController_config_t;

typedef struct {
    led_type_t type;

    uint8_t* buffer;
    uint16_t buffer_size;
} led_channel_t;

class LedController {
  public:
    LedController();
    ~LedController();

    esp_err_t load_config();
    esp_err_t init();
    esp_err_t show();
    esp_err_t del();

    esp_err_t load_config_test();
    esp_err_t fill(uint8_t, uint8_t, uint8_t);

  private:
    uint8_t hw_num;
    hw_config_t dev_configs[12];

    uint8_t ws2812b_num;
    ws2812b_handle_t ws2812b_devs[4];

    uint8_t pca9955b_num;
    pca9955b_handle_t pca9955b_devs[8];

    uint8_t ch_num;
    ch_config_t ch_configs[44];
    led_channel_t ch[44];

    i2c_master_bus_handle_t bus_handle;
};
