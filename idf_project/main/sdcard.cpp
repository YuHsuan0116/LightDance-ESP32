#include "sdcard.hpp"

SD_CARD::SD_CARD() {
    host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    print_card_info();
}

SD_CARD::~SD_CARD() {
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
}

void SD_CARD::print_card_info() {
    ESP_LOGI("SD", "Name: %s", card->cid.name);
    ESP_LOGI("SD", "Capacity: %llu MB", (uint64_t)card->csd.capacity * card->csd.sector_size / (1024 * 1024));
    ESP_LOGI("SD", "Speed: %u kbit/s", card->csd.tr_speed / 1000);
    ESP_LOGI("SD", "Manufacturer ID: %d", card->cid.mfg_id);
    ESP_LOGI("SD", "OEM ID: %d", card->cid.oem_id);
}

esp_err_t SD_CARD::sd_write_file(const char* path, const char* data) {
    FILE* f = fopen(path, "w");
    if(f == NULL) {
        return ESP_FAIL;
    }

    fprintf(f, data);
    fclose(f);

    return ESP_OK;
}

esp_err_t SD_CARD::sd_read_file(const char* path, char* buffer) {
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

esp_err_t SD_CARD::sd_read_grb(const char* path, vector<tuple<uint8_t, uint8_t, uint8_t>>& vec) {
    FILE* f = fopen(path, "r");
    if(f == NULL) {
        return ESP_FAIL;
    }

    vec.clear();

    int r, g, b;
    while(fscanf(f, "%u %u %u", &g, &r, &b) == 3) {
        vec.push_back(std::make_tuple(g, r, b));
    }

    fclose(f);
    return ESP_OK;
}