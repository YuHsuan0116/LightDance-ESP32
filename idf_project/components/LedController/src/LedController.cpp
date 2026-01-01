#include "LedController.hpp"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "BoardConfig.h"
#include "pca9955b_hal.h"
#include "ws2812b_hal.h"

#include "string.h"

LedController::LedController() {}

LedController::~LedController() {}

esp_err_t LedController::init(ch_info_t _ch_info) {
    ch_info = _ch_info;
    i2c_bus_init(GPIO_NUM_21, GPIO_NUM_22, &bus_handle);

    for(int i = 0; i < WS2812B_NUM; i++) {
        ws2812b_init(BOARD_HW_CONFIG.rmt_pins[i], ch_info.rmt_strips[i], &ws2812b_devs[i]);
    }
    for(int i = 0; i < PCA9955B_NUM; i++) {
        pca9955b_init(BOARD_HW_CONFIG.i2c_addrs[i], bus_handle, &pca9955b_devs[i]);
    }

    for(int i = 0; i < WS2812B_NUM; i++) {
        buffer_entrance[i] = (uint8_t*)&ws2812b_devs[i]->buffer;
    }
    for(int i = 0; i < PCA9955B_CH_NUM; i++) {
        buffer_entrance[WS2812B_NUM + i] = (uint8_t*)&pca9955b_devs[i]->buffer.ch[i % 5];
    }

    return ESP_OK;
}

esp_err_t LedController::write_buffer(uint8_t** buffer) {
    for(int ch_idx = 0; ch_idx < WS2812B_NUM; ch_idx++) {
        for(int pixel_idx = 0; pixel_idx < ch_info.pixel_counts[ch_idx]; pixel_idx++) {
            ws2812b_set_pixel(ws2812b_devs[ch_idx],
                              pixel_idx,
                              buffer[ch_idx][3 * pixel_idx + 0],
                              buffer[ch_idx][3 * pixel_idx + 1],
                              buffer[ch_idx][3 * pixel_idx + 2]);
        }
    }
    for(int ch_idx = 0; ch_idx < PCA9955B_CH_NUM; ch_idx++) {
        for(int pixel_idx = 0; pixel_idx < ch_info.pixel_counts[WS2812B_NUM + ch_idx]; pixel_idx++) {
            pca9955b_set_pixel(pca9955b_devs[ch_idx % 5],
                               pixel_idx,
                               buffer[WS2812B_NUM + ch_idx][3 * pixel_idx + 0],
                               buffer[WS2812B_NUM + ch_idx][3 * pixel_idx + 1],
                               buffer[WS2812B_NUM + ch_idx][3 * pixel_idx + 2]);
        }
    }

    return ESP_OK;
}

static uint64_t start;
static uint64_t end;
esp_err_t LedController::show() {
    if(SHOW_TIME_PER_FRAME) {
        start = esp_timer_get_time();
    }
    for(int i = 0; i < WS2812B_NUM; i++) {
        ws2812b_show(ws2812b_devs[i]);
    }
    for(int i = 0; i < PCA9955B_NUM; i++) {
        pca9955b_show(pca9955b_devs[i]);
    }
    for(int i = 0; i < WS2812B_NUM; i++) {
        ws2812b_wait_done(ws2812b_devs[i]);
    }
    if(SHOW_TIME_PER_FRAME) {
        end = esp_timer_get_time();
        ESP_LOGI("LedController.cpp", "show(): %llu us", end - start);
    }

    return ESP_OK;
}

esp_err_t LedController::deinit() {
    for(int i = 0; i < WS2812B_NUM; i++) {
        ws2812b_del(&(ws2812b_devs[i]));
    }
    for(int i = 0; i < PCA9955B_NUM; i++) {
        pca9955b_del(&(pca9955b_devs[i]));
    }

    // memset(&ch_info, 0, sizeof(ch_info_t));
    memset(buffer_entrance, 0, 3 * (WS2812B_NUM + PCA9955B_CH_NUM) * sizeof(uint8_t));

    i2c_del_master_bus(bus_handle);

    return ESP_OK;
}

esp_err_t LedController::config_test() {
    for(int i = 0; i < WS2812B_NUM; i++) {
        ch_info.rmt_strips[i] = 5;
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
    memcpy(buffer_entrance[idx], buffer, 3 * ch_info.pixel_counts[idx] * sizeof(uint8_t));
    return ESP_OK;
}

esp_err_t LedController::dev_write_buffer(int idx, uint8_t* buffer) {
    if(idx < WS2812B_NUM) {
        ws2812b_write(ws2812b_devs[idx], (uint8_t*)buffer);
    } else {
        pca9955b_write(pca9955b_devs[idx - WS2812B_NUM], buffer);
    }
    return ESP_OK;
}

esp_err_t LedController::clear_buffer() {
    for(int i = 0; i < WS2812B_NUM; i++) {
        memset(ws2812b_devs[i]->buffer, 0, 3 * ws2812b_devs[i]->pixel_num * sizeof(uint8_t));
    }
    for(int i = 0; i < PCA9955B_NUM; i++) {
        memset(pca9955b_devs[i]->buffer.data, 0, 15);
    }
    return ESP_OK;
}

void Controller_test() {
    uint8_t red[4] = {15, 0, 0, 15};
    uint8_t green[4] = {0, 15, 0, 15};
    uint8_t blue[4] = {0, 0, 15, 15};

    LedController controller;
    ch_info_t ch_info;

    for(int i = 0; i < WS2812B_NUM; i++) {
        ch_info.rmt_strips[i] = 100;
    }
    for(int i = 0; i < PCA9955B_CH_NUM; i++) {
        ch_info.i2c_leds[i] = 1;
    }

    controller.init(ch_info);

    vTaskDelay(pdMS_TO_TICKS(1000));

    for(int i = 0; i < 100; i++) {
        controller.fill(red[i % 4], green[i % 4], blue[i % 4]);
        controller.show();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    controller.deinit();
}

void LedController::print_buffer() {
    for(int i = 0; i < WS2812B_NUM; i++) {
        ws2812b_print_buffer(ws2812b_devs[i]);
    }
}