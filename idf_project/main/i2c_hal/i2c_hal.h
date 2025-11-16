#pragma once

#include "../def.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_TIMEOUT_MS 100
#define I2C_FREQ 100000

#define IREFALL_ADDR 0x45

typedef struct {
    bool need_reset_IREF;
    uint8_t addr;
    uint8_t pca_channel;
    uint8_t reg_addr;
    i2c_master_dev_handle_t i2c_dev_handle;
} pca9955b_handle_t;

typedef struct {
    uint8_t addr;
    i2c_master_dev_handle_t i2c_dev_handle;
    uint8_t used_count;
    bool used[5];
} i2c_dev_entry_t;

i2c_master_dev_handle_t find_i2c_handle(uint8_t addr);

esp_err_t i2c_bus_init(gpio_num_t i2c_gpio_sda, gpio_num_t i2c_gpio_scl, i2c_master_bus_handle_t* ret_i2c_bus_handle);

esp_err_t i2c_write_hal(i2c_master_dev_handle_t i2c_dev_handle, uint8_t* const buffer, size_t const buffer_size, size_t const i2c_timeout_ms);
// i2c_write_hal(I2C_NUM_0, 0x5e, buffer, buffer_size, I2C_TIMEOUT_MS);

esp_err_t pca9955b_write_IREFALL(uint8_t IREF_val, pca9955b_handle_t* pca9955b);

esp_err_t pca9955b_write_rgb(uint8_t const r, uint8_t const g, uint8_t const b, pca9955b_handle_t* pca9955b);

esp_err_t pca9955b_config(led_config_t* led_config, pca9955b_handle_t* pca9955b);

esp_err_t pca9955b_del(pca9955b_handle_t* pca9955b);

#ifdef __cplusplus
}
#endif