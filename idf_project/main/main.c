#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include "esp_timer.h"

#include "LedDriver.h"

#define N_PCA9955B 0
#define N_STRIP_CH 8
#define N_OF_CH 0
#define N_COLOR 7

#define MAXIMUM_BRIGHTNESS 15

led_config_t ch_configs[N_STRIP_CH + N_OF_CH];

uint8_t pca9955b_addresses[N_PCA9955B] = {};
uint8_t ws2812b_gpio[N_STRIP_CH] = {16, 17, 18, 19, 21, 22, 23, 25};

uint8_t r[N_COLOR] = {255, 255, 255, 0, 0, 0, 139};
uint8_t g[N_COLOR] = {0, 127, 255, 255, 255, 0, 0};
uint8_t b[N_COLOR] = {0, 0, 0, 0, 255, 255, 255};

color_t strip_frame[N_STRIP_CH][WS2812B_MAXIMUM_LED_COUNT];
color_t of_frame[N_OF_CH][1];
color_t* cplt_frame[N_STRIP_CH + N_OF_CH];

color_t GREEN = {255, 0, 0};
color_t RED = {0, 255, 0};
color_t BLUE = {0, 0, 255};

color_t colors[N_COLOR];

int count = 0;
uint64_t start_time, end_time;

void config_led_driver(LedDriver_handle_t* LedDriver);

void app_main(void) {
    LedDriver_handle_t LedDriver = {0};
    LedDriver_init(&LedDriver);
    config_led_driver(&LedDriver);

    while(1) {
        count++;
        for(int i = 0; i < N_STRIP_CH; i++) {
            for(int j = 0; j < WS2812B_MAXIMUM_LED_COUNT; j++) {
                strip_frame[i][j] = colors[((i + j - count) % N_COLOR + N_COLOR) % N_COLOR];
            }
        }
        for(int i = 0; i < N_OF_CH; i++) {
            of_frame[i][0] = colors[((i - count) % N_COLOR + N_COLOR) % N_COLOR];
        }

        start_time = esp_timer_get_time();
        LedDriver_write(cplt_frame, &LedDriver);
        end_time = esp_timer_get_time();
        ESP_LOGI("main", "%lld us", end_time - start_time);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    LedDriver_del(&LedDriver);
}

void config_led_driver(LedDriver_handle_t* LedDriver) {
    for(int i = 0; i < N_STRIP_CH; i++) {
        ch_configs[i].type = LED_TYPE_STRIP;
        ch_configs[i].led_count = 100;
        ch_configs[i].rmt_gpio = ws2812b_gpio[i];
    }

    for(int i = 0; i < N_OF_CH; i++) {
        ch_configs[N_STRIP_CH + i].type = LED_TYPE_OF;
        ch_configs[N_STRIP_CH + i].i2c_addr = pca9955b_addresses[i / 5];
        ch_configs[N_STRIP_CH + i].pca_channel = i % 5;
    }

    LedDriver_config(ch_configs, N_STRIP_CH + N_OF_CH, LedDriver);

    for(int i = 0; i < N_STRIP_CH; i++) {
        cplt_frame[i] = strip_frame[i];
    }
    for(int i = 0; i < N_OF_CH; i++) {
        cplt_frame[N_STRIP_CH + i] = of_frame[i];
    }

    for(int i = 0; i < N_COLOR; i++) {
        colors[i].green = (g[i] * MAXIMUM_BRIGHTNESS) / 255;
        colors[i].red = (r[i] * MAXIMUM_BRIGHTNESS) / 255;
        colors[i].blue = (b[i] * MAXIMUM_BRIGHTNESS) / 255;
    }
}