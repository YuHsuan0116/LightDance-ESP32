#pragma once

#include "../def.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RMT_TIMEOUT_MS 1
#define WS2812B_RESOLUTION 10000000
#define WS2812B_MAXIMUM_LED_COUNT 100

typedef struct {
    uint32_t resolution;
} rmt_encoder_config_t;

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t* bytes_encoder;
    rmt_encoder_t* copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} encoder_t;

typedef struct {
    bool rmt_activate;
    uint8_t led_count;
    rmt_channel_handle_t rmt_channel;
    rmt_encoder_handle_t rmt_encoder;

} ws2812b_handle_t;

size_t encode(rmt_encoder_t* rmt_encoder, rmt_channel_handle_t rmt_channel, const void* buffer, size_t buffer_size, rmt_encode_state_t* ret_state);

esp_err_t del_encoder(rmt_encoder_t* rmt_encoder);

esp_err_t encoder_reset(rmt_encoder_t* rmt_encoder);

esp_err_t ws2812b_new_encoder(rmt_encoder_handle_t* ret_encoder);

esp_err_t ws2812b_new_channel(gpio_num_t rmt_gpio, rmt_channel_handle_t* ret_channel);

esp_err_t ws2812b_config(led_config_t* led_config, ws2812b_handle_t* ws2812b);

esp_err_t ws2812b_write(const uint8_t* buffer, const size_t buffer_size, ws2812b_handle_t* ws2812b);

esp_err_t ws2812b_del(ws2812b_handle_t* ws2812b);

// esp_err_t ws2812b_write_rgb(uint8_t const r, uint8_t const g, uint8_t const b, ws2812b_handle_t* ws2812b);

#ifdef __cplusplus
}
#endif