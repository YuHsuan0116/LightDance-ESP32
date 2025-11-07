#include <string.h>
#include <tuple>
#include <vector>
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

extern "C" void app_main(void);

#define MOUNT_POINT "/sdcard"
#define MAX_CHAR_SIZE 256
class SD_CARD {
  public:
    SD_CARD() {
        host = SDMMC_HOST_DEFAULT();

        slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
        slot_config.width = 4;
        slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

        esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024,

        };

        esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    }
    ~SD_CARD() {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    }

    void print_card_info() {
        sdmmc_card_print_info(stdout, card);
    }

    esp_err_t sd_write_file(const char* path, const char* data) {
        FILE* f = fopen(path, "w");
        if(f == NULL) {
            return ESP_FAIL;
        }

        fprintf(f, data);
        fclose(f);

        return ESP_OK;
    }

    esp_err_t sd_read_file(const char* path, char* buffer) {
        FILE* f = fopen(path, "r");
        if(f == NULL) {
            return ESP_FAIL;
        }

        fgets(buffer, MAX_CHAR_SIZE, f);
        fclose(f);

        char* pos = strchr(buffer, '\n');
        if(pos) {
            *pos = '\0';
        }

        return ESP_OK;
    }

    esp_err_t sd_read_grb(const char* path, std::vector<std::tuple<int, int, int>>& vec) {
        FILE* f = fopen(path, "r");
        if(f == NULL) {
            return ESP_FAIL;
        }

        vec.clear();

        int r, g, b;
        while(fscanf(f, "%d %d %d", &g, &r, &b) == 3) {
            vec.push_back(std::make_tuple(g, r, b));
        }

        fclose(f);
        return ESP_OK;
    }

  private:
    sdmmc_host_t host;
    sdmmc_slot_config_t slot_config;
    sdmmc_card_t* card;
    esp_vfs_fat_mount_config_t mount_config;
    const char* TAG = "SD_CARD";
};

void app_main(void) {
    const char* TAG = "main";
    SD_CARD sd_card;
    sd_card.print_card_info();

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
    printf("%s\n", buffer);

    std::vector<std::tuple<int, int, int>> colors;
    ret = sd_card.sd_read_grb(file_table, colors);
    for(auto color: colors) {
        printf("%d %d %d\n", std::get<0>(color), std::get<1>(color), std::get<2>(color));
    }
}
