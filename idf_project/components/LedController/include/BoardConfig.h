#pragma once
#include <stdint.h>
#include "driver/gpio.h"

#define WS2812B_NUM 8
#define PCA9955B_NUM 6
#define PCA9955B_CH_NUM 5 * PCA9955B_NUM

#define I2C_FREQ 100000

#define I2C_TIMEOUT_MS 20
#define RMT_TIMEOUT_MS 20

typedef struct {
    union {
        gpio_num_t rmt_pins[WS2812B_NUM];

        struct {
            gpio_num_t ws2812b_0;  // ch0
            gpio_num_t ws2812b_1;  // ch1
            gpio_num_t ws2812b_2;  // ch2
            gpio_num_t ws2812b_3;  // ch3
            gpio_num_t ws2812b_4;  // ch4
            gpio_num_t ws2812b_5;  // ch5
            gpio_num_t ws2812b_6;  // ch6
            gpio_num_t ws2812b_7;  // ch7
        };
    };

    union {
        uint8_t i2c_addrs[PCA9955B_NUM];

        struct {
            uint8_t pca9955b_0;  // ch 8~12
            uint8_t pca9955b_1;  // ch 13~17
            uint8_t pca9955b_2;  // ch 18~22
            uint8_t pca9955b_3;  // ch 23~27
            uint8_t pca9955b_4;  // ch 28~32
            uint8_t pca9955b_5;  // ch 33~37
            uint8_t pca9955b_6;  // ch 38~42
            uint8_t pca9955b_7;  // ch 43~47
        };
    };

} hw_config_t;

typedef struct {
    union {
        uint16_t pixel_counts[WS2812B_NUM + PCA9955B_CH_NUM];

        struct {
            uint16_t rmt_strips[WS2812B_NUM];
            uint16_t i2c_leds[PCA9955B_CH_NUM];
        };
    };
} ch_info_t;

extern const hw_config_t BOARD_HW_CONFIG;
