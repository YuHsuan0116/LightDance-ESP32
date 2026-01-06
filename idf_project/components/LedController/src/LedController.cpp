#include "LedController.hpp"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "BoardConfig.h"
#include "pca9955b_hal.h"
#include "ws2812b_hal.h"

#include "string.h"

static const char* TAG = "LedController";

LedController::LedController() {}

LedController::~LedController() {}

esp_err_t LedController::init(ch_info_t _ch_info) {
    esp_err_t ret = ESP_OK;
    ch_info = _ch_info;

    // 1. Input Validation
    ESP_RETURN_ON_FALSE(GPIO_IS_VALID_GPIO(GPIO_NUM_21), ESP_ERR_INVALID_ARG, TAG, "Invalid SDA GPIO");
    ESP_RETURN_ON_FALSE(GPIO_IS_VALID_GPIO(GPIO_NUM_22), ESP_ERR_INVALID_ARG, TAG, "Invalid SCL GPIO");

    // 2. Initialize output handles to 0
    memset(ws2812b_devs, 0, sizeof(ws2812b_devs));
    memset(pca9955b_devs, 0, sizeof(pca9955b_devs));
    bus_handle = NULL;

    // 3. Initialize I2C Bus
    ESP_GOTO_ON_ERROR(i2c_bus_init(GPIO_NUM_21, GPIO_NUM_22, &bus_handle), err, TAG, "Failed to initialize I2C bus");

    // 4. Initialize WS2812B Strips
    for(int i = 0; i < WS2812B_NUM; i++) {
        ESP_GOTO_ON_ERROR(
            ws2812b_init(BOARD_HW_CONFIG.rmt_pins[i], ch_info.rmt_strips[i], &ws2812b_devs[i]), err, TAG, "Failed to init WS2812B[%d]", i);
    }

    // 5. Initialize PCA9955B Chips
    for(int i = 0; i < PCA9955B_NUM; i++) {
        ESP_GOTO_ON_ERROR(pca9955b_init(BOARD_HW_CONFIG.i2c_addrs[i], bus_handle, &pca9955b_devs[i]), err, TAG, "Failed to init PCA9955B[%d]", i);
    }

    ESP_LOGI(TAG, "LedController initialized successfully");
    return ESP_OK;

err:
    // If init failed, cleanup whatever was allocated
    deinit();
    return ret;
}

esp_err_t LedController::write_buffer(int ch_idx, uint8_t* data) {
    // 1. Validate Input
    ESP_RETURN_ON_FALSE(data, ESP_ERR_INVALID_ARG, TAG, "Data buffer is NULL");

    // 2. Handle WS2812B Strips (Indices 0 to WS2812B_NUM - 1)
    if(ch_idx < WS2812B_NUM) {
        // Ensure the device handle is valid before writing
        ESP_RETURN_ON_FALSE(ws2812b_devs[ch_idx], ESP_ERR_INVALID_STATE, TAG, "WS2812B[%d] not initialized", ch_idx);

        // Pass the full strip buffer to the HAL
        return ws2812b_write(ws2812b_devs[ch_idx], data);
    }

    if(ch_idx >= WS2812B_NUM) {
        int relative_idx = ch_idx - WS2812B_NUM;

        // Calculate device and pixel index (5 LEDs per PCA9955B chip)
        int dev_idx = relative_idx / 5;
        int pixel_idx = relative_idx % 5;

        // Validate PCA device bounds
        if(dev_idx >= PCA9955B_NUM) {
            ESP_LOGE(TAG, "Channel index %d out of range (Max PCA dev: %d)", ch_idx, PCA9955B_NUM - 1);
            return ESP_ERR_INVALID_ARG;
        }

        // Ensure the device handle is valid
        ESP_RETURN_ON_FALSE(pca9955b_devs[dev_idx], ESP_ERR_INVALID_STATE, TAG, "PCA9955B[%d] not initialized", dev_idx);

        // Convert Data Format (Assume Input is GRB, Output to PCA is RGB)
        // data[0] = Green, data[1] = Red, data[2] = Blue
        uint8_t green = data[0];
        uint8_t red = data[1];
        uint8_t blue = data[2];

        return pca9955b_set_pixel(pca9955b_devs[dev_idx], pixel_idx, red, green, blue);
    }

    return ESP_ERR_INVALID_ARG;
}

esp_err_t LedController::show() {
    esp_err_t ret = ESP_OK;
    esp_err_t err = ESP_OK;

#if SHOW_TIME_PER_FRAME
    uint64_t start = esp_timer_get_time();
#endif

    // 1. Trigger WS2812B transmission (Asynchronous/Non-blocking)
    for(int i = 0; i < WS2812B_NUM; i++) {
        if(ws2812b_devs[i]) {
            err = ws2812b_show(ws2812b_devs[i]);
            if(err != ESP_OK) {
                // Log error but continue to try updating other LEDs
                ESP_LOGE(TAG, "Failed to show WS2812B[%d]: %s", i, esp_err_to_name(err));
                ret = err;  // Latch the error code
            }
        }
    }

    // 2. Trigger PCA9955B transmission (Synchronous/Blocking)
    for(int i = 0; i < PCA9955B_NUM; i++) {
        if(pca9955b_devs[i]) {
            err = pca9955b_show(pca9955b_devs[i]);
            if(err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to show PCA9955B[%d]: %s", i, esp_err_to_name(err));
                ret = err;
            }
        }
    }

    // 3. Wait for WS2812B transmission to complete
    for(int i = 0; i < WS2812B_NUM; i++) {
        if(ws2812b_devs[i]) {
            err = ws2812b_wait_done(ws2812b_devs[i]);
            if(err != ESP_OK) {
                ESP_LOGE(TAG, "Wait done failed for WS2812B[%d]: %s", i, esp_err_to_name(err));
                ret = err;
            }
        }
    }

#if SHOW_TIME_PER_FRAME
    end = esp_timer_get_time();
    ESP_LOGI(TAG, "show() execution time: %llu us", (end - start));
#endif

    // Return the last error encountered, or ESP_OK if all went well
    return ret;
}

