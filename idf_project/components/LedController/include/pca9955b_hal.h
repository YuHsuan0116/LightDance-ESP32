#pragma once

#include "driver/i2c_master.h"

#include "BoardConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    i2c_master_dev_handle_t i2c_dev_handle;

    bool need_reset_IREF;
    uint8_t i2c_addr;
    uint8_t* buffer;
    uint8_t* IREF_cmd;
} pca9955b_dev_t;

typedef pca9955b_dev_t* pca9955b_handle_t;

esp_err_t pca9955b_init(uint8_t i2c_addr, pca9955b_handle_t* pca9955);
esp_err_t pca9955b_show(pca9955b_handle_t pca9955b);
esp_err_t pca9955b_del(pca9955b_handle_t* pca9955b);

esp_err_t pca9955b_fill(pca9955b_handle_t pca9955b, uint8_t red, uint8_t green, uint8_t blue);
esp_err_t pca9955b_write_buffer(pca9955b_handle_t pca9955b, uint8_t* buffer);

esp_err_t i2c_bus_init(gpio_num_t i2c_gpio_sda, gpio_num_t i2c_gpio_scl, i2c_master_bus_handle_t* ret_i2c_bus_handle);

void pca9955b_test();

#ifdef __cplusplus
}
#endif