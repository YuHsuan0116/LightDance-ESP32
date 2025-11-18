#pragma once

#include "def.h"
#include "pca9955b_hal.h"
#include "ws2812b_hal.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Handle for a single LED channel.
 */
typedef struct {
    led_type_t type;            /*!< LED driver type */
    pca9955b_handle_t pca9955b; /*!< PCA9955B channel handle */
    ws2812b_handle_t ws2812b;   /*!< WS2812B strip handle */
} channel_handle_t;

/**
 * @brief Configure a channel handle .
 *
 * @param[in]  led_config     LED configuration parameters.
 * @param[out] channel_handle Channel handle to initialize.
 *
 * @return
 *      - ESP_OK: Configuration completed successfully.
 *      - ESP_ERR_INVALID_ARG: Null pointers or invalid LED type.
 *      - Other: Errors returned by the selected driver configuration function.
 */
esp_err_t channel_handle_config(led_config_t* led_config, channel_handle_t* channel_handle);

/**
 * @brief Write color data to a channel handle.
 *
 * @param[in] colors          Pointer to color array.
 * @param[in] channel_handle  Target channel handle.
 *
 * @return
 *      - ESP_OK: Write completed successfully.
 *      - ESP_ERR_INVALID_ARG: Null pointers.
 *      - ESP_ERR_NOT_SUPPORTED: Unsupported channel type.
 *      - Other: Errors returned by the underlying driver write function.
 */
esp_err_t channel_handle_write(color_t* colors, channel_handle_t* channel_handle);

/**
 * @brief Delete a channel handle and release its associated resources.
 *
 * @param[in,out] channel_handle  Channel handle to delete.
 *
 * @return
 *      - ESP_OK: Operation completed successfully.
 *      - ESP_ERR_INVALID_ARG: Null channel handle.
 *      - ESP_ERR_NOT_SUPPORTED: Unsupported channel type.
 *      - Other: Errors returned by the underlying driver delete function.
 */
esp_err_t channel_handle_del(channel_handle_t* channel_handle);

#ifdef __cplusplus
}
#endif