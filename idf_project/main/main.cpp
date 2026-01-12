#include "color.h"
#include "console.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
// #include "player.h"

#include "framebuffer.h"
#include "player_v2.hpp"

extern "C" void app_main();

#define seg 256
#define BRIGHTNESS 127

grb8_t red = {.g = 0, .r = BRIGHTNESS, .b = 0};
grb8_t green = {.g = BRIGHTNESS, .r = 0, .b = 0};
grb8_t blue = {.g = 0, .r = 0, .b = BRIGHTNESS};

frame_data frame;
grb8_t pool[3] = {
    red,
    green,
    blue,
};

static LedController controller;
static FrameBuffer fb;

size_t a = sizeof(controller);
size_t b = sizeof(FrameBuffer);

TaskHandle_t t;

void test(void* parmeter) {}

void app_main() {

    start_console();

    // xTaskCreate(test, NULL, 2048, NULL, 5, &t);

    // vTaskDelay(pdMS_TO_TICKS(1000));
    // p.test(63, 0, 0);
}