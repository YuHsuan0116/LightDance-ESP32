#include "color.h"
#include "console.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "player.h"

#include "framebuffer.h"

extern "C" void app_main();

#define seg 256
#define BRIGHTNESS 127

grb8_t red = {.g = 0, .r = BRIGHTNESS, .b = 0};
grb8_t green = {.g = BRIGHTNESS, .r = 0, .b = 0};
grb8_t blue = {.g = 0, .r = 0, .b = BRIGHTNESS};

void test1() {
    ws2812b_handle_t ws1, ws2;
    ws2812b_init(GPIO_NUM_32, 300, &ws1);
    ws2812b_init(GPIO_NUM_25, 5, &ws2);

    // ws2812b_fill(ws1, BRIGHTNESS, BRIGHTNESS, BRIGHTNESS);
    // ws2812b_show(ws1);

    // uint64_t start = esp_timer_get_time();

    for(int j = 0; j < 10; j++) {
        for(int i = 0; i <= seg; i++) {
            grb8_t c = grb_lerp_hsv_u8(red, green, (uint16_t)i * 255 / seg);
            grb8_t c2 = grb_lerp_u8(red, green, (uint16_t)i * 255 / seg);
            ws2812b_fill(ws1, c.r, c.g, c.b);
            ws2812b_show(ws1);
            ws2812b_fill(ws2, c2.r, c2.g, c2.b);
            ws2812b_show(ws2);
            vTaskDelay(pdMS_TO_TICKS(10));
            ESP_EARLY_LOGI("main", "seg%d, c,  r: %d, g: %d, b: %d", i, c.r, c.g, c.b);
            ESP_EARLY_LOGI("main", "seg%d, c2, r: %d, g: %d, b: %d", i, c2.r, c2.g, c2.b);
        }
        for(int i = 0; i <= seg; i++) {
            grb8_t c = grb_lerp_hsv_u8(green, blue, (uint16_t)i * 255 / seg);
            grb8_t c2 = grb_lerp_u8(green, blue, (uint16_t)i * 255 / seg);

            ws2812b_fill(ws1, c.r, c.g, c.b);
            ws2812b_show(ws1);
            ws2812b_fill(ws2, c2.r, c2.g, c2.b);
            ws2812b_show(ws2);
            vTaskDelay(pdMS_TO_TICKS(10));
            ESP_EARLY_LOGI("main", "seg%d, c, r: %d, g: %d, b: %d", i, c.r, c.g, c.b);
            ESP_EARLY_LOGI("main", "seg%d, c2, r: %d, g: %d, b: %d", i, c2.r, c2.g, c2.b);
        }
        for(int i = 0; i <= seg; i++) {
            grb8_t c = grb_lerp_hsv_u8(blue, red, (uint16_t)i * 255 / seg);
            grb8_t c2 = grb_lerp_u8(blue, red, (uint16_t)i * 255 / seg);

            ws2812b_fill(ws1, c.r, c.g, c.b);
            ws2812b_show(ws1);
            ws2812b_fill(ws2, c2.r, c2.g, c2.b);
            ws2812b_show(ws2);
            vTaskDelay(pdMS_TO_TICKS(10));
            ESP_EARLY_LOGI("main", "seg%d, c, r: %d, g: %d, b: %d", i, c.r, c.g, c.b);
            ESP_EARLY_LOGI("main", "seg%d, c2, r: %d, g: %d, b: %d", i, c2.r, c2.g, c2.b);
        }
    }

    // uint64_t end = esp_timer_get_time();
    // ESP_LOGI("main", "take: %llu us", end - start);

    ws2812b_del(&ws1);
    ws2812b_del(&ws2);
}

frame_data frame;
grb8_t pool[3] = {
    red,
    green,
    blue,
};

void app_main() {

    for(int i = 0; i < WS2812B_NUM; i++) {
        ch_info.rmt_strips[i] = 100;
    }
    for(int i = 0; i < PCA9955B_CH_NUM; i++) {
        ch_info.i2c_leds[i] = 1;
    }

    // ws2812b_handle_t ws[WS2812B_NUM];
    // for(int i = 0; i < WS2812B_NUM; i++) {
    //     ws2812b_init(BOARD_HW_CONFIG.rmt_pins[i], 100, &ws[i]);
    // }

    // LedController controller;
    // controller.init();

    // for(int i = 0; i < 100; i++) {
    // for(int j = 0; j < 100; j++) {
    //     frame.ws2812b[0][j] = pool[i % 3];
    // }

    // for(int j = 0; j < WS2812B_NUM; j++) {
    //     ws2812b_write(ws[j], (uint8_t*)frame.ws2812b[0]);
    //     // ws2812b_fill(ws[j], pool[i % 3].r, pool[i % 3].g, pool[i % 3].b);
    // }

    // controller.fill(pool[i % 3].r, pool[i % 3].g, pool[i % 3].b);

    // uint64_t start = esp_timer_get_time();
    // for(int j = 0; j < WS2812B_NUM / 2; j++) {
    //     ws2812b_show(ws[j]);
    // }

    // for(int j = 0; j < WS2812B_NUM / 2; j++) {
    //     ws2812b_wait_done(ws[j]);
    // }

    // for(int j = 8 / 2; j < 8; j++) {
    //     ws2812b_show(ws[j]);
    // }

    // for(int j = 8 / 2; j < 8; j++) {
    //     ws2812b_wait_done(ws[j]);
    // }
    //     controller.show();

    //     uint64_t end = esp_timer_get_time();
    //     ESP_LOGI("main", "%llu us", end - start);

    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }

    // controller.deinit();

    // for(int i = 0; i < WS2812B_NUM; i++) {
    //     ws2812b_del(&ws[i]);
    // }

    // for(int i = 0; i < PCA9955B_CH_NUM; i++) {
    //     ch_info.i2c_leds[i] = 1;
    // }

    static LedController controller;
    controller.init();

    static FrameBuffer fb;

    uint64_t start = esp_timer_get_time();
    for(int i = 0; i < 1000; i++) {
        uint64_t t1 = esp_timer_get_time();
        controller.show();
        uint64_t t2 = esp_timer_get_time();
        fb.compute((esp_timer_get_time() - start) / 1000);
        uint64_t t3 = esp_timer_get_time();
        fb.render(controller);
        uint64_t t4 = esp_timer_get_time();
        // ESP_LOGI("main", "%llu us, %llu us, %llu us", t2 - t1, t3 - t2, t4 - t3);
        // fb.print_buffer();
        // vTaskDelay(pdMS_TO_TICKS(10));
    }

    controller.deinit();

    // static table_frame_t frame;

    // test_read_frame(&frame);
    // print_table_frame(frame);

    // test_read_frame(&frame);
    // print_table_frame(frame);
}