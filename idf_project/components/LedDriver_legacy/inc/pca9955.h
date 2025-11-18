#pragma once

#include "driver/i2c_master.h"
#include "led_def.h"

/**
 * @brief Initialize an I2C master bus on I2C_NUM_0.
 *
 * @param[in]  SCL_GPIO     GPIO number for the I2C SCL line.
 * @param[in]  SDA_GPIO     GPIO number for the I2C SDA line.
 * @param[out] ret_i2c_bus  Pointer to receive the created I2C master bus handle.
 *
 * @return ESP_OK on success;
 */
esp_err_t i2c_bus_init(int SCL_GPIO, int SDA_GPIO, i2c_master_bus_handle_t* ret_i2c_bus);