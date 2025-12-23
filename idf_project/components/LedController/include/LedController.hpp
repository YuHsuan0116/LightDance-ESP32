#pragma once

#include "BoardConfig.h"
#include "pca9955b_hal.h"
#include "ws2812b_hal.h"

class LedController {
  public:
    LedController();
    ~LedController();

    esp_err_t load_ch_info();
    esp_err_t init();
    esp_err_t show();
    esp_err_t del();

    esp_err_t load_config_test();
    esp_err_t ch_write_buffer(int, uint8_t*);
    esp_err_t fill(uint8_t, uint8_t, uint8_t);
    esp_err_t clear_buffer();
    esp_err_t black_out();

  private:
    ws2812b_handle_t ws2812b_devs[WS2812B_NUM];
    pca9955b_handle_t pca9955b_devs[PCA9955B_NUM];

    ch_info_t ch_info;
    uint8_t* buffer_entrance[WS2812B_NUM + PCA9955B_CH_NUM];

    i2c_master_bus_handle_t bus_handle;
};

void Controller_test();