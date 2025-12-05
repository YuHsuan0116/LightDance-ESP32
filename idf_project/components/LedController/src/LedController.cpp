#include "LedController.hpp"

#include "pca9955b_hal.h"
#include "ws2812b_hal.h"

#include "string.h"

LedController::LedController() {
    hw_num = 0;
    ws2812b_num = 0;
    pca9955b_num = 0;
    ch_num = 0;
}

LedController::~LedController() {}

esp_err_t LedController::load_config() {
    LedController_config_t* LedController_config = (LedController_config_t*)calloc(1, sizeof(LedController_config_t));

    // TODO: reader give LedController_config
    // read_config(LedController)

    hw_num = LedController_config->hw_num;
    for(int i = 0; i < hw_num; i++) {
        dev_configs[i] = LedController_config->hw_configs[i];
    }

    ch_num = LedController_config->ch_num;
    for(int i = 0; i < ch_num; i++) {
        ch_configs[i] = LedController_config->ch_configs[i];
    }

    free(LedController_config);

    return ESP_OK;
}

esp_err_t LedController::init() {
    i2c_bus_init(GPIO_NUM_21, GPIO_NUM_22, &bus_handle);

    ws2812b_num = 0;
    pca9955b_num = 0;

    for(int i = 0; i < hw_num; i++) {
        if(dev_configs[i].type == LED_TYPE_WS2812B) {
            ws2812b_init(dev_configs[i].gpio_num, dev_configs[i].pixel_num, &ws2812b_devs[ws2812b_num++]);
        }

        if(dev_configs[i].type == LED_TYPE_PCA9955B) {
            pca9955b_init(dev_configs[i].i2c_addr, &pca9955b_devs[pca9955b_num++]);
        }
    }

    for(int i = 0; i < ch_num; i++) {
        if(ch_configs[i].type == LED_TYPE_PCA9955B) {
            int idx = ch_configs[i].ch_idx;
            ch[idx].type = LED_TYPE_PCA9955B;

            int pca9955b_idx = -1;
            for(int j = 0; j < pca9955b_num; j++) {
                if(pca9955b_devs[j]->i2c_addr == ch_configs[i].i2c_addr) {
                    pca9955b_idx = j;
                    break;
                }
            }
            if(pca9955b_idx != -1) {
                ch[idx].buffer = pca9955b_devs[pca9955b_idx]->buffer + 1 + 3 * ch_configs[i].pca_ch;
                ch[idx].buffer_size = 3;
            }
        }

        if(ch_configs[i].type == LED_TYPE_WS2812B) {
            int idx = ch_configs[i].ch_idx;
            ch[idx].type = LED_TYPE_WS2812B;

            int ws2812b_idx = -1;
            for(int j = 0; j < ws2812b_num; j++) {
                if(ws2812b_devs[j]->gpio_num == ch_configs[i].rmt_gpio) {
                    ws2812b_idx = j;
                    break;
                }
            }
            if(ws2812b_idx != -1) {
                ch[idx].buffer = ws2812b_devs[ws2812b_idx]->buffer;
                ch[idx].buffer_size = ws2812b_devs[ws2812b_idx]->buffer_size;
            }
        }
    }

    return ESP_OK;
}

esp_err_t LedController::show() {
    for(int i = 0; i < ws2812b_num; i++) {
        ws2812b_show(ws2812b_devs[i]);
    }
    for(int i = 0; i < pca9955b_num; i++) {
        pca9955b_show(pca9955b_devs[i]);
    }

    return ESP_OK;
}

esp_err_t LedController::del() {
    for(int i = 0; i < ws2812b_num; i++) {
        ws2812b_del(&(ws2812b_devs[i]));
    }
    for(int i = 0; i < pca9955b_num; i++) {
        pca9955b_del(&(pca9955b_devs[i]));
    }

    hw_num = 0;
    memset(dev_configs, 0, 12 * sizeof(hw_config_t));
    ch_num = 0;
    memset(ch_configs, 0, 44 * sizeof(ch_config_t));
    memset(ch, 0, 44 * sizeof(led_channel_t));

    ws2812b_num = 0;
    pca9955b_num = 0;

    i2c_del_master_bus(bus_handle);

    return ESP_OK;
}

esp_err_t LedController::load_config_test() {
    hw_num = 6;
    ws2812b_num = 4;
    pca9955b_num = 2;

    uint8_t ws2812b_gpio[8] = {32, 25, 26, 27, 19, 18, 5, 17};
    for(int i = 0; i < ws2812b_num; i++) {
        dev_configs[i].type = LED_TYPE_WS2812B;
        dev_configs[i].gpio_num = (gpio_num_t)ws2812b_gpio[i];
        dev_configs[i].pixel_num = 150;
    }

    uint8_t pca9955b_addr[2] = {0x5c, 0x5e};
    for(int i = 0; i < pca9955b_num; i++) {
        dev_configs[ws2812b_num + i].type = LED_TYPE_PCA9955B;
        dev_configs[ws2812b_num + i].i2c_addr = pca9955b_addr[i];
    }

    ch_num = 14;

    for(int i = 0; i < ws2812b_num; i++) {
        ch_configs[i].ch_idx = i;
        ch_configs[i].type = LED_TYPE_WS2812B;
        ch_configs[i].rmt_gpio = (gpio_num_t)ws2812b_gpio[i];
    }

    for(int i = 0; i < 5 * pca9955b_num; i++) {
        ch_configs[ws2812b_num + i].ch_idx = ws2812b_num + i;
        ch_configs[ws2812b_num + i].type = LED_TYPE_PCA9955B;
        ch_configs[ws2812b_num + i].i2c_addr = pca9955b_addr[i / 5];
        ch_configs[ws2812b_num + i].pca_ch = i % 5;
    }

    return ESP_OK;
}

esp_err_t LedController::fill(uint8_t red, uint8_t green, uint8_t blue) {
    for(int i = 0; i < ws2812b_num; i++) {
        ws2812b_fill(ws2812b_devs[i], red, green, blue);
    }
    for(int i = 0; i < pca9955b_num; i++) {
        pca9955b_fill(pca9955b_devs[i], red, green, blue);
    }

    return ESP_OK;
}

esp_err_t LedController::ch_write_buffer(int idx, uint8_t* buffer) {
    memcpy(ch[idx].buffer, buffer, ch[idx].buffer_size);
    return ESP_OK;
}

esp_err_t LedController::clear_buffer() {
    for(int i = 0; i < ws2812b_num; i++) {
        memset(ws2812b_devs[i]->buffer, 0, ws2812b_devs[i]->buffer_size);
    }
    for(int i = 0; i < pca9955b_num; i++) {
        memset(pca9955b_devs[i]->buffer + 1, 0, 15);
    }
    return ESP_OK;
}