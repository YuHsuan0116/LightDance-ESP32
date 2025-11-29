#pragma once

#include "driver/gpio.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"

#include "ws2812b_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RMT_MAX_CH_NUM 4
#define RMT_TIMEOUT_MS 20

typedef struct {
    rmt_channel_handle_t rmt_channel;
    rmt_encoder_handle_t rmt_encoder;

    gpio_num_t gpio_num;
    uint16_t pixel_num;
    uint8_t* buffer;
    size_t buffer_size;
} ws2812b_dev_t;

typedef ws2812b_dev_t* ws2812b_handle_t;

esp_err_t ws2812b_init(gpio_num_t gpio_num, uint16_t pixel_num, ws2812b_handle_t* ws2812b);
esp_err_t ws2812b_show(ws2812b_handle_t ws2812b);
esp_err_t ws2812b_del(ws2812b_handle_t* ws2812b);

void ws2812b_fill(ws2812b_handle_t ws2812b, uint8_t red, uint8_t green, uint8_t blue);
esp_err_t ws2812b_wait_done(ws2812b_handle_t ws2812b);
void ws2812b_test();

#ifdef __cplusplus
}
#endif