#pragma once

#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "esp_attr.h"
#include "led_def.h"

/** @brief WS2812B encoder clock resolution in Hz (10 MHz). */
#define WS2812_RESOLUTION 10000000

/**
 * @brief Configuration parameters for the WS2812B RMT encoder.
 *
 * Used to specify timing resolution when creating a new encoder instance.
 */
typedef struct {
    uint32_t resolution;
} encoder_config_t;

/**
 * @brief WS2812B RMT encoder internal context.
 *
 * Holds the base encoder interface and sub-encoders for encoding
 * WS2812B data and reset pulses.
 */
typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t* bytes_encoder;
    rmt_encoder_t* copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} encoder_t;

/**
 * @brief Create and initialize a new WS2812B-compatible RMT encoder.
 *
 * Configures an RMT encoder to generate the WS2812B signal pattern,
 * including bit timing for '0', '1', and the reset pulse.
 *
 * @param[out] ret_encoder Pointer to store the created encoder handle.
 *
 * @return
 *  - ESP_OK: Encoder created successfully.
 *  - ESP_ERR_NO_MEM: Memory allocation failed.
 *  - Other error codes from RMT encoder creation functions.
 */
esp_err_t new_encoder(rmt_encoder_handle_t* ret_encoder);

/**
 * @brief Create and enable a new RMT TX channel for WS2812B.
 *
 * Initializes an RMT TX channel with the given GPIO pin, configured
 * to drive WS2812B LEDs at the specified resolution and memory settings.
 *
 * @param[in]  gpio         GPIO number to output the RMT signal.
 * @param[out] ret_channel  Pointer to store the created RMT TX channel handle.
 *
 * @return
 *  - ESP_OK: Channel created and enabled successfully.
 *  - Error codes from rmt_new_tx_channel() or rmt_enable().
 */
esp_err_t new_channel(int gpio, rmt_channel_handle_t* ret_channel);

/**
 * @brief Encode WS2812B RGB data and reset pulse using RMT.
 *
 * This function implements the custom RMT encoder for WS2812B LEDs.
 * It first encodes the RGB data stream, then appends the required
 * reset pulse. The function may be called multiple times until the
 * encoding is complete or the RMT memory is full.
 *
 * @param[in]  encoder     Pointer to the base RMT encoder object.
 * @param[in]  channel     RMT TX channel handle where data will be sent.
 * @param[in]  data        Pointer to RGB data to encode.
 * @param[in]  data_size   Size of the RGB data in bytes.
 * @param[out] ret_state   Pointer to store the current encoding state flags.
 *
 * @return
 *  The number of RMT symbols encoded in this call.
 *  May be less than the total needed if RMT memory is full.
 */
size_t encode(rmt_encoder_t* encoder, rmt_channel_handle_t channel, const void* data, size_t data_size, rmt_encode_state_t* ret_state);

/**
 * @brief Delete a WS2812B RMT encoder and free its resources.
 *
 * This function releases the byte and copy encoders associated with
 * the given WS2812B encoder, then frees the encoder memory itself.
 *
 * @param[in] encoder Pointer to the base RMT encoder to delete.
 *
 * @return
 *  - ESP_OK: Encoder deleted successfully.
 */
esp_err_t del_encoder(rmt_encoder_t* encoder);

/**
 * @brief Reset the WS2812B RMT encoder state.
 *
 * Resets both the byte and copy encoders associated with the WS2812B encoder
 * and sets the encoder state back to @ref RMT_ENCODING_RESET.
 *
 * @param[in] encoder Pointer to the base RMT encoder to reset.
 *
 * @return
 *  - ESP_OK: Encoder reset successfully.
 */
esp_err_t encoder_reset(rmt_encoder_t* encoder);