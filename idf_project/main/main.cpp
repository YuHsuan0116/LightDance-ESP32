#include "esp_timer.h"
#include "sdcard.hpp"

extern "C" void app_main(void);

int64_t start_time, end_time;
vector<tuple<uint8_t, uint8_t, uint8_t>> tests;
const char* rgb800 = MOUNT_POINT "/rgb800.txt";
const char* rgb2400 = MOUNT_POINT "/rgb2400.txt";

void test800(SD_CARD* card) {
    card->sd_read_grb(rgb800, tests);
    card->sd_read_grb(rgb800, tests);
    card->sd_read_grb(rgb800, tests);
}

void test2400(SD_CARD* card) {
    card->sd_read_grb(rgb2400, tests);
}

void app_main(void) {
    const char* TAG = "main";
    SD_CARD sd_card;

    start_time = esp_timer_get_time();
    test800(&sd_card);
    end_time = esp_timer_get_time();
    ESP_LOGI(TAG, "test800 takes %lld us", (end_time - start_time));

    start_time = esp_timer_get_time();
    test2400(&sd_card);
    end_time = esp_timer_get_time();
    ESP_LOGI(TAG, "test2400 takes %lld us", (end_time - start_time));
}
