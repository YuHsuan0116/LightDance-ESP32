#pragma once

#include "driver/gpio.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"

#include "BoardConfig.h"
#include "ws2812b_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    rmt_channel_handle_t rmt_channel;
    rmt_encoder_handle_t rmt_encoder;

    uint16_t pixel_num;
    uint8_t* buffer;
} ws2812b_dev_t;

typedef ws2812b_dev_t* ws2812b_handle_t;

esp_err_t ws2812b_init(gpio_num_t gpio_num, uint16_t pixel_num, ws2812b_handle_t* ws2812b);
esp_err_t ws2812b_set_pixel(ws2812b_handle_t ws2812b, int pixel_idx, uint8_t red, uint8_t green, uint8_t blue);
esp_err_t ws2812b_show(ws2812b_handle_t ws2812b);
esp_err_t ws2812b_del(ws2812b_handle_t* ws2812b);

esp_err_t ws2812b_write(ws2812b_handle_t ws2812b, uint8_t* _buffer);
void ws2812b_fill(ws2812b_handle_t ws2812b, uint8_t red, uint8_t green, uint8_t blue);
esp_err_t ws2812b_wait_done(ws2812b_handle_t ws2812b);

void ws2812b_test();

#ifdef __cplusplus
}
#endif