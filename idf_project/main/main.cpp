#include "driver/gptimer.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "player.hpp"

#define N_SIGNAL 8

extern "C" void app_main();

event_type_t event_type[N_SIGNAL] = {
    EVENT_START,
    EVENT_PLAY,
    EVENT_PAUSE,
    EVENT_PLAY,
    EVENT_PAUSE,
    EVENT_PLAY,
    EVENT_PAUSE,
    EVENT_STOP,
};
int event_interval[N_SIGNAL] = {1, 5, 3, 5, 3, 5, 3, 5};

void signal_task(void* pvParameters) {
    Player* player = (Player*)pvParameters;
    event_handle_t event;
    for(int j = 0; j < 2; j++) {
        for(int i = 0; i < N_SIGNAL; i++) {
            event.type = event_type[i];
            event.master_timer = esp_timer_get_time();

            ESP_LOGI("singal_task", "send signal!");
            player->queueEvent(&event);
            vTaskDelay(pdMS_TO_TICKS(500 * event_interval[i]));
        }
    }
    event.type = EVENT_DELETE;
    event.master_timer = esp_timer_get_time();
    ESP_LOGI("singal_task", "send signal!");
    player->queueEvent(&event);
    vTaskDelay(pdMS_TO_TICKS(1000));
    vTaskDelete(NULL);
}

void app_main(void) {
    uint8_t ws2812b_gpio[8] = {16, 17, 18, 19, 26, 27, 32, 33};
    uint8_t pca9955b_addresses[8] = {0x5e, 0x5c};

    static player_config_t player_config;

    player_config.fps = 30;
    player_config.wait_done = true;
    player_config.maximum_brightness = 15;

    player_config.ws2812b_count = 8;
    for(int i = 0; i < player_config.ws2812b_count; i++) {
        player_config.ws2812b_gpio[i] = ws2812b_gpio[i];
    }
    player_config.pca9955b_count = 2;
    player_config.pca9955b_ch_count = 10;
    for(int i = 0; i < (player_config.pca9955b_count); i++) {
        player_config.pca9955b_addresses[i] = pca9955b_addresses[i];
    }

    static Player player(player_config);
    player.init();

    vTaskDelay(pdMS_TO_TICKS(500));
    xTaskCreate(signal_task, "signal_task", 2048, &player, 5, NULL);
}