#include <stdlib.h>
#include <time.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "LedController.hpp"

#define WS2812B_NUM 4
#define PCA9955B_NUM 2

extern "C" void app_main();

uint8_t red[4] = {255, 0, 0, 255};
uint8_t green[4] = {0, 255, 0, 255};
uint8_t blue[4] = {0, 0, 255, 255};

void test1();

void app_main() {
    LedController controller;

    controller.load_config_test();
    controller.init();

    vTaskDelay(pdMS_TO_TICKS(1000));

    for(int i = 0; i < 100; i++) {
        controller.fill(red[i % 4], green[i % 4], blue[i % 4]);
        controller.show();

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    controller.del();
}
