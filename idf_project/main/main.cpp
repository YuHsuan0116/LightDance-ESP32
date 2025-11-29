#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "LedController.hpp"

extern "C" void app_main();

uint8_t red[3] = {255, 0, 0};
uint8_t green[3] = {0, 255, 0};
uint8_t blue[3] = {0, 0, 255};

void app_main() {
    LedController controller;

    controller.load_config_test();
    controller.init();

    vTaskDelay(pdMS_TO_TICKS(1000));

    for(int i = 0; i < 100; i++) {
        controller.fill(red[i % 3], green[i % 3], blue[i % 3]);
        controller.show();
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    controller.del();
}