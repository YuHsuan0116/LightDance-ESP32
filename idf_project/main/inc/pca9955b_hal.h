#pragma once

#include "def.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_TIMEOUT_MS 100
#define I2C_SCL_FREQ 100000

#define PCA9955B_MAXIMUM_COUNT 8
#define PCA9955B_IREFALL_ADDR 0x45

/**
 * @brief PCA9955B channel handle
 */
typedef struct {
    bool need_reset_IREF;                   /*!< Whether this channel needs IREF reset before next update */
    uint8_t i2c_addr;                       /*!< I2C address of the PCA9955B device */
    uint8_t pca_channel;                    /*!< Channel index (0–14) inside the PCA9955B */
    uint8_t reg_addr;                       /*!< Register address for  PWM/ */
    int8_t dev_list_idx;                    /*!< Index in the device list */
    i2c_master_dev_handle_t i2c_dev_handle; /*!< I2C device handle */
} pca9955b_handle_t;

/**
 * @brief Entry for an I2C device handle shared by multiple pca9955b handles
 */
typedef struct {
    uint8_t addr;                           /*!< 7-bit I2C device address */
    i2c_master_dev_handle_t i2c_dev_handle; /*!< I2C device handle */
    uint8_t used_count;                     /*!< Number of active users referencing this dev handle */
    bool used[5];                           /*!< Usage flags for up to 5 handles */
} i2c_dev_entry_t;

/**
 * @brief Find the index of an existing I2C device entry by address.
 *
 * @param[in] addr   7-bit I2C device address to search for.
 * @return
 *      - >= 0 : Index of the matching device entry.
 *      - -1   : Device not found.
 */
int8_t find_i2c_dev_list_idx(uint8_t addr);

/**
 * @brief Add a PCA9955B handle to the I2C device list.
 *
 * @param[in,out] pca9955b  PCA9955B handle.
 *
 * @return
 *      - ESP_OK: Added successfully.
 *      - ESP_ERR_INVALID_ARG: Null handle or channel already in use.
 *      - ESP_ERR_NO_MEM: No free device list entry available.
 *      - Other error codes: Propagated from i2c_dev_list_add().
 */
esp_err_t pca9955b_add_i2c_dev_list(pca9955b_handle_t* pca9955);

/**
 * @brief Add an I2C device to the device list.
 *
 * @param[in] i2c_addr          7-bit I2C device address.
 * @param[in] i2c_dev_list_idx  Index of the device list entry to fill.
 *
 * @return
 *      - ESP_OK: Device added successfully.
 *      - ESP_ERR_INVALID_ARG: Invalid list index.
 *      - ESP_ERR_INVALID_STATE: I2C bus not initialized, or device handle is NULL.
 *      - ESP_FAIL / other: Error returned by underlying I2C driver.
 */
esp_err_t i2c_dev_list_add(uint8_t i2c_addr, int8_t i2c_dev_list_idx);

/**
 * @brief Initialize an I2C master bus on I2C_NUM_0.
 *
 * @param[in]  i2c_gpio_sda        GPIO number for I2C SDA.
 * @param[in]  i2c_gpio_scl        GPIO number for I2C SCL.
 * @param[out] ret_i2c_bus_handle  Returned I2C master bus handle.
 *
 * @return
 *      - ESP_OK: Bus initialized successfully.
 *      - ESP_ERR_INVALID_ARG: SDA/SCL pins invalid or identical.
 *      - ESP_FAIL / others: Error returned by i2c_new_master_bus().
 */
esp_err_t i2c_bus_init(gpio_num_t i2c_gpio_sda, gpio_num_t i2c_gpio_scl, i2c_master_bus_handle_t* ret_i2c_bus_handle);

/**
 * @brief Write a buffer to an I2C device.
 *
 * @param[in] i2c_dev_handle  I2C device handle.
 * @param[in] buffer          Pointer to data buffer to send.
 * @param[in] buffer_size     Size of the buffer in bytes.
 * @param[in] i2c_timeout_ms  I2C transfer timeout in milliseconds.
 *
 * @return
 *      - ESP_OK: Write succeeded.
 *      - ESP_ERR_INVALID_ARG: Invalid handle, buffer, or size.
 *      - Other error codes returned by i2c_master_transmit().
 */

esp_err_t i2c_write_hal(i2c_master_dev_handle_t i2c_dev_handle, uint8_t* const buffer, size_t const buffer_size, size_t const i2c_timeout_ms);

/**
 * @brief Write the IREFALL register of a PCA9955B device.
 *
 * @param[in] IREF_val   IREFALL value to write.
 * @param[in] pca9955b   PCA9955B channel handle.
 *
 * @return
 *      - ESP_OK: Register written successfully.
 *      - ESP_ERR_INVALID_ARG: Null handle.
 *      - ESP_ERR_INVALID_STATE: Invalid or uninitialized I2C device handle.
 *      - Other: Error returned by i2c_write_hal().
 */
esp_err_t pca9955b_write_IREFALL(uint8_t IREF_val, pca9955b_handle_t* pca9955b);

/**
 * @brief Write RGB PWM values to a PCA9955B channel.
 *
 * @param[in] r         Red   PWM value.
 * @param[in] g         Green PWM value.
 * @param[in] b         Blue  PWM value.
 * @param[in] pca9955b  PCA9955B channel handle.
 *
 * @return
 *      - ESP_OK: RGB values written successfully.
 *      - ESP_ERR_INVALID_ARG: Null handle or invalid channel index.
 *      - ESP_ERR_INVALID_STATE: I2C device handle not initialized.
 *      - Other: Error returned by I2C operations.
 */
esp_err_t pca9955b_write_rgb(uint8_t const r, uint8_t const g, uint8_t const b, pca9955b_handle_t* pca9955b);

/**
 * @brief Configure a PCA9955B channel handle.
 *
 * @param[in]  led_config  LED configuration parameters.
 * @param[out] pca9955b    PCA9955B channel handle to initialize.
 *
 * @return
 *      - ESP_OK: Configuration completed successfully.
 *      - ESP_ERR_INVALID_ARG: Null pointer or invalid channel index.
 *      - Other: Errors returned by pca9955b_add_i2c_dev_list().
 */
esp_err_t pca9955b_config(led_config_t* led_config, pca9955b_handle_t* pca9955b);

/**
 * @brief Delete a PCA9955B channel handle and release device resources.
 *
 * @param[in,out] pca9955b  PCA9955B handle.
 *
 * @return
 *      - ESP_OK: Operation completed (always returns ESP_OK).
 */
esp_err_t pca9955b_del(pca9955b_handle_t* pca9955b);

#ifdef __cplusplus
}
#endif