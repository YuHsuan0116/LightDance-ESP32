#pragma once

#include "../def.h"
#include "../i2c_hal/i2c_hal.h"
#include "../rmt_hal/rmt_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    led_type_t type;
    pca9955b_handle_t pca9955b;
    ws2812b_handle_t ws2812b;
} channel_handle_t;

esp_err_t channel_handle_config(led_config_t* led_config, channel_handle_t* channel_handle);

esp_err_t channel_handle_write(color_t* colors, channel_handle_t* channel_handle);

esp_err_t channel_handle_del(channel_handle_t* channel_handle);

esp_err_t channel_handle_write_RGB(const uint8_t r, const uint8_t g, const uint8_t b, channel_handle_t* channel_handle);

#ifdef __cplusplus
}
#endif