esp_err_t LedController::deinit() {
    ESP_LOGI(TAG, "De-initializing LED Controller...");

    // 1. Free WS2812B Devices
    for(int i = 0; i < WS2812B_NUM; i++) {
        if(ws2812b_del(&(ws2812b_devs[i])) != ESP_OK) {
            ESP_LOGW(TAG, "Error deleting WS2812B[%d]", i);
        }
    }

    // 2. Free PCA9955B Devices
    for(int i = 0; i < PCA9955B_NUM; i++) {
        if(pca9955b_del(&(pca9955b_devs[i])) != ESP_OK) {
            ESP_LOGW(TAG, "Error deleting PCA9955B[%d]", i);
        }
    }

    // 3. Free I2C Bus
    if(bus_handle != NULL) {
        esp_err_t err = i2c_del_master_bus(bus_handle);
        if(err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete I2C bus: %s", esp_err_to_name(err));
        }
        bus_handle = NULL;  // Prevent double-free if deinit is called again
    }

    ESP_LOGI(TAG, "De-initialization complete");
    return ESP_OK;
}

esp_err_t LedController::fill(uint8_t red, uint8_t green, uint8_t blue) {
    esp_err_t ret = ESP_OK;
    esp_err_t err = ESP_OK;

    // 1. Fill WS2812B Strips
    for(int i = 0; i < WS2812B_NUM; i++) {
        // Ensure the device handle is valid before operation
        if(ws2812b_devs[i] != NULL) {
            err = ws2812b_fill(ws2812b_devs[i], red, green, blue);

            if(err != ESP_OK) {
                ESP_LOGW(TAG, "Failed to fill WS2812B[%d]: %s", i, esp_err_to_name(err));
                ret = err;
            }
        }
    }

    // 2. Fill PCA9955B Chips
    for(int i = 0; i < PCA9955B_NUM; i++) {
        // Ensure the device handle is valid
        if(pca9955b_devs[i] != NULL) {
            err = pca9955b_fill(pca9955b_devs[i], red, green, blue);

            if(err != ESP_OK) {
                ESP_LOGW(TAG, "Failed to fill PCA9955B[%d]: %s", i, esp_err_to_name(err));
                ret = err;
            }
        }
    }

    // Return ESP_OK only if all devices succeeded, otherwise return the last error code
    return ret;
}

esp_err_t LedController::black_out() {
    // 1. Clear internal buffers to Black (0, 0, 0)

    esp_err_t ret = fill(0, 0, 0);

    if(ret != ESP_OK) {
        ESP_LOGW(TAG, "Blackout fill incomplete: %s", esp_err_to_name(ret));
    }

    // 2. Flush changes to hardware immediately
    esp_err_t ret_show = show();

    // 3. Return the first error encountered
    if(ret != ESP_OK) {
        return ret;
    }

    return ret_show;
}

void LedController::print_buffer() {
    for(int i = 0; i < WS2812B_NUM; i++) {
        ws2812b_print_buffer(ws2812b_devs[i]);
    }
}

const uint8_t RGB_COLORS[3][3] = {
    {31, 0, 0},  // Red
    {0, 31, 0},  // Green
    {0, 0, 31}   // Blue
};

void Controller_test() {
    static const char* TAG_TEST = "PATTERN_TEST";

    LedController controller;
    ch_info_t ch_info = {0};
    esp_err_t ret;

    ESP_LOGI(TAG_TEST, "Starting Pattern Test (4 Frames, R-G-B Interleaved)...");

    for(int i = 0; i < WS2812B_NUM; i++) {
        ch_info.rmt_strips[i] = 10;
    }

    for(int i = 0; i < PCA9955B_CH_NUM; i++) {
        ch_info.i2c_leds[i] = 1;
    }

    ret = controller.init(ch_info);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG_TEST, "Init failed");
        return;
    }

    for(int frame = 0; frame < 20; frame++) {
        ESP_LOGI(TAG_TEST, "Displaying Frame %d", frame);

        uint8_t strip_buffer[10 * 3];

        for(int strip_idx = 0; strip_idx < WS2812B_NUM; strip_idx++) {
            for(int pixel_idx = 0; pixel_idx < 10; pixel_idx++) {
                int color_idx = (strip_idx + frame) % 3;
                strip_buffer[pixel_idx * 3 + 0] = RGB_COLORS[color_idx][1];  // Green
                strip_buffer[pixel_idx * 3 + 1] = RGB_COLORS[color_idx][0];  // Red
                strip_buffer[pixel_idx * 3 + 2] = RGB_COLORS[color_idx][2];  // Blue
            }
            controller.write_buffer(strip_idx, strip_buffer);
        }

        for(int i = 0; i < PCA9955B_NUM; i++) {
            for(int pixel_idx = 0; pixel_idx < 5; pixel_idx++) {
                int ch_idx = WS2812B_NUM + 5 * i + pixel_idx;
                int color_idx = (i + frame) % 3;

                uint8_t pixel_data[3];
                pixel_data[0] = RGB_COLORS[color_idx][1];  // Green
                pixel_data[1] = RGB_COLORS[color_idx][0];  // Red
                pixel_data[2] = RGB_COLORS[color_idx][2];  // Blue

                controller.write_buffer(ch_idx, pixel_data);
            }
        }

        controller.show();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG_TEST, "Test Finished.");
    controller.deinit();
}