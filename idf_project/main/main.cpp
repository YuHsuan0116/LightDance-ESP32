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
    for(int i = 0; i < N_SIGNAL; i++) {
        event.type = event_type[i];
        event.master_timer = esp_timer_get_time();

        ESP_LOGI("singal_task", "send signal!");
        player->sendEvent(&event);
        vTaskDelay(pdMS_TO_TICKS(1000 * event_interval[i]));
    }

    vTaskDelete(NULL);
}

Player player(40);

void app_main(void) {
    player.init();

    xTaskCreate(player_task, "player_task", 2048, (void*)&player, 5, NULL);
    xTaskCreate(signal_task, "signal_task", 2048, (void*)&player, 5, NULL);
}