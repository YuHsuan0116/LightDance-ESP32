#pragma once

#include "driver/i2c_master.h"

#include "BoardConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Packed 16-byte buffer mapping 1 PWM register + 5 RGB channels (3 bytes each).
 *
 * Memory layout (16 bytes total):
 *   [0]        pwm_reg0
 *   [1..15]    channel color data (union view as raw bytes or 5×3 RGB)
 */
typedef struct __attribute__((packed)) {
    uint8_t pwm_reg0; /*!< Global PWM register value */
    union {
        uint8_t data[15]; /*!< Raw access to 15-byte channel color payload */
        struct __attribute__((packed)) {
            uint8_t ch[5][3]; /*!< 5 channels × 3 bytes (R,G,B) */
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

    uint8_t i2c_addr;         /*!< 7-bit I2C device address */
    pca9955b_buffer_t buffer; /*!< PWM register + LED color buffer */

    bool need_reset_IREF; /*!< Set true if IREF register needs to be reinitialized */
    uint8_t IREF_cmd[2];  /*!< 2-byte IREF reset command to send over I2C */
} pca9955b_dev_t;

/**
 * @brief Opaque handle to PCA9955B device instance.
 */
typedef pca9955b_dev_t* pca9955b_handle_t;

/**
 * @brief Initialize an I2C master bus.
 *
 * Configures GPIOs for SDA/SCL and returns a master bus handle.
 *
 * @param i2c_gpio_sda  GPIO pin for I2C SDA
 * @param i2c_gpio_scl  GPIO pin for I2C SCL
 * @param ret_i2c_bus_handle  Pointer to receive initialized I2C bus handle
 * @return ESP_OK on success, or I2C/driver error code
 */
esp_err_t i2c_bus_init(gpio_num_t i2c_gpio_sda, gpio_num_t i2c_gpio_scl, i2c_master_bus_handle_t* ret_i2c_bus_handle);

/**
 * @brief Initialize a PCA9955B LED driver on an existing I2C master bus.
 *
 * @param i2c_addr  7-bit I2C address of the PCA9955B device
 * @param i2c_bus_handle  Initialized I2C master bus handle
 * @param pca9955b  Pointer to receive the created PCA9955B device handle
 * @return ESP_OK on success, or memory/I2C/driver error code
 */
esp_err_t pca9955b_init(uint8_t i2c_addr, i2c_master_bus_handle_t i2c_bus_handle, pca9955b_handle_t* pca9955);

/**
 * @brief Set RGB value of a single pixel in the PCA9955B frame buffer.
 *
 * @param pca9955b    PCA9955B device handle
 * @param pixel_idx   Pixel index (0–4)
 * @param red         8-bit red intensity
 * @param green       8-bit green intensity
 * @param blue        8-bit blue intensity
 * @return ESP_OK
 */
esp_err_t pca9955b_set_pixel(pca9955b_handle_t pca9955b, uint8_t pixel_idx, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Flush the 16-byte packed LED frame buffer to the PCA9955B over I2C.
 *
 * @param pca9955b  PCA9955B device handle
 * @return ESP_OK on success, or I2C transmit error
 */
esp_err_t pca9955b_show(pca9955b_handle_t pca9955b);

/**
 * @brief Deinitialize and free a PCA9955B device instance.
 *
 * @param pca9955b  Pointer to PCA9955B device handle to delete
 * @return ESP_OK
 */
esp_err_t pca9955b_del(pca9955b_handle_t* pca9955b);

esp_err_t pca9955b_write(pca9955b_handle_t pca9955b, uint8_t* buffer);
esp_err_t pca9955b_fill(pca9955b_handle_t pca9955b, uint8_t red, uint8_t green, uint8_t blue);

void pca9955b_test();

#ifdef __cplusplus
}
#endif