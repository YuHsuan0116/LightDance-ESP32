#include "LedDriver.h"

/**
 * @brief buffer for WS2812B RGB data.
 */
static uint8_t cmd[3 * WS2812B_MAXIMUM_LED_COUNT];

esp_err_t LedDriver_init_i2c(LedDriver_handle_t* LedDriver) {
    esp_err_t ret = ESP_OK;
    if(LedDriver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ret = i2c_bus_init(I2C_GPIO_SDA, I2C_GPIO_SCL, &(LedDriver->bus_handle));
    if(ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

esp_err_t LedDriver_del_i2c(LedDriver_handle_t* LedDriver) {
    esp_err_t ret = ESP_OK;

    if(LedDriver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(LedDriver->bus_handle == NULL) {
        return ESP_OK;
    }

    ret = i2c_del_master_bus(LedDriver->bus_handle);
    if(ret != ESP_OK) {
        return ret;
    }

    LedDriver->bus_handle = NULL;

    return ESP_OK;
}

esp_err_t LedDriver_init(LedDriver_handle_t* LedDriver) {
    esp_err_t ret = ESP_OK;

    if(LedDriver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(LedDriver, 0, sizeof(LedDriver_handle_t));

    ret = LedDriver_init_i2c(LedDriver);
    if(ret != ESP_OK) {
        return ret;
    }

    LedDriver->initialized = true;

    return ESP_OK;
}

esp_err_t LedDriver_config(led_config_t* led_configs, int channel_number, LedDriver_handle_t* LedDriver) {
    esp_err_t ret = ESP_OK;

    if(led_configs == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(LedDriver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(!LedDriver->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if(channel_number <= 0 || channel_number > LED_DRIVER_MAX_CHANNEL_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    LedDriver->channel_number = channel_number;

    for(int i = 0; i < LedDriver->channel_number; i++) {
        ret = channel_handle_config(&led_configs[i], &LedDriver->channel_handle[i]);
        if(ret != ESP_OK) {
            for(int j = 0; i < j; j++) {
                (void)channel_handle_del(&LedDriver->channel_handle[j]);
            }

            LedDriver->channel_number = 0;
            return ret;
        }
    }

    return ESP_OK;
}

esp_err_t LedDriver_del(LedDriver_handle_t* LedDriver) {
    esp_err_t ret = ESP_OK;
    esp_err_t last_error = ESP_OK;

    if(LedDriver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(!LedDriver->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    for(int i = 0; i < LedDriver->channel_number; i++) {
        ret = channel_handle_del(&LedDriver->channel_handle[i]);
        if(ret != ESP_OK) {
            last_error = ret;
        }
    }

    ret = LedDriver_del_i2c(LedDriver);
    if(ret != ESP_OK) {
        last_error = ret;
    }

    memset(LedDriver, 0, sizeof(LedDriver_handle_t));

    return last_error;
}

esp_err_t LedDriver_write(color_t** colors, LedDriver_handle_t* LedDriver) {
    esp_err_t ret = ESP_OK;
    esp_err_t last_error = ESP_OK;

    if(colors == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(LedDriver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(!LedDriver->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    for(int i = 0; i < LedDriver->channel_number; i++) {
        if(colors[i] == NULL) {
            continue;
        }

        ret = channel_handle_write(colors[i], &(LedDriver->channel_handle[i]));
        if(ret != ESP_OK) {
            last_error = ret;
        }
    }

    return last_error;
}

esp_err_t LedDriver_set_rgb(uint8_t red, uint8_t green, uint8_t blue, LedDriver_handle_t* LedDriver) {
    esp_err_t ret = ESP_OK;
    esp_err_t last_error = ESP_OK;

    if(LedDriver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(!LedDriver->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    for(int i = 0; i < WS2812B_MAXIMUM_LED_COUNT; i++) {
        cmd[3 * i] = green;
        cmd[3 * i + 1] = red;
        cmd[3 * i + 2] = blue;
    }

    for(int i = 0; i < LedDriver->channel_number; i++) {
        ret = channel_handle_write((color_t*)cmd, &(LedDriver->channel_handle[i]));
        if(ret != ESP_OK) {
            last_error = ret;
        }
    }
    return last_error;
}

esp_err_t LedDriver_set_color(color_t color, LedDriver_handle_t* LedDriver) {
    esp_err_t ret = ESP_OK;

    if(LedDriver == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(!LedDriver->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ret = LedDriver_set_rgb(color.red, color.green, color.red, LedDriver);
    if(ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}
