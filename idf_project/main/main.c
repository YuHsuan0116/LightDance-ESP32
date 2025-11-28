#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "LedDriver/pca9955b_hal.h"
#include "LedDriver/ws2812b_hal.h"

void app_main() {

    i2c_master_bus_handle_t bus_handle;
    i2c_bus_init(21, 22, &bus_handle);

    pca9955b_handle_t pca9955b[2];
    uint8_t i2c_addr[2] = {0x5c, 0x5e};

    for(int idx = 0; idx < 2; idx++) {
        pca9955b_init(i2c_addr[idx], &pca9955b[idx]);
    }

    ws2812b_handle_t ws2812b[8];
    uint8_t gpio[8] = {32, 25, 26, 27, 19, 18, 5, 17};

    for(int idx = 0; idx < 8; idx++) {
        ws2812b_init(gpio[idx], 100, &ws2812b[idx]);
    }

    uint8_t r[3] = {255, 0, 0};
    uint8_t g[3] = {0, 255, 0};
    uint8_t b[3] = {0, 0, 255};

    for(int i = 0; i < 100; i++) {
        for(int idx = 0; idx < 8; idx++) {
            ws2812b_fill(ws2812b[idx], r[i % 3], g[i % 3], b[i % 3]);
        }
        for(int idx = 0; idx < 2; idx++) {
            pca9955b_fill(pca9955b[idx], r[i % 3], g[i % 3], b[i % 3]);
        }

        uint64_t start_time = esp_timer_get_time();
        for(int j = 0; j < 4; j++) {
            for(int idx = 0; idx < 2; idx++) {
                pca9955b_show(pca9955b[idx]);
            }
        }
        for(int idx = 0; idx < 8; idx++) {
            ws2812b_show(ws2812b[idx]);
        }
        uint64_t end_time1 = esp_timer_get_time();

        for(int idx = 0; idx < 8; idx++) {
            ws2812b_wait_done(ws2812b[idx]);
        }
        uint64_t end_time2 = esp_timer_get_time();

        ESP_LOGI("main_test", "show: %lld us, wait_done: %lld us", end_time1 - start_time, end_time2 - start_time);

        vTaskDelay(pdMS_TO_TICKS(200));
    }

    for(int idx = 0; idx < 8; idx++) {
        ws2812b_del(&ws2812b[idx]);
    }

    for(int idx = 0; idx < 2; idx++) {
        pca9955b_del(&pca9955b[idx]);
    }

    i2c_del_master_bus(bus_handle);
}