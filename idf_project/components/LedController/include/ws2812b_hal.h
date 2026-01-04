#pragma once

#include "driver/gpio.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"

#include "BoardConfig.h"
#include "ws2812b_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WS2812B LED strip device descriptor.
 *
 * Holds the RMT TX channel, RMT encoder handle, total pixel count, and a pointer
 * to the contiguous pixel color buffer used for frame output.
 *
 * The pixel buffer is dynamically allocated and must be freed by the caller.
 */
typedef struct {
    rmt_channel_handle_t rmt_channel; /*!< RMT TX channel handle for LED signal output */
    rmt_encoder_handle_t rmt_encoder; /*!< RMT encoder handle (bit timing + reset symbol) */

    gpio_num_t gpio_num; /*!< Number of the gpio pin */
    uint16_t pixel_num;  /*!< Number of pixels in the LED strip */
    uint8_t* buffer;     /*!< Pointer to pixel color data in GRB order */
} ws2812b_dev_t;

/**
 * @brief Opaque handle to WS2812B device instance.
 */
typedef ws2812b_dev_t* ws2812b_handle_t;

/**
 * @brief Allocates and initializes the WS2812B driver handle.
 * * @param[in]  gpio_num   GPIO pin for the data signal.
 * @param[in]  pixel_num  Total number of LEDs in the strip.
 * @param[out] ws2812b    Pointer to the handle to be initialized.
 * * @return
 * - ESP_OK: Success.
 * - ESP_ERR_NO_MEM: Allocation failed.
 * - ESP_ERR_INVALID_ARG: Invalid arguments.
 */
esp_err_t ws2812b_init(gpio_num_t gpio_num, uint16_t pixel_num, ws2812b_handle_t* ws2812b);

/**
 * @brief Sets the RGB color for a specific pixel in the buffer.
 *
 * @param[in] ws2812b    Driver handle.
 * @param[in] pixel_idx  Index of the pixel (0 to pixel_num - 1).
 * @param[in] red        Red intensity (0-255).
 * @param[in] green      Green intensity (0-255).
 * @param[in] blue       Blue intensity (0-255).
 *
 * @return
 * - ESP_OK: Success.
 * - ESP_ERR_INVALID_ARG: Handle is NULL or index out of bounds.
 */
esp_err_t ws2812b_set_pixel(ws2812b_handle_t ws2812b, int pixel_idx, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Transmits the internal buffer data to the LED strip.
 *
 * @param[in] ws2812b  Driver handle.
 *
 * @return
 * - ESP_OK: Transmission queued successfully.
 * - ESP_ERR_INVALID_STATE: RMT driver not initialized or queue is full.
 * - ESP_ERR_INVALID_ARG: Handle is NULL.
 */
esp_err_t ws2812b_show(ws2812b_handle_t ws2812b);

/**
 * @brief Deallocates the WS2812B driver and releases all resources.
 * * @note This function attempts to turn off the LEDs before deletion.
 * Errors during the cleanup process are logged but do not stop execution
 * to ensure memory is always freed.
 *
 * @param[in,out] ws2812b  Double pointer to the handle. Will be set to NULL on success.
 *
 * @return
 * - ESP_OK: Success.
 * - ESP_ERR_INVALID_ARG: Handle is null.
 */
esp_err_t ws2812b_del(ws2812b_handle_t* ws2812b);

/**
 * @brief Bulk updates the internal LED color buffer from an external source.
 * * @note This function only updates the data in memory. It does NOT send the
 * signal to the LEDs immediately.
 *
 * @param[in] ws2812b   Driver handle.
 * @param[in] src_data  Pointer to the source byte array (Must be pixel_num * 3 bytes).
 *
 * @return
 * - ESP_OK: Success.
 * - ESP_ERR_INVALID_ARG: Null pointer detected.
 */
esp_err_t ws2812b_write(ws2812b_handle_t ws2812b, uint8_t* _buffer);

/**
 * @brief Fills the entire LED strip with a single color.
 *
 * @param[in] ws2812b  Driver handle.
 * @param[in] red      Red intensity (0-255).
 * @param[in] green    Green intensity (0-255).
 * @param[in] blue     Blue intensity (0-255).
 *
 * @return
 * - ESP_OK: Success.
 * - ESP_ERR_INVALID_ARG: Handle is NULL.
 */
esp_err_t ws2812b_fill(ws2812b_handle_t ws2812b, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Blocks until the RMT transmission is finished.
 *
 * @param[in] ws2812b  Driver handle.
 *
 * @return
 * - ESP_OK: Transmission finished.
 * - ESP_ERR_TIMEOUT: Wait timed out (LEDs might still be updating).
 * - ESP_ERR_INVALID_ARG: Handle is NULL.
 */
esp_err_t ws2812b_wait_done(ws2812b_handle_t ws2812b);

/**
 * @brief Dumps the current raw buffer content to the console in Hex format.
 *
 * @note The buffer stores data in GRB order (Green, Red, Blue) corresponding
 * to the physical wire protocol, not the logical RGB order.
 * * @param[in] ws2812b  Driver handle.
 *
 * @return
 * - ESP_OK: Success.
 * - ESP_ERR_INVALID_ARG: Handle is NULL.
 */
esp_err_t ws2812b_print_buffer(ws2812b_handle_t ws2812b);

esp_err_t ws2812b_get_pixel(ws2812b_handle_t ws2812b, int pixel_idx, uint8_t* red, uint8_t* green, uint8_t* blue);

void ws2812b_test();
void ws2812b_test2();

#ifdef __cplusplus
}
#endif