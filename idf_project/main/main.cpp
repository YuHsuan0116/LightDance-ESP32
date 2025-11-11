#include "sdcard.hpp"

extern "C" void app_main(void);

void app_main(void) {
    const char* TAG = "main";
    SD_CARD sd_card;

    const char* file_hello = MOUNT_POINT "/hello.txt";
    const char* file_table = MOUNT_POINT "/table.txt";
    const char* data = "Hello\n";

    esp_err_t ret;
    ret = sd_card.sd_write_file(file_hello, data);
    if(ret != ESP_OK) {
        ESP_LOGI(TAG, "write file failed");
        return;
    }
    ESP_LOGI(TAG, "write file successed");

    char buffer[MAX_CHAR_SIZE];
    ret = sd_card.sd_read_file(file_hello, buffer);
    if(ret != ESP_OK) {
        ESP_LOGI(TAG, "read file failed");
        return;
    }
    ESP_LOGI(TAG, "read file successed");
    printf("%s\n", buffer);

    vector<tuple<uint8_t, uint8_t, uint8_t>> colors;
    ret = sd_card.sd_read_grb(file_table, colors);
    if(ret != ESP_OK) {
        ESP_LOGI(TAG, "read grb failed");
        return;
    }
    ESP_LOGI(TAG, "read grb successed");
    for(auto color: colors) {
        printf("%d %d %d\n", get<0>(color), get<1>(color), get<2>(color));
    }
}
