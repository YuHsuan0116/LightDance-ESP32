#pragma once

#include "pca9955driver.hpp"
#include "ws2812driver.hpp"

class ChannelHandle {
  public:
    /**
     * @brief Configure this channel for either WS2812 (strip) or PCA9955 (open-drain) output.
     * @param[in] config LED configuration; uses @c config.type to select backend.
     * @return ESP_OK on success; otherwise an esp_err_t from the selected backend.
     */
    esp_err_t config(const led_config_t config);

    /**
     * @brief Write color data to the configured backend.
     * @param[in] colors Pointer to color array; interpretation depends on backend.
     * @return ESP_OK on success; otherwise an esp_err_t from the backend.
     */
    esp_err_t write(const color_t* colors);

    /**
     * @brief Detach and release resources from the configured backend.
     * @return ESP_OK on success; otherwise an esp_err_t from the backend.
     */
    esp_err_t detach();

    /**
     * @brief Block until the backend finishes pending transmissions.
     * @return ESP_OK on success; otherwise an esp_err_t from the backend.
     */
    esp_err_t wait_done();

    /**
     * @brief Set the GRB color for the current LED channel.
     *
     * Calls the corresponding driver (WS2812 or PCA9955) based on channel type.
     *
     * @param[in] GRB  Target color value in GRB order.
     * @return ESP_OK on success; otherwise an esp_err_t from the underlying driver.
     */
    esp_err_t set_GRB(const color_t& GRB);

  private:
    LED_TYPE_t type;       /**< Selected backend type. */
    pca9955Driver pca9955; /**< PCA9955 backend driver. */
    ws2812Driver ws2812;   /**< WS2812 backend driver. */
};