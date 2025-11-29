#include "pca9955b_hal.h"

#include "string.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PCA9955B_IREFALL_ADDR 0x45

esp_err_t pca9955b_init(uint8_t i2c_addr, pca9955b_handle_t* pca9955b) {
    esp_err_t ret = ESP_OK;

    *pca9955b = NULL;

    pca9955b_dev_t* dev = (pca9955b_dev_t*)calloc(1, sizeof(pca9955b_dev_t));
    if(dev == NULL) {
        return ESP_ERR_NO_MEM;
    }

    dev->i2c_addr = i2c_addr;
    dev->need_reset_IREF = true;

    dev->buffer = (uint8_t*)calloc(16, sizeof(uint8_t));
    if(dev->buffer == NULL) {
        free(dev);
        return ESP_ERR_NO_MEM;
    }
    dev->buffer[0] = 0x88;

    dev->IREF_cmd = (uint8_t*)calloc(2, sizeof(uint8_t));
    if(dev->IREF_cmd == NULL) {
        free(dev->buffer);
        free(dev);
        return ESP_ERR_NO_MEM;
    }
    dev->IREF_cmd[0] = PCA9955B_IREFALL_ADDR;
    dev->IREF_cmd[1] = 0xff;

    i2c_master_bus_handle_t i2c_bus_handle = NULL;
    ret = i2c_master_get_bus_handle(I2C_NUM_0, &i2c_bus_handle);
    if(ret != ESP_OK) {
        return ret;
    }
    if(i2c_bus_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t i2c_dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_addr,
        .scl_speed_hz = I2C_FREQ,
    };

    ret = i2c_master_bus_add_device(i2c_bus_handle, &i2c_dev_config, &dev->i2c_dev_handle);
    if(ret != ESP_OK) {
        free(dev->IREF_cmd);
        free(dev->buffer);
        free(dev);
        return ret;
    }

    *pca9955b = dev;

    ret = i2c_master_transmit(dev->i2c_dev_handle, dev->IREF_cmd, 2, I2C_TIMEOUT_MS);
    if(ret == ESP_OK) {
        dev->need_reset_IREF = false;
    }

    return ESP_OK;
}

esp_err_t pca9955b_show(pca9955b_handle_t pca9955b) {
    esp_err_t ret = ESP_OK;

    if(pca9955b == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(pca9955b->need_reset_IREF) {
        ret = i2c_master_transmit(pca9955b->i2c_dev_handle, pca9955b->IREF_cmd, 2, I2C_TIMEOUT_MS);
        if(ret == ESP_OK) {
            pca9955b->need_reset_IREF = false;
        } else {
            return ret;
        }
    }

    ret = i2c_master_transmit(pca9955b->i2c_dev_handle, pca9955b->buffer, 16, I2C_TIMEOUT_MS);
    if(ret != ESP_OK) {
        pca9955b->need_reset_IREF = true;
        return ret;
    }
    return ESP_OK;
}

esp_err_t pca9955b_del(pca9955b_handle_t* pca9955b) {
    if(pca9955b == NULL || *pca9955b == NULL) {
        return ESP_OK;
    }

    pca9955b_dev_t* dev = *pca9955b;

    if(dev->buffer) {
        memset(dev->buffer + 1, 0, 15 * sizeof(uint8_t));
        pca9955b_show(dev);
    }

    if(dev->i2c_dev_handle) {
        i2c_master_bus_rm_device(dev->i2c_dev_handle);
    }

    if(dev->IREF_cmd) {
        free(dev->IREF_cmd);
    }
    if(dev->buffer) {
        free(dev->buffer);
    }

    free(dev);

    *pca9955b = NULL;

    return ESP_OK;
}

esp_err_t pca9955b_fill(pca9955b_handle_t pca9955b, uint8_t red, uint8_t green, uint8_t blue) {
    for(int i = 0; i < 5; i++) {
        pca9955b->buffer[3 * i + 1] = red;
        pca9955b->buffer[3 * i + 2] = green;
        pca9955b->buffer[3 * i + 3] = blue;
    }

    return ESP_OK;
}

esp_err_t i2c_bus_init(gpio_num_t i2c_gpio_sda, gpio_num_t i2c_gpio_scl, i2c_master_bus_handle_t* ret_i2c_bus_handle) {
    esp_err_t ret = ESP_OK;

    if(i2c_gpio_sda == i2c_gpio_scl) {
        return ESP_ERR_INVALID_ARG;
    }

    *ret_i2c_bus_handle = NULL;

    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = i2c_gpio_sda,
        .scl_io_num = i2c_gpio_scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ret = i2c_new_master_bus(&i2c_bus_config, ret_i2c_bus_handle);
    if(ret != ESP_OK) {
        *ret_i2c_bus_handle = NULL;
        return ret;
    }

    return ESP_OK;
}

void pca9955b_test() {
    i2c_master_bus_handle_t bus_handle;
    i2c_bus_init(21, 22, &bus_handle);

    pca9955b_handle_t pca9955b[2];
    uint8_t i2c_addr[2] = {0x5c, 0x5e};

    uint8_t r[3] = {255, 0, 0};
    uint8_t g[3] = {0, 255, 0};
    uint8_t b[3] = {0, 0, 255};

    esp_err_t ret = ESP_OK;
    for(int idx = 0; idx < 2; idx++) {
        ret = pca9955b_init(i2c_addr[idx], &pca9955b[idx]);
        if(ret != ESP_OK) {
            ESP_LOGE("pca9955b_test", "bad");
        }
    }

    for(int i = 0; i < 100; i++) {
        for(int idx = 0; idx < 2; idx++) {
            pca9955b_fill(pca9955b[idx], r[i % 3], g[i % 3], b[i % 3]);
        }

        uint64_t start_time = esp_timer_get_time();
        for(int j = 0; j < 1; j++) {
            for(int idx = 0; idx < 2; idx++) {
                pca9955b_show(pca9955b[idx]);
            }
        }
        uint64_t end_time1 = esp_timer_get_time();

        ESP_LOGI("pca9955b_test", "show: %lld us", end_time1 - start_time);

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    for(int idx = 0; idx < 2; idx++) {
        pca9955b_del(&pca9955b[idx]);
    }

    i2c_del_master_bus(bus_handle);
}