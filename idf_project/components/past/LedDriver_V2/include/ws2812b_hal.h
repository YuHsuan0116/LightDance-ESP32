#pragma once

#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RMT_TIMEOUT_MS 100
#define WS2812B_RESOLUTION 10000000
#define WS2812B_MAXIMUM_LED_COUNT 100
#define WS2812B_MAXIMUM_COUNT 8

/**
 * @brief Handle for a WS2812B LED controller.
 */
typedef struct {
    bool initialized;                 /*!< Whether the WS2812B driver is initialized */
    uint8_t led_count;                /*!< Number of LEDs in the strip */
    rmt_channel_handle_t rmt_channel; /*!< RMT channel handle */
    rmt_encoder_handle_t rmt_encoder; /*!< RMT encoder handle */
} ws2812b_handle_t;

/**
 * @brief Create a new WS2812B composite RMT encoder.
 *
 * @param[out] ret_encoder  Returned encoder handle.
 *
 * @return
 *      - ESP_OK: Encoder created successfully.
 *      - ESP_ERR_INVALID_ARG: Null output pointer.
 *      - ESP_ERR_NO_MEM: Memory allocation failed.
 *      - Other: Errors returned by rmt_new_bytes_encoder() or rmt_new_copy_encoder().
 */
esp_err_t ws2812b_new_encoder(rmt_encoder_handle_t* ret_encoder);

/**
 * @brief Create a new RMT TX channel for driving WS2812B LEDs.
 *
 * @param[in]  rmt_gpio     GPIO number used for RMT output.
 * @param[out] ret_channel  Returned RMT channel handle.
 *
 * @return
 *      - ESP_OK: Channel created and enabled successfully.
 *      - ESP_ERR_INVALID_ARG: Null output pointer.
 *      - Others: Errors returned by rmt_new_tx_channel() or rmt_enable().
 */
esp_err_t ws2812b_new_channel(gpio_num_t rmt_gpio, rmt_channel_handle_t* ret_channel);

esp_err_t ws2812b_enable(ws2812b_handle_t* ws2812b);

/**
 * @brief Configure a WS2812B LED strip handle.
 *
 * @param[in]  led_config  LED configuration parameters.
 * @param[out] ws2812b     WS2812B handle to initialize.
 *
 * @return
 *      - ESP_OK: Configuration completed successfully.
 *      - ESP_ERR_INVALID_ARG: Null pointer or invalid LED count.
 *      - Other: Errors returned from channel or encoder creation.
 */
esp_err_t ws2812b_config(led_config_t* led_config, ws2812b_handle_t* ws2812b);

/**
 * @brief Transmit WS2812B LED data using the configured RMT channel and encoder.
 *
 * @param[in] buffer        Pointer to RGB data to send.
 * @param[in] buffer_size   Size of the RGB buffer in bytes.
 * @param[in] ws2812b       WS2812B handle used for transmission.
 *
 * @return
 *      - ESP_OK: Transmission completed successfully.
 *      - ESP_ERR_INVALID_ARG: Null buffer or handle.
 *      - ESP_ERR_INVALID_STATE: Handle not initialized or missing RMT objects.
 *      - Others: Errors returned by rmt_transmit().
 */
esp_err_t ws2812b_write(const uint8_t* buffer, const size_t buffer_size, ws2812b_handle_t* ws2812b);

/**
 * @brief Delete a WS2812B handle and release its RMT resources.
 *
 * @param[in,out] ws2812b  WS2812B handle to delete.
 *
 * @return
 *      - ESP_OK: Deleted successfully or already uninitialized.
 *      - ESP_ERR_INVALID_ARG: Null handle.
 *      - Other: Last error returned by RMT or encoder deletion.
 */
esp_err_t ws2812b_del(ws2812b_handle_t* ws2812b);

#ifdef __cplusplus
}
#endif