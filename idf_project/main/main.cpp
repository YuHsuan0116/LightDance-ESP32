#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include "def.h"
#include "led_driver/led_driver.h"

extern "C" void app_main();

#define N_STRIP_CH 1
#define N_OF_CH 5

void config_led_driver(LedDriver_handle_t* LedDriver) {
    led_config_t ch_configs[N_STRIP_CH + N_OF_CH];

    uint8_t pca9955b_addresses[1] = {0x5e};

    for(int i = 0; i < N_STRIP_CH; i++) {
        ch_configs[i].type = LED_TYPE_STRIP;
        ch_configs[i].led_count = 100;
        ch_configs[i].rmt_gpio = 12;
    }

    for(int i = 0; i < N_OF_CH; i++) {
        ch_configs[N_STRIP_CH + i].type = LED_TYPE_OF;
        ch_configs[N_STRIP_CH + i].i2c_addr = pca9955b_addresses[i / 5];
        ch_configs[N_STRIP_CH + i].pca_channel = i % 5;
    }
    LedDriver_config(ch_configs, N_STRIP_CH + N_OF_CH, LedDriver);
}

uint8_t r[7] = {255, 255, 255, 0, 0, 0, 139};
uint8_t g[7] = {0, 127, 255, 255, 255, 0, 0};
uint8_t b[7] = {0, 0, 0, 0, 255, 255, 255};

void app_main(void) {
    LedDriver_handle_t LedDriver = {0};
    LedDriver_init(&LedDriver);
    config_led_driver(&LedDriver);
    uint8_t t = 0;
    while(1) {
        LedDriver_set_rgb(r[t % 7], g[t % 7], b[t % 7], &LedDriver);
        t += 1;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
