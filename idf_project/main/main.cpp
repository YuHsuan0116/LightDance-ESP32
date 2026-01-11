#include "color.h"
#include "console.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
// #include "player.h"

#include "framebuffer.h"

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

void app_main() {
    for(int i = 0; i < WS2812B_NUM; i++) {
        ch_info.rmt_strips[i] = WS2812B_MAX_PIXEL_NUM;
    }
    for(int i = 0; i < PCA9955B_CH_NUM; i++) {
        ch_info.i2c_leds[i] = 1;
    }

    controller.init();
    fb.init();

    while(esp_timer_get_time() <= 10 * 1000 * 1000) {
        controller.show();

        fb.compute(esp_timer_get_time() / 1000);
        fb.render(controller);
    }

    ESP_LOGI("main", "end!");

    controller.deinit();
}