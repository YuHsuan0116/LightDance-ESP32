#pragma once

extern "C" {
#include "ws2812.h"
}

class ws2812Driver {
  public:
    /** @brief Construct a WS2812 driver object (RMT inactive by default). */
    ws2812Driver();

    /**
     * @brief Configure RMT channel and encoder for WS2812 output.
     * @param[in] config LED configuration; uses gpio_or_addr as output GPIO and led_count as number of LEDs.
     * @return ESP_OK on success; otherwise an esp_err_t from RMT/encoder creation.
     */
    esp_err_t config(const led_config_t config);

    /**
     * @brief Transmit WS2812 color data using the configured RMT channel.
     * @param[in] colors Pointer to an array of @c color_t with at least @c led_count elements.
     * @return ESP_OK on success; otherwise an esp_err_t from RMT transmit.
     */
    esp_err_t write(const color_t* colors);

    /**
     * @brief Disable and delete the RMT channel if active; release resources.
     * @return ESP_OK on success; otherwise an esp_err_t from RMT deletion.
     */
    esp_err_t detach();

    /**
     * @brief Block until all pending RMT transmissions are complete.
     * @return ESP_OK on success; otherwise an esp_err_t from RMT wait.
     */
    esp_err_t wait_done();

    /**
     * @brief Set all WS2812 LEDs to the specified GRB color.
     * @param[in] GRB  The color value to be applied to all LEDs.
     * @return
     *     - ESP_OK on success
     *     - ESP_FAIL or other esp_err_t from lower-level write/wait_done functions
     */
    esp_err_t set_GRB(const color_t& GRB);

  private:
    bool rmt_activate;            /**< Whether the RMT channel is active. */
    uint8_t led_count;            /**< Number of LEDs to transmit. */
    rmt_channel_handle_t channel; /**< RMT TX channel handle. */
    rmt_encoder_handle_t encoder; /**< WS2812B encoder handle. */
};