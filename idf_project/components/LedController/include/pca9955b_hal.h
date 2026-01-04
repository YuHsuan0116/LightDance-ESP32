#pragma once

#include "driver/i2c_master.h"

#include "BoardConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Packed 16-byte buffer mapping 1 Command Byte + 15 PWM channels.
 *
 * This structure is designed for I2C burst writes.
 * Memory layout (16 bytes total):
 * [0]      Command Byte (Start Register Address + Auto-Increment Flag)
 * [1..15]  Channel color data (Mapped as 5 RGB LEDs)
 *
 * @note This covers PWM0 to PWM14. PWM15 is unused in this layout.
 */
typedef struct __attribute__((packed)) {
    uint8_t command_byte; /*!< I2C Command byte: Register Address | Auto-Increment Bit */
    union {
        uint8_t data[15]; /*!< Raw access to 15-byte channel color payload */
        struct __attribute__((packed)) {
            uint8_t ch[5][3]; /*!< logical mapping: 5 LEDs x 3 channels (R, G, B) */
        };
    };
} pca9955b_buffer_t;

/**
 * @brief PCA9955B device descriptor for I2C-based constant-current LED driver control.
 *
 * Holds bus handle, device address, LED frame buffer, and IREF reset command.
 */
typedef struct {
    i2c_master_dev_handle_t i2c_dev_handle; /*!< I2C bus device handle */
    uint8_t i2c_addr;                       /*!< 7-bit I2C device address */

    pca9955b_buffer_t buffer; /*!< PWM register + LED color buffer */
    bool need_update;         /*!< Dirty flag for buffer */

    bool need_reset_IREF; /*!< Set true if IREF register needs to be reinitialized */
    uint8_t IREF_cmd[2];  /*!< 2-byte IREF reset command to send over I2C */
} pca9955b_dev_t;

/**
 * @brief Opaque handle to PCA9955B device instance.
 */
typedef pca9955b_dev_t* pca9955b_handle_t;

/**
 * @brief Initializes the I2C master bus.
 *
 * This function configures the I2C controller in master mode.
 *
 * @note The internal pull-ups are enabled, but they are weak (~45kOhm).
 * For 400kHz operation or long wires, external pull-ups (e.g., 2.2k - 4.7k)
 * are strongly recommended.
 *
 * @param[in]  i2c_gpio_sda       GPIO number for SDA line.
 * @param[in]  i2c_gpio_scl       GPIO number for SCL line.
 * @param[out] ret_i2c_bus_handle Pointer to store the created I2C bus handle.
 *
 * @return
 * - ESP_OK: Success.
 * - ESP_ERR_INVALID_ARG: SDA/SCL pins are the same, or handle pointer is NULL.
 * - ESP_FAIL: Driver installation failed.
 */
esp_err_t i2c_bus_init(gpio_num_t i2c_gpio_sda, gpio_num_t i2c_gpio_scl, i2c_master_bus_handle_t* ret_i2c_bus_handle);

/**
 * @brief Initializes the PCA9955B LED driver.
 *
 * This function allocates memory, registers the I2C device, and performs the
 * initial hardware setup.
 *
 * @param[in]  i2c_addr        I2C address of the device (usually 0x69 for PCA9955B).
 * @param[in]  i2c_bus_handle  Handle to the configured I2C master bus.
 * @param[out] pca9955b        Pointer to store the created device handle.
 *
 * @return
 * - ESP_OK: Success.
 * - ESP_ERR_INVALID_ARG: Null pointer arguments.
 * - ESP_ERR_NO_MEM: Memory allocation failed.
 * - ESP_ERR_NOT_FOUND: Device not acknowledged on I2C bus.
 */
esp_err_t pca9955b_init(uint8_t i2c_addr, i2c_master_bus_handle_t i2c_bus_handle, pca9955b_handle_t* pca9955);

/**
 * @brief Sets the RGB color for a specific logical LED in the internal shadow buffer.
 *
 * @note This function only updates the local buffer. It does not transmit data
 * to the PCA9955B chip immediately.
 *
 * @param[in] pca9955b  Handle to the PCA9955B device.
 * @param[in] pixel_idx Index of the LED pixel (0 to 4).
 * @param[in] red       Red intensity value (0-255).
 * @param[in] green     Green intensity value (0-255).
 * @param[in] blue      Blue intensity value (0-255).
 *
 * @return
 * - ESP_OK: Success.
 * - ESP_ERR_INVALID_ARG: Handle is NULL or pixel_idx is out of range.
 */
esp_err_t pca9955b_set_pixel(pca9955b_handle_t pca9955b, uint8_t pixel_idx, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Transmits the internal color buffer to the PCA9955B via I2C.
 *
 * This function checks if the buffer has been modified (dirty flag) before transmitting.
 * It also handles automatic IREF restoration if a previous transmission failed.
 *
 * @param[in] pca9955b Handle to the PCA9955B device.
 *
 * @return
 * - ESP_OK: Success (or no update needed).
 * - ESP_ERR_INVALID_ARG: Handle is NULL.
 * - ESP_ERR_TIMEOUT: I2C bus is busy.
 * - ESP_FAIL: I2C transmission failed (Device NACK).
 */
esp_err_t pca9955b_show(pca9955b_handle_t pca9955b);

/**
 * @brief Deinitializes the PCA9955B device.
 *
 * This function turns off all LEDs (best effort), removes the device from the I2C bus,
 * frees the allocated memory, and sets the handle to NULL.
 *
 * @param[in,out] pca9955b Pointer to the device handle. Will be set to NULL on success.
 *
 * @return
 * - ESP_OK: Success.
 * - ESP_OK: Even if the handle was already NULL (idempotent).
 */
esp_err_t pca9955b_del(pca9955b_handle_t* pca9955b);

/**
 * @brief Bulk updates all LED channels by copying a raw byte array to the internal buffer.
 *
 * This allows setting all 15 PWM channels at once using a pre-calculated buffer.
 *
 * @note The input buffer must contain exactly 15 bytes (PWM0 to PWM14).
 * @note This function only updates the local buffer.
 *
 * @param[in] pca9955b Handle to the PCA9955B device.
 * @param[in] _buffer  Pointer to a 15-byte array containing PWM values.
 *
 * @return
 * - ESP_OK: Success.
 * - ESP_ERR_INVALID_ARG: Handle or buffer is NULL.
 */
esp_err_t pca9955b_write(pca9955b_handle_t pca9955b, const uint8_t* buffer);

/**
 * @brief Fills all LED channels with a single color.
 *
 * This is a helper function to set all 5 RGB LEDs to the same color instantly.
 *
 * @note This function only updates the local buffer. Call pca9955b_show() to transmit.
 *
 * @param[in] pca9955b Handle to the PCA9955B device.
 * @param[in] red      Red intensity value (0-255).
 * @param[in] green    Green intensity value (0-255).
 * @param[in] blue     Blue intensity value (0-255).
 *
 * @return
 * - ESP_OK: Success.
 * - ESP_ERR_INVALID_ARG: Handle is NULL.
 */
esp_err_t pca9955b_fill(pca9955b_handle_t pca9955b, uint8_t red, uint8_t green, uint8_t blue);

void pca9955b_test1();
void pca9955b_test2();

#ifdef __cplusplus
}
#endif