#include "rmt_hal.h"

#define RMT_BYTES_ENCODER_CONFIG_DEFAULT()                                      \
    {                                                                           \
        .bit0 =                                                                 \
            {                                                                   \
                .level0 = 1,                                                    \
                .duration0 = (uint32_t)(0.4 * (WS2812B_RESOLUTION) / 1000000),  \
                .level1 = 0,                                                    \
                .duration1 = (uint32_t)(0.85 * (WS2812B_RESOLUTION) / 1000000), \
            },                                                                  \
        .bit1 =                                                                 \
            {                                                                   \
                .level0 = 1,                                                    \
                .duration0 = (uint32_t)(0.8 * (WS2812B_RESOLUTION) / 1000000),  \
                .level1 = 0,                                                    \
                .duration1 = (uint32_t)(0.45 * (WS2812B_RESOLUTION) / 1000000), \
            },                                                                  \
        .flags =                                                                \
            {                                                                   \
                .msb_first = 1,                                                 \
            },                                                                  \
    }

#define WS2812B_RESET_TICKS (WS2812B_RESOLUTION / 1000000 * 50 / 2)

#define WS2812B_RESET_CODE_DEFAULT()      \
    ((rmt_symbol_word_t){                 \
        .level0 = 0,                      \
        .duration0 = WS2812B_RESET_TICKS, \
        .level1 = 0,                      \
        .duration1 = WS2812B_RESET_TICKS, \
    })

static rmt_transmit_config_t rmt_tx_config = {
    .loop_count = 0,
    .flags =
        {
            .eot_level = 0,
            .queue_nonblocking = false,
        },
};

size_t encode(rmt_encoder_t* rmt_encoder, rmt_channel_handle_t rmt_channel, const void* buffer, size_t buffer_size, rmt_encode_state_t* ret_state) {
    encoder_t* ws2812b_encoder = __containerof(rmt_encoder, encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = ws2812b_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = ws2812b_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;
    switch(ws2812b_encoder->state) {
        case 0:  // send RGB data
            encoded_symbols += bytes_encoder->encode(bytes_encoder, rmt_channel, buffer, buffer_size, &session_state);
            if(session_state & RMT_ENCODING_COMPLETE) {
                ws2812b_encoder->state = 1;
            }
            if(session_state & RMT_ENCODING_MEM_FULL) {
                state |= RMT_ENCODING_MEM_FULL;
                break;  // stop encoding, skip reset code
            }
            /* fall through */
        case 1:  // send reset code
            encoded_symbols +=
                copy_encoder->encode(copy_encoder, rmt_channel, &ws2812b_encoder->reset_code, sizeof(ws2812b_encoder->reset_code), &session_state);
            if(session_state & RMT_ENCODING_COMPLETE) {
                ws2812b_encoder->state = RMT_ENCODING_RESET;
                state |= RMT_ENCODING_COMPLETE;
            }
            if(session_state & RMT_ENCODING_MEM_FULL) {
                state |= RMT_ENCODING_MEM_FULL;
            }
            break;
    }

    *ret_state = state;
    return encoded_symbols;
}

esp_err_t del_encoder(rmt_encoder_t* rmt_encoder) {
    encoder_t* ws2812b_encoder = __containerof(rmt_encoder, encoder_t, base);
    rmt_del_encoder(ws2812b_encoder->bytes_encoder);
    rmt_del_encoder(ws2812b_encoder->copy_encoder);
    free(ws2812b_encoder);
    return ESP_OK;
}

esp_err_t encoder_reset(rmt_encoder_t* rmt_encoder) {
    encoder_t* ws2812b_encoder = __containerof(rmt_encoder, encoder_t, base);
    rmt_encoder_reset(ws2812b_encoder->bytes_encoder);
    rmt_encoder_reset(ws2812b_encoder->copy_encoder);
    ws2812b_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

esp_err_t ws2812b_new_encoder(rmt_encoder_handle_t* ret_encoder) {
    encoder_t* ws2812b_encoder = (encoder_t*)rmt_alloc_encoder_mem(sizeof(encoder_t));
    if(!ws2812b_encoder) {
        return ESP_ERR_NO_MEM;
    }

    ws2812b_encoder->base.encode = encode;
    ws2812b_encoder->base.del = del_encoder;
    ws2812b_encoder->base.reset = encoder_reset;

    rmt_bytes_encoder_config_t rmt_bytes_encoder_config = RMT_BYTES_ENCODER_CONFIG_DEFAULT();
    rmt_new_bytes_encoder(&rmt_bytes_encoder_config, &ws2812b_encoder->bytes_encoder);

    rmt_copy_encoder_config_t rmt_copy_encoder_config = {};
    rmt_new_copy_encoder(&rmt_copy_encoder_config, &ws2812b_encoder->copy_encoder);

    ws2812b_encoder->reset_code = WS2812B_RESET_CODE_DEFAULT();

    *ret_encoder = &ws2812b_encoder->base;
    return ESP_OK;
}

esp_err_t ws2812b_new_channel(gpio_num_t rmt_gpio, rmt_channel_handle_t* ret_channel) {
    rmt_tx_channel_config_t rmt_tx_channel_config = {
        .gpio_num = rmt_gpio,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = WS2812B_RESOLUTION,
        .mem_block_symbols = 64,
        .trans_queue_depth = 1,
    };
    rmt_new_tx_channel(&rmt_tx_channel_config, ret_channel);
    rmt_enable((*ret_channel));
    return ESP_OK;
}

esp_err_t ws2812b_config(led_config_t* led_config, ws2812b_handle_t* ws2812b) {
    ws2812b->led_count = led_config->led_count;
    ws2812b_new_channel(led_config->rmt_gpio, &ws2812b->rmt_channel);
    ws2812b_new_encoder(&ws2812b->rmt_encoder);
    ws2812b->rmt_activate = true;
    return ESP_OK;
}

esp_err_t ws2812b_write(const uint8_t* buffer, const size_t buffer_size, ws2812b_handle_t* ws2812b) {
    rmt_transmit(ws2812b->rmt_channel, ws2812b->rmt_encoder, buffer, buffer_size, &rmt_tx_config);
    return ESP_OK;
}

esp_err_t ws2812b_del(ws2812b_handle_t* ws2812b) {
    rmt_tx_wait_all_done(ws2812b->rmt_channel, RMT_TIMEOUT_MS);

    if(ws2812b->rmt_encoder) {
        if(ws2812b->rmt_encoder->del) {
            (void)ws2812b->rmt_encoder->del(ws2812b->rmt_encoder);
        }
        ws2812b->rmt_encoder = NULL;
    }

    rmt_disable(ws2812b->rmt_channel);
    rmt_del_channel(ws2812b->rmt_channel);

    return ESP_OK;
}

// esp_err_t ws2812b_write_rgb(uint8_t const r, uint8_t const g, uint8_t const b, ws2812b_handle_t* ws2812b) {
//     for(int i = 0; i < ws2812b->led_count; i++) {
//         ws2812b->cmd[3 * i] = g;
//         ws2812b->cmd[3 * i + 1] = r;
//         ws2812b->cmd[3 * i + 2] = b;
//     }
//     ws2812b_write(ws2812b->cmd, ws2812b->cmd_size, ws2812b);
//     return ESP_OK;
// }
