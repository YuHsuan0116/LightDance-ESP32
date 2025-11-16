#include "led_driver.h"

static uint8_t cmd[3 * WS2812B_MAXIMUM_LED_COUNT];
static uint8_t cmd_size;

esp_err_t LedDriver_init_i2c(LedDriver_handle_t* LedDriver) {

    return i2c_bus_init(I2C_GPIO_SDA, I2C_GPIO_SCL, &(LedDriver->bus_handle));
}

esp_err_t LedDriver_del_i2c(LedDriver_handle_t* LedDriver) {
    return i2c_del_master_bus(LedDriver->bus_handle);
}

esp_err_t LedDriver_init(LedDriver_handle_t* LedDriver) {

    return LedDriver_init_i2c(LedDriver);
}

esp_err_t LedDriver_config(led_config_t* led_configs, int _channel_number, LedDriver_handle_t* LedDriver) {
    LedDriver->channel_number = _channel_number;
    for(int i = 0; i < LedDriver->channel_number; i++) {
        channel_handle_config(&led_configs[i], &LedDriver->channel_handle[i]);
    }
    return ESP_OK;
}

esp_err_t LedDriver_del(LedDriver_handle_t* LedDriver) {
    for(int i = 0; i < LedDriver->channel_number; i++) {
        channel_handle_del(&LedDriver->channel_handle[i]);
    }
    LedDriver_del_i2c(LedDriver);
    return ESP_OK;
}

esp_err_t LedDriver_write(color_t** colors, LedDriver_handle_t* LedDriver) {
    for(int i = 0; i < LedDriver->channel_number; i++) {
        channel_handle_write(colors[i], &(LedDriver->channel_handle[i]));
    }
    return ESP_OK;
}

esp_err_t LedDriver_set_rgb(uint8_t r, uint8_t g, uint8_t b, LedDriver_handle_t* LedDriver) {
    for(int i = 0; i < WS2812B_MAXIMUM_LED_COUNT; i++) {
        cmd[3 * i] = g;
        cmd[3 * i + 1] = r;
        cmd[3 * i + 2] = b;
    }
    for(int i = 0; i < LedDriver->channel_number; i++) {
        channel_handle_write((color_t*)cmd, &(LedDriver->channel_handle[i]));
    }
    return ESP_OK;
}
