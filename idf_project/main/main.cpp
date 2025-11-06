#include <driver/sdmmc_defs.h>
#include <driver/sdmmc_host.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdmmc_cmd.h>
#include <stdio.h>


extern "C" void app_main(void);

static const char* TAG = "SDMMC";

void app_main(void) {
    esp_err_t ret;

    // --- Step 1. Configure SDMMC host ---
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;  // safer for initialization

    // --- Step 2. Configure slot ---
    // Slot 1 (the one available on ESP32-WROOM) supports only 1-bit mode
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;              // only D0 line (GPIO2)
    slot.cd = SDMMC_SLOT_NO_CD;  // no card-detect pin
    slot.wp = SDMMC_SLOT_NO_WP;  // no write-protect pin

    // --- Step 3. Initialize host and slot ---
    ret = sdmmc_host_init();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "sdmmc_host_init failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = sdmmc_host_init_slot(SDMMC_HOST_SLOT_1, &slot);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "sdmmc_host_init_slot failed: %s", esp_err_to_name(ret));
        sdmmc_host_deinit();
        return;
    }

    // --- Step 4. Probe the card ---
    sdmmc_card_t card;
    ret = sdmmc_card_init(&host, &card);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Card init failed (0x%x): %s", ret, esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SD card initialized successfully!");
        sdmmc_card_print_info(stdout, &card);
    }

    // --- Step 5. Check real frequency ---
    int real_freq_khz = 0;
    sdmmc_host_get_real_freq(SDMMC_HOST_SLOT_1, &real_freq_khz);
    ESP_LOGI(TAG, "Actual SDMMC clock frequency: %d kHz", real_freq_khz);

    // --- Step 6. Deinitialize ---
    sdmmc_host_deinit_slot(SDMMC_HOST_SLOT_1);
    sdmmc_host_deinit();
}
