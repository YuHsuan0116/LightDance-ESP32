#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "LedController.hpp"

#include "BoardConfig.h"
#include "pca9955b_hal.h"
#include "ws2812b_hal.h"

#include "string.h"

LedController::LedController() {}

LedController::~LedController() {}

esp_err_t LedController::load_ch_info() {
    // TODO: reader give ch_info;

    return ESP_OK;
}

esp_err_t LedController::init() {
    i2c_bus_init(GPIO_NUM_21, GPIO_NUM_22, &bus_handle);

    for(int i = 0; i < WS2812B_NUM; i++) {
        ws2812b_init(BOARD_HW_CONFIG.rmt_pins[i], ch_info.rmt_strips[i], &ws2812b_devs[i]);
    }
    for(int i = 0; i < PCA9955B_NUM; i++) {
        pca9955b_init(BOARD_HW_CONFIG.i2c_addrs[i], &pca9955b_devs[i]);
    }

    for(int i = 0; i < WS2812B_NUM; i++) {
        buffer_entrance[i] = ws2812b_devs[i]->buffer;
    }
    for(int i = 0; i < PCA9955B_CH_NUM; i++) {
        buffer_entrance[WS2812B_NUM + i] = pca9955b_devs[i / 5]->buffer + 1 + 3 * (i % 5);
    }

    return ESP_OK;
}

esp_err_t LedController::show() {
    for(int i = 0; i < WS2812B_NUM; i++) {
        ws2812b_show(ws2812b_devs[i]);
    }
    for(int i = 0; i < PCA9955B_NUM; i++) {
        pca9955b_show(pca9955b_devs[i]);
    }

    return ESP_OK;
}

esp_err_t LedController::del() {
    for(int i = 0; i < WS2812B_NUM; i++) {
        ws2812b_del(&(ws2812b_devs[i]));
    }
    for(int i = 0; i < PCA9955B_NUM; i++) {
        pca9955b_del(&(pca9955b_devs[i]));
    }

    memset(&ch_info, 0, sizeof(ch_info_t));
    memset(buffer_entrance, 0, (WS2812B_NUM + PCA9955B_CH_NUM) * sizeof(uint8_t*));

    i2c_del_master_bus(bus_handle);

    return ESP_OK;
}

esp_err_t LedController::load_config_test() {
    for(int i = 0; i < WS2812B_NUM; i++) {
        ch_info.rmt_strips[i] = 100;
    }
    for(int i = 0; i < PCA9955B_CH_NUM; i++) {
        ch_info.i2c_leds[i] = 1;
    }
    return ESP_OK;
}

esp_err_t LedController::fill(uint8_t red, uint8_t green, uint8_t blue) {
    for(int i = 0; i < WS2812B_NUM; i++) {
        ws2812b_fill(ws2812b_devs[i], red, green, blue);
    }
    for(int i = 0; i < PCA9955B_NUM; i++) {
        pca9955b_fill(pca9955b_devs[i], red, green, blue);
    }
    return ESP_OK;
}

esp_err_t LedController::ch_write_buffer(int idx, uint8_t* buffer) {
    memcpy(buffer_entrance[idx], buffer, 3 * ch_info.pixel_counts[idx]);
    return ESP_OK;
}

esp_err_t LedController::clear_buffer() {
    for(int i = 0; i < WS2812B_NUM; i++) {
        memset(ws2812b_devs[i]->buffer, 0, ws2812b_devs[i]->buffer_size);
    }
    for(int i = 0; i < PCA9955B_NUM; i++) {
        memset(pca9955b_devs[i]->buffer + 1, 0, 15);
    }
    return ESP_OK;
}

void Controller_test() {
    uint8_t red[4] = {15, 0, 0, 15};
    uint8_t green[4] = {0, 15, 0, 15};
    uint8_t blue[4] = {0, 0, 15, 15};

    LedController controller;

    controller.load_config_test();
    controller.init();

    vTaskDelay(pdMS_TO_TICKS(1000));

    for(int i = 0; i < 100; i++) {
        controller.fill(red[i % 4], green[i % 4], blue[i % 4]);
        controller.show();

        vTaskDelay(pdMS_TO_TICKS(250));
    }

    controller.del();
}