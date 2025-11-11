#pragma once

#include <string>
#include <tuple>
#include <vector>
#include "driver/sdmmc_host.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

using namespace std;

#define MOUNT_POINT "/sdcard"
#define MAX_CHAR_SIZE 64

class SD_CARD {
  public:
    SD_CARD();
    ~SD_CARD();

    void print_card_info();
    esp_err_t sd_write_file(const char*, const char*);
    esp_err_t sd_read_file(const char*, char*);
    esp_err_t sd_read_grb(const char*, vector<tuple<uint8_t, uint8_t, uint8_t>>&);

  private:
    sdmmc_host_t host;
    sdmmc_card_t* card;
    sdmmc_slot_config_t slot_config;
    esp_vfs_fat_mount_config_t mount_config;
    const string TAG = "SD_CARD";
};