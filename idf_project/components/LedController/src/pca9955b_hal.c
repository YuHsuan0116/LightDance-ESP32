#include "pca9955b_hal.h"

#include "string.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PCA9955B_PWM0_ADDR 0x88  // with auto increment
#define PCA9955B_IREFALL_ADDR 0x45

esp_err_t pca9955b_init(uint8_t i2c_addr, i2c_master_bus_handle_t i2c_bus_handle, pca9955b_handle_t* pca9955b) {
    esp_err_t ret = ESP_OK;

    if(i2c_bus_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    *pca9955b = NULL;

    pca9955b_dev_t* dev = (pca9955b_dev_t*)calloc(1, sizeof(pca9955b_dev_t));
    if(dev == NULL) {
        return ESP_ERR_NO_MEM;
    }

    dev->i2c_addr = i2c_addr;
    dev->buffer.pwm_reg0 = PCA9955B_PWM0_ADDR;

    dev->need_reset_IREF = true;
    dev->IREF_cmd[0] = PCA9955B_IREFALL_ADDR;
    dev->IREF_cmd[1] = 0xff;

    i2c_device_config_t i2c_dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_addr,
        .scl_speed_hz = I2C_FREQ,
    };

    ret = i2c_master_bus_add_device(i2c_bus_handle, &i2c_dev_config, &dev->i2c_dev_handle);
    if(ret != ESP_OK) {
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

esp_err_t pca9955b_set_pixel(pca9955b_handle_t pca9955b, uint8_t pixel_idx, uint8_t red, uint8_t green, uint8_t blue) {
    pca9955b->buffer.ch[pixel_idx][0] = red;
    pca9955b->buffer.ch[pixel_idx][1] = green;
    pca9955b->buffer.ch[pixel_idx][2] = blue;

    return ESP_OK;
}
esp_err_t pca9955b_write(pca9955b_handle_t pca9955b, uint8_t* _buffer) {
    memcpy(pca9955b->buffer.data, _buffer, 15 * sizeof(uint8_t));

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

    ret = i2c_master_transmit(pca9955b->i2c_dev_handle, (uint8_t*)&pca9955b->buffer, 16, I2C_TIMEOUT_MS);
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

    memset(dev->buffer.data, 0, 15 * sizeof(uint8_t));
    pca9955b_show(dev);
    i2c_master_bus_rm_device(dev->i2c_dev_handle);

    free(dev);
    *pca9955b = NULL;
    return ESP_OK;
}

esp_err_t pca9955b_fill(pca9955b_handle_t pca9955b, uint8_t red, uint8_t green, uint8_t blue) {
    for(int i = 0; i < 5; i++) {
        pca9955b->buffer.ch[i][0] = red;
        pca9955b->buffer.ch[i][1] = green;
        pca9955b->buffer.ch[i][2] = blue;
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
    i2c_bus_init(GPIO_NUM_21, GPIO_NUM_22, &bus_handle);

    pca9955b_handle_t pca9955b[PCA9955B_NUM];

    uint8_t r[3] = {15, 0, 0};
    uint8_t g[3] = {0, 15, 0};
    uint8_t b[3] = {0, 0, 15};

    esp_err_t ret = ESP_OK;
    ESP_LOGI("pca9955b_test", "testing pca9955b_init");

    for(int idx = 0; idx < PCA9955B_NUM; idx++) {
        ret = pca9955b_init(BOARD_HW_CONFIG.i2c_addrs[idx], bus_handle, &pca9955b[idx]);
        if(ret != ESP_OK) {
            ESP_LOGE("pca9955b_test", "pca9955b_init failed");
        }
    }
    ESP_LOGI("pca9955b_test", "pca9955b_init successed");

    ESP_LOGI("pca9955b_test", "testing pca9955b_set_pixel and pca9955b_show");
    for(int i = 0; i < 30; i++) {
        for(int idx = 0; idx < PCA9955B_NUM; idx++) {
            for(int pixel_idx = 0; pixel_idx < 5; pixel_idx++) {
                pca9955b_set_pixel(pca9955b[idx], pixel_idx, r[i % 3], g[i % 3], b[i % 3]);
            }
        }

        for(int idx = 0; idx < PCA9955B_NUM; idx++) {
            pca9955b_show(pca9955b[idx]);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI("pca9955b_test", "pca9955b_set_pixel and pca9955b_show successed");

    ESP_LOGI("pca9955b_test", "testing pca9955b_del");
    for(int idx = 0; idx < PCA9955B_NUM; idx++) {
        pca9955b_del(&pca9955b[idx]);
    }

    ESP_LOGI("pca9955b_test", "pca9955b_del successed");

    i2c_del_master_bus(bus_handle);
}