#pragma once

#include "../channel_handle/channel_handle.h"

#define MAX_CHANNEL_COUNT 48
#define I2C_GPIO_SDA GPIO_NUM_21
#define I2C_GPIO_SCL GPIO_NUM_22

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int channel_number;
    i2c_master_bus_handle_t bus_handle;
    channel_handle_t channel_handle[MAX_CHANNEL_COUNT];
} LedDriver_handle_t;

esp_err_t LedDriver_init_i2c(LedDriver_handle_t* LedDriver);

esp_err_t LedDriver_del_i2c(LedDriver_handle_t* LedDriver);

esp_err_t LedDriver_init(LedDriver_handle_t* LedDriver);

esp_err_t LedDriver_config(led_config_t* led_configs, int _channel_number, LedDriver_handle_t* LedDriver);

esp_err_t LedDriver_del(LedDriver_handle_t* LedDriver);

esp_err_t LedDriver_set_rgb(uint8_t r, uint8_t g, uint8_t b, LedDriver_handle_t* LedDriver);

#ifdef __cplusplus
}
#endif