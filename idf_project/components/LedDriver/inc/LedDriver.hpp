#pragma once

#include "channelhandle.hpp"

/**
 * @brief High-level LED driver coordinating multiple channels.
 *
 * Owns an I2C bus handle and dispatches per-channel operations
 * to ChannelHandle backends (WS2812 strip or PCA9955).
 */
class LedDriver {
  public:
    /** @brief Construct a LedDriver (bus is initialized in ctor). */
    LedDriver();

    /** @brief Destruct the LedDriver and release resources (best effort). */
    ~LedDriver();

    /**
     * @brief Configure N channels with the provided configurations.
     * @param[in] configs Pointer to an array of led_config_t of size @_ch_num.
     * @param[in] _ch_num Number of channels to configure (0..MAX_CHANNEL_NUM).
     * @return ESP_OK on success; otherwise an esp_err_t.
     */
    esp_err_t config(const led_config_t* configs, const int _ch_num);

    /**
     * @brief Write a frame (per-channel color buffers).
     * @param[in] frame Pointer to an array of @c color_t* of size @c ch_num.
     *              Each entry points to the channel's color buffer.
     * @return ESP_OK on success; otherwise an esp_err_t.
     */
    esp_err_t write(const color_t** frame);

    /**
     * @brief Write a test pattern to a single channel and wait for completion.
     * @param[in] ch_idx Channel index (0-based).
     * @param[in] colors Pointer to the channel's color buffer.
     * @return ESP_OK on success; otherwise an esp_err_t.
     */
    esp_err_t part_test(const int ch_idx, const color_t* colors);

    /**
     * @brief Detach all configured channels and release their resources.
     * @return ESP_OK on success; otherwise an esp_err_t.
     */
    esp_err_t reset();

    /**
     * @brief Clear all channels to zero color (best effort).
     * @return ESP_OK on success; otherwise an esp_err_t.
     */
    esp_err_t clear_frame();

    /**
     * @brief Block until all channels finish pending transmissions.
     * @return ESP_OK on success; otherwise an esp_err_t.
     */
    esp_err_t wait_all_done();

    /**
     * @brief Set the GRB color for all LED channels.
     *
     * Propagates the specified color to each channel handle.
     *
     * @param[in] GRB  Target color value in GRB order.
     * @return ESP_OK if all channels succeed; otherwise the last error encountered.
     */
    esp_err_t set_GRB(const color_t& GRB);

  private:
    int ch_num;                                           /**< Number of configured channels, -1 if none. */
    i2c_master_bus_handle_t bus_handle;                   /**< I2C master bus handle. */
    static ChannelHandle channel_handle[MAX_CHANNEL_NUM]; /**< Shared per-channel backends. */
};