#include <stdlib.h>
#include <time.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "LedController.hpp"

uint8_t red[4] = {255, 0, 0, 255};
uint8_t green[4] = {0, 255, 0, 255};
uint8_t blue[4] = {0, 0, 255, 255};

void example_main() {
    LedController controller;

    controller.load_config_test();
    controller.init();

    vTaskDelay(pdMS_TO_TICKS(1000));

    for(int i = 0; i < 100; i++) {
        controller.fill(red[i % 4], green[i % 4], blue[i % 4]);
        controller.show();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    controller.del();
}
