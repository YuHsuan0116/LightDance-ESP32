#include "ws2812.h"

static const char *TAG = "ws2812";

esp_err_t new_encoder(rmt_encoder_handle_t *ret_encoder) {
  encoder_config_t config = {
      .resolution = WS2812_RESOLUTION,
  };

  encoder_t *encoder = (encoder_t *)rmt_alloc_encoder_mem(sizeof(encoder_t));
  if (!encoder) {
    return ESP_ERR_NO_MEM; // Allocation failed
  }

  encoder->base.encode = encode;
  encoder->base.del = del_encoder;
  encoder->base.reset = encoder_reset;

  rmt_bytes_encoder_config_t bytes_encoder_config = {
      .bit0 =
          {
              .level0 = 1,
              .duration0 = 0.4 * config.resolution / 1000000,
              .level1 = 0,
              .duration1 = 0.85 * config.resolution / 1000000,
          },
      .bit1 =
          {
              .level0 = 1,
              .duration0 = 0.8 * config.resolution / 1000000,
              .level1 = 0,
              .duration1 = 0.45 * config.resolution / 1000000,
          },
      .flags.msb_first = 1,
  };
  rmt_new_bytes_encoder(&bytes_encoder_config, &encoder->bytes_encoder);

  rmt_copy_encoder_config_t copy_encoder_config = {};
  rmt_new_copy_encoder(&copy_encoder_config, &encoder->copy_encoder);

  uint32_t reset_ticks = config.resolution / 1000000 * 50 / 2;
  encoder->reset_code = (rmt_symbol_word_t){
      .level0 = 0,
      .duration0 = reset_ticks,
      .level1 = 0,
      .duration1 = reset_ticks,
  };

  *ret_encoder = &encoder->base;
  return ESP_OK;
}

esp_err_t new_channel(int gpio, rmt_channel_handle_t *ret_channel) {
  rmt_tx_channel_config_t tx_chan_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .gpio_num = gpio,
      .mem_block_symbols = 64,
      .resolution_hz = WS2812_RESOLUTION,
      .trans_queue_depth = 1,
  };
  ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&tx_chan_config, ret_channel), TAG,
                      "new_tx_channel failed");
  ESP_RETURN_ON_ERROR(rmt_enable(*ret_channel), TAG, "enable failed");
  return ESP_OK;
}

size_t IRAM_ATTR encode(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                        const void *data, size_t data_size,
                        rmt_encode_state_t *ret_state) {
  encoder_t *ws2812_encoder = __containerof(encoder, encoder_t, base);
  rmt_encoder_handle_t bytes_encoder = ws2812_encoder->bytes_encoder;
  rmt_encoder_handle_t copy_encoder = ws2812_encoder->copy_encoder;
  rmt_encode_state_t session_state = RMT_ENCODING_RESET;
  rmt_encode_state_t state = RMT_ENCODING_RESET;
  size_t encoded_symbols = 0;
  switch (ws2812_encoder->state) {
  case 0: // send RGB data
    encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, data,
                                             data_size, &session_state);
    if (session_state & RMT_ENCODING_COMPLETE) {
      ws2812_encoder->state = 1;
    }
    if (session_state & RMT_ENCODING_MEM_FULL) {
      state |= RMT_ENCODING_MEM_FULL;
      break; // stop encoding, skip reset code
    }
    /* fall through */
  case 1: // send reset code
    encoded_symbols += copy_encoder->encode(
        copy_encoder, channel, &ws2812_encoder->reset_code,
        sizeof(ws2812_encoder->reset_code), &session_state);
    if (session_state & RMT_ENCODING_COMPLETE) {
      ws2812_encoder->state = RMT_ENCODING_RESET;
      state |= RMT_ENCODING_COMPLETE;
    }
    if (session_state & RMT_ENCODING_MEM_FULL) {
      state |= RMT_ENCODING_MEM_FULL;
    }
    break;
  }

  *ret_state = state;
  return encoded_symbols;
}

esp_err_t del_encoder(rmt_encoder_t *encoder) {
  encoder_t *ws2812_encoder = __containerof(encoder, encoder_t, base);
  rmt_del_encoder(ws2812_encoder->bytes_encoder);
  rmt_del_encoder(ws2812_encoder->copy_encoder);
  free(ws2812_encoder);
  return ESP_OK;
}

esp_err_t encoder_reset(rmt_encoder_t *encoder) {
  encoder_t *ws2812_encoder = __containerof(encoder, encoder_t, base);
  rmt_encoder_reset(ws2812_encoder->bytes_encoder);
  rmt_encoder_reset(ws2812_encoder->copy_encoder);
  ws2812_encoder->state = RMT_ENCODING_RESET;
  return ESP_OK;
}