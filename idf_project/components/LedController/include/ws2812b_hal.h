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

    uint16_t pixel_num; /*!< Number of pixels in the LED strip */
    uint8_t* buffer;    /*!< Pointer to pixel color data in GRB order */
} ws2812b_dev_t;

/**
 * @brief Opaque handle to WS2812B device instance.
 */
typedef ws2812b_dev_t* ws2812b_handle_t;

/**
 * @brief Initialize a WS2812B LED strip device using RMT.
 *
 * Allocates a contiguous pixel buffer (3 bytes per pixel), creates a composite
 * RMT encoder (bit timing + reset symbol), initializes an RMT TX channel, and
 * enables the channel for real-time LED frame output.
 *
 * The device instance must be released via ws2812b_del().
 *
 * @param gpio_num    GPIO pin for WS2812B LED data line
 * @param pixel_num   Number of pixels in the LED strip (must be > 0)
 * @param ws2812b    Pointer to receive created WS2812B device handle
 * @return ESP_OK on success, or memory/RMT error code
 */
esp_err_t ws2812b_init(gpio_num_t gpio_num, uint16_t pixel_num, ws2812b_handle_t* ws2812b);

/**
 * @brief Set a pixel color in the WS2812B strip buffer (GRB memory order).
 *
 * Maps the provided RGB values into GRB byte order at the target pixel offset.
 * Caller must ensure 0 ≤ pixel_idx < ws2812b->pixel_num.
 *
 * @param ws2812b    WS2812B device handle
 * @param pixel_idx  Pixel index in the strip
 * @param red        8-bit red intensity
 * @param green      8-bit green intensity
 * @param blue       8-bit blue intensity
 * @return ESP_OK
 */
esp_err_t ws2812b_set_pixel(ws2812b_handle_t ws2812b, int pixel_idx, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Transmit the full WS2812B pixel buffer to the LED strip using RMT.
 *
 * Validates the device handle and ensures both RMT channel and encoder are ready,
 * then triggers a non-blocking RMT TX transaction.
 *
 * @param ws2812b  WS2812B device handle
 * @return ESP_OK, or RMT transmit error code
 */
esp_err_t ws2812b_show(ws2812b_handle_t ws2812b);

/**
 * @brief Deinitialize and free a WS2812B device and release its RMT resources.
 *
 * If the RMT channel is active, the LED buffer is cleared and flushed to the strip
 * before shutdown. The function then waits for pending transmissions, disables and
 * deletes the RMT channel, deletes the encoder, frees the pixel buffer, and finally
 * frees the device container. The caller handle is set to NULL on exit.
 *
 * Safe cleanup order: flush → wait TX done → disable → delete channel/encoder → free heap.
 *
 * @param ws2812b  Pointer to WS2812B device handle to delete
 * @return ESP_OK, or invalid argument if handle is NULL
 */
esp_err_t ws2812b_del(ws2812b_handle_t* ws2812b);

esp_err_t ws2812b_write(ws2812b_handle_t ws2812b, uint8_t* _buffer);
void ws2812b_fill(ws2812b_handle_t ws2812b, uint8_t red, uint8_t green, uint8_t blue);
esp_err_t ws2812b_wait_done(ws2812b_handle_t ws2812b);

void ws2812b_print_buffer(ws2812b_handle_t ws2812b);

void ws2812b_test();

#ifdef __cplusplus
}
#endif