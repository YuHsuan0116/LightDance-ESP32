#pragma once

#include "channel_handle.h"

/**
 * @brief Maximum number of LED channels supported by the driver.
 */
#define LED_DRIVER_MAX_CHANNEL_COUNT 48

/**
 * @brief Default GPIO used for I2C SDA.
 */
#define I2C_GPIO_SDA GPIO_NUM_21

/**
 * @brief Default GPIO used for I2C SCL.
 */
#define I2C_GPIO_SCL GPIO_NUM_22

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handle for the LED driver managing multiple channels.
 */
typedef struct {
    bool initialized;                                              /*!< Whether the driver is initialized */
    int channel_number;                                            /*!< Number of configured channels */
    i2c_master_bus_handle_t bus_handle;                            /*!< I2C master bus handle */
    channel_handle_t channel_handle[LED_DRIVER_MAX_CHANNEL_COUNT]; /*!< channel driver handles */
} LedDriver_handle_t;

/**
 * @brief Initialize the I2C bus for the LED driver.
 *
 * @param[in,out] LedDriver  LED driver handle.
 *
 * @return
 *      - ESP_OK: Bus initialized successfully.
 *      - ESP_ERR_INVALID_ARG: Null driver handle.
 *      - Others: Errors returned by i2c_bus_init().
 */
esp_err_t LedDriver_init_i2c(LedDriver_handle_t* LedDriver);

/**
 * @brief Delete the I2C bus used by the LED driver.
 *
 * @param[in,out] LedDriver  LED driver handle.
 *
 * @return
 *      - ESP_OK: Bus deleted successfully or already NULL.
 *      - ESP_ERR_INVALID_ARG: Null driver handle.
 *      - Others: Errors returned by i2c_del_master_bus().
 */
esp_err_t LedDriver_del_i2c(LedDriver_handle_t* LedDriver);

/**
 * @brief Initialize the LED driver handle.
 *
 * @param[in,out] LedDriver  LED driver handle.
 *
 * @return
 *      - ESP_OK: Initialization completed successfully.
 *      - ESP_ERR_INVALID_ARG: Null driver handle.
 *      - Others: Errors returned by LedDriver_init_i2c().
 */
esp_err_t LedDriver_init(LedDriver_handle_t* LedDriver);

/**
 * @brief Configure all LED channels managed by the LED driver.
 *
 * @param[in]  led_configs   Array of LED configuration structures.
 * @param[in]  channel_number Number of channels to configure.
 * @param[in,out] LedDriver  LED driver handle.
 *
 * @return
 *      - ESP_OK: All channels configured successfully.
 *      - ESP_ERR_INVALID_ARG: Null pointers or invalid channel count.
 *      - ESP_ERR_INVALID_STATE: Driver not initialized.
 *      - Others: Errors returned by channel_handle_config().
 */
esp_err_t LedDriver_config(led_config_t* led_configs, int _channel_number, LedDriver_handle_t* LedDriver);

/**
 * @brief Delete the LED driver and release all associated resources.
 *
 * @param[in,out] LedDriver  LED driver handle to delete.
 *
 * @return
 *      - ESP_OK: Deleted successfully.
 *      - ESP_ERR_INVALID_ARG: Null driver handle.
 *      - ESP_ERR_INVALID_STATE: Driver not initialized.
 *      - Other: Last error returned by channel or bus cleanup.
 */
esp_err_t LedDriver_del(LedDriver_handle_t* LedDriver);

/**
 * @brief Write color data to all configured LED channels.
 *
 * @param[in]  colors     Array of color buffers, one per channel.
 * @param[in]  LedDriver  LED driver handle.
 *
 * @return
 *      - ESP_OK: All writes succeeded or no errors occurred.
 *      - ESP_ERR_INVALID_ARG: Null pointers.
 *      - ESP_ERR_INVALID_STATE: Driver not initialized.
 *      - Others: Last error returned by channel_handle_write().
 */
esp_err_t LedDriver_write(color_t** colors, LedDriver_handle_t* LedDriver);

/**
 * @brief Set all channels to the same RGB value.
 *
 * @param[in]  red          Red   component (0–255).
 * @param[in]  green          Green component (0–255).
 * @param[in]  blue          Blue  component (0–255).
 * @param[in]  LedDriver  LED driver handle.
 *
 * @return
 *      - ESP_OK: Operation completed successfully.
 *      - ESP_ERR_INVALID_ARG: Null driver handle.
 *      - ESP_ERR_INVALID_STATE: Driver not initialized.
 *      - Others: Last error returned by channel_handle_write().
 */
esp_err_t LedDriver_set_rgb(uint8_t red, uint8_t green, uint8_t blue, LedDriver_handle_t* LedDriver);

/**
 * @brief Set all channels to a single color value.
 *
 * @param[in]  color      Color value to apply.
 * @param[in]  LedDriver  LED driver handle.
 *
 * @return
 *      - ESP_OK: Operation completed successfully.
 *      - ESP_ERR_INVALID_ARG: Null driver handle.
 *      - ESP_ERR_INVALID_STATE: Driver not initialized.
 *      - Others: Errors returned by LedDriver_set_rgb().
 */
esp_err_t LedDriver_set_color(color_t color, LedDriver_handle_t* LedDriver);

#ifdef __cplusplus
}
#endif