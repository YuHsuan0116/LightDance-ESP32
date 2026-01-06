#include "ws2812b_hal.h"

#include "string.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WS2812B_RESOLUTION 10000000

static const char* TAG = "WS2812";

/**
 * @brief Initializes the RMT TX channel for WS2812B data transmission.
 *
 * @param[in]  gpio_num   GPIO pin for data signal (Must be a valid output pin).
 * @param[in]  pixel_num  Total LED count (Used for buffer size estimation).
 * @param[out] channel    Pointer to store the created RMT channel handle.
 *
 * @return
 * - ESP_OK: Success.
 * - ESP_ERR_INVALID_ARG: Null pointer or invalid GPIO.
 * - ESP_ERR_NO_MEM: Insufficient RMT memory blocks.
 */
static esp_err_t ws2812b_init_channel(gpio_num_t gpio_num, uint16_t pixel_num, rmt_channel_handle_t* channel) {
    // Check for critical null pointer and hardware validity
    ESP_RETURN_ON_FALSE(channel, ESP_ERR_INVALID_ARG, TAG, "Channel pointer is invalid");
    ESP_RETURN_ON_FALSE(GPIO_IS_VALID_OUTPUT_GPIO(gpio_num), ESP_ERR_INVALID_ARG, TAG, "Invalid GPIO %d", gpio_num);

    rmt_tx_channel_config_t rmt_tx_channel_config = {
        .gpio_num = gpio_num,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = WS2812B_RESOLUTION,
        .mem_block_symbols = 64,
        .trans_queue_depth = 8,
        .flags.with_dma = 0,
    };

    // Attempt to create channel, auto-log error if fails
    ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&rmt_tx_channel_config, channel), TAG, "RMT create failed on GPIO %d", gpio_num);

    return ESP_OK;
}

esp_err_t ws2812b_init(gpio_num_t gpio_num, uint16_t pixel_num, ws2812b_handle_t* ws2812b) {
    esp_err_t ret = ESP_OK;
    ws2812b_dev_t* dev = NULL;

    // 1. Validation
    ESP_GOTO_ON_FALSE(ws2812b && pixel_num > 0, ESP_ERR_INVALID_ARG, err, TAG, "Invalid arguments");

    *ws2812b = NULL;  // Ensure output is clean before starting

    // 2. Allocation (Device Container)
    dev = calloc(1, sizeof(ws2812b_dev_t));
    ESP_GOTO_ON_FALSE(dev, ESP_ERR_NO_MEM, err, TAG, "Device allocation failed");
    dev->gpio_num = gpio_num;
    dev->pixel_num = pixel_num;

    // 3. Allocation (Pixel Buffer)
    dev->buffer = heap_caps_calloc(pixel_num * 3, 1, MALLOC_CAP_8BIT);
    ESP_GOTO_ON_FALSE(dev->buffer, ESP_ERR_NO_MEM, err, TAG, "Buffer allocation failed");

    // 4. RMT Encoder Setup
    ESP_GOTO_ON_ERROR(rmt_new_encoder(&dev->rmt_encoder), err, TAG, "Encoder creation failed");

    // 5. RMT Channel Setup
    ESP_GOTO_ON_ERROR(ws2812b_init_channel(gpio_num, pixel_num, &dev->rmt_channel), err, TAG, "Channel init failed");

    // 6. Enable RMT
    ESP_GOTO_ON_ERROR(rmt_enable(dev->rmt_channel), err, TAG, "RMT enable failed");

    rmt_transmit_config_t tx_config = {
        .loop_count = 0,  // Transmit once
    };
    ESP_GOTO_ON_ERROR(rmt_transmit(dev->rmt_channel, dev->rmt_encoder, dev->buffer, pixel_num * 3, &tx_config), err, TAG, "Failed to clear LEDs");
    rmt_tx_wait_all_done(dev->rmt_channel, RMT_TIMEOUT_MS);

    // Success: Assign handle and return
    *ws2812b = dev;
    ESP_LOGI(TAG, "WS2812B driver initialized (GPIO: %d, Pixels: %d)", gpio_num, pixel_num);
    return ESP_OK;

err:
    if(dev) {
        if(dev->rmt_channel) {
            rmt_del_channel(dev->rmt_channel);
        }
        if(dev->rmt_encoder) {
            rmt_del_encoder(dev->rmt_encoder);
        }
        if(dev->buffer) {
            free(dev->buffer);
        }
        free(dev);
    }

    // If user passed a non-NULL handle pointer, ensure it's NULL on failure
    if(ws2812b)
        *ws2812b = NULL;

    return ret;
}

esp_err_t ws2812b_set_pixel(ws2812b_handle_t ws2812b, int pixel_idx, uint8_t red, uint8_t green, uint8_t blue) {
    // 1. Check if handle exists
    ESP_RETURN_ON_FALSE(ws2812b, ESP_ERR_INVALID_ARG, TAG, "Handle is NULL");

    // 2. Check for Buffer Overflow (CRITICAL)
    if(pixel_idx < 0 || pixel_idx >= ws2812b->pixel_num) {
        ESP_LOGE(TAG, "Pixel index %d out of bounds (Max: %d)", pixel_idx, ws2812b->pixel_num);
        return ESP_ERR_INVALID_ARG;
    }

    // 3. Set Color (GRB Format for WS2812B)
    uint32_t offset = pixel_idx * 3;
    ws2812b->buffer[offset + 0] = green;
    ws2812b->buffer[offset + 1] = red;
    ws2812b->buffer[offset + 2] = blue;

    return ESP_OK;
}

esp_err_t ws2812b_write(ws2812b_handle_t ws2812b, uint8_t* _buffer) {
    // 1. Validate Handle
    ESP_RETURN_ON_FALSE(ws2812b, ESP_ERR_INVALID_ARG, TAG, "Handle is NULL");

    // 2. Validate Source Buffer
    ESP_RETURN_ON_FALSE(_buffer, ESP_ERR_INVALID_ARG, TAG, "Source buffer is NULL");

    // 3. Validate Internal Buffer (Defensive Programming)
    ESP_RETURN_ON_FALSE(ws2812b->buffer, ESP_ERR_INVALID_STATE, TAG, "Internal buffer is not allocated");

    // 4. Perform Fast Copy
    memcpy(ws2812b->buffer, _buffer, ws2812b->pixel_num * 3 * sizeof(uint8_t));

    return ESP_OK;
}

// Config: One-shot transmission, idle low (reset), non-blocking
static const rmt_transmit_config_t rmt_tx_config = {
    .loop_count = 0,
    .flags =
        {
            .eot_level = 0,
            .queue_nonblocking = true,
        },
};

esp_err_t ws2812b_show(ws2812b_handle_t ws2812b) {

    // 1. Basic Pointer Validation
    ESP_RETURN_ON_FALSE(ws2812b, ESP_ERR_INVALID_ARG, TAG, "Handle is NULL");

    // 2. State Validation
    ESP_RETURN_ON_FALSE(ws2812b->rmt_channel && ws2812b->rmt_encoder, ESP_ERR_INVALID_STATE, TAG, "RMT not initialized");

    // 3. Transmit
    size_t payload_size = ws2812b->pixel_num * 3;

    ESP_RETURN_ON_ERROR(
        rmt_transmit(ws2812b->rmt_channel, ws2812b->rmt_encoder, ws2812b->buffer, payload_size, &rmt_tx_config), TAG, "Failed to transmit");

    return ESP_OK;
}

esp_err_t ws2812b_del(ws2812b_handle_t* ws2812b) {
    // 1. Validate Pointer
    if(ws2812b == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(*ws2812b == NULL) {
        return ESP_OK;
    }

    ws2812b_dev_t* dev = *ws2812b;

    int saved_gpio = dev->gpio_num;

    // 2. Attempt to Turn Off LEDs (Graceful Shutdown)
    if(dev->rmt_channel && dev->buffer) {
        // Clear buffer to black
        memset(dev->buffer, 0, dev->pixel_num * 3);

        rmt_transmit_config_t tx_config = {.loop_count = 0};
        if(rmt_transmit(dev->rmt_channel, dev->rmt_encoder, dev->buffer, dev->pixel_num * 3, &tx_config) == ESP_OK) {
            rmt_tx_wait_all_done(dev->rmt_channel, RMT_TIMEOUT_MS);
        }
    }

    // 3. Teardown RMT (Best Effort)
    if(dev->rmt_channel) {
        // Disable first
        if(rmt_disable(dev->rmt_channel) != ESP_OK) {
            ESP_LOGW(TAG, "Failed to disable RMT channel during cleanup");
        }
        // Delete channel
        if(rmt_del_channel(dev->rmt_channel) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete RMT channel");
        }
    }

    if(dev->rmt_encoder) {
        if(rmt_del_encoder(dev->rmt_encoder) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete RMT encoder");
        }
    }

    // 4. Free Memory
    if(dev->buffer) {
        free(dev->buffer);
    }

    free(dev);

    // 5. Safety: Nullify the caller's pointer to prevent Use-After-Free
    *ws2812b = NULL;
    ESP_LOGI(TAG, "WS2812B (GPIO %d) de-initialized successfully", saved_gpio);

    return ESP_OK;
}

esp_err_t ws2812b_fill(ws2812b_handle_t ws2812b, uint8_t red, uint8_t green, uint8_t blue) {
    // 1. Validation
    ESP_RETURN_ON_FALSE(ws2812b, ESP_ERR_INVALID_ARG, TAG, "Handle is NULL");

    // Defensive check: Ensure buffer was allocated
    ESP_RETURN_ON_FALSE(ws2812b->buffer, ESP_ERR_INVALID_STATE, TAG, "Buffer is NULL");

    // 2. Optimization check
    // If all colors are 0 (turning off), memset is significantly faster than a loop
    if(red == 0 && green == 0 && blue == 0) {
        memset(ws2812b->buffer, 0, ws2812b->pixel_num * 3);
        return ESP_OK;
    }

    // 3. Fill Buffer
    // Loop unrolling or pointer arithmetic could optimize this, but compiler usually handles it well.
    uint8_t* ptr = ws2812b->buffer;
    for(int i = 0; i < ws2812b->pixel_num; i++) {
        *ptr++ = green;  // G
        *ptr++ = red;    // R
        *ptr++ = blue;   // B
    }

    return ESP_OK;
}

esp_err_t ws2812b_wait_done(ws2812b_handle_t ws2812b) {
    // 1. Safety Check
    ESP_RETURN_ON_FALSE(ws2812b, ESP_ERR_INVALID_ARG, TAG, "Handle is NULL");

    // 2. State Check
    ESP_RETURN_ON_FALSE(ws2812b->rmt_channel, ESP_ERR_INVALID_STATE, TAG, "RMT channel not initialized");

    // 3. Wait for Done
    return rmt_tx_wait_all_done(ws2812b->rmt_channel, RMT_TIMEOUT_MS);
}

esp_err_t ws2812b_print_buffer(ws2812b_handle_t ws2812b) {
    // 1. Validation
    ESP_RETURN_ON_FALSE(ws2812b, ESP_ERR_INVALID_ARG, TAG, "Handle is NULL");
    ESP_RETURN_ON_FALSE(ws2812b->buffer, ESP_ERR_INVALID_STATE, TAG, "Buffer is NULL");

    // 2. Log Header
    ESP_LOGI(TAG, "Dumping Buffer (%d pixels, GRB format):", ws2812b->pixel_num);

    // 3. Hex Dump
    // ESP-IDF built-in function.
    // It prints the memory address offset and data in a readable 16-byte-per-line format.
    // LOG_LEVEL_INFO ensures it only prints if the log level is appropriate.
    ESP_LOG_BUFFER_HEXDUMP(TAG, ws2812b->buffer, ws2812b->pixel_num * 3, ESP_LOG_INFO);

    return ESP_OK;
}

esp_err_t ws2812b_get_pixel(ws2812b_handle_t ws2812b, int pixel_idx, uint8_t* red, uint8_t* green, uint8_t* blue) {
    if(!ws2812b || !ws2812b->buffer || pixel_idx < 0 || pixel_idx >= ws2812b->pixel_num) {
        return ESP_ERR_INVALID_ARG;
    }
    // GRB mapping
    *green = ws2812b->buffer[pixel_idx * 3 + 0];
    *red = ws2812b->buffer[pixel_idx * 3 + 1];
    *blue = ws2812b->buffer[pixel_idx * 3 + 2];
    return ESP_OK;
}

void ws2812b_test() {
    ws2812b_handle_t ws2812b[WS2812B_NUM];
    uint8_t pixel_num = 10;

    for(int idx = 0; idx < WS2812B_NUM; idx++) {
        ws2812b_init(BOARD_HW_CONFIG.rmt_pins[idx], pixel_num, &ws2812b[idx]);
    }

    uint8_t r[3] = {15, 0, 0};
    uint8_t g[3] = {0, 15, 0};
    uint8_t b[3] = {0, 0, 15};

    for(int i = 0; i < 30; i++) {
        for(int idx = 0; idx < WS2812B_NUM; idx++) {
            for(int pixel_idx = 0; pixel_idx < pixel_num; pixel_idx++) {
                ws2812b_set_pixel(ws2812b[idx], pixel_idx, r[i % 3], g[i % 3], b[i % 3]);
            }
        }

        for(int idx = 0; idx < WS2812B_NUM; idx++) {
            ws2812b_show(ws2812b[idx]);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    for(int idx = 0; idx < WS2812B_NUM; idx++) {
        ws2812b_del(&ws2812b[idx]);
    }
}

static const char* TAG_WS_TEST = "WS2812B_TEST";
#define TEST_PIXEL_NUM 10

void ws2812b_test2() {
    esp_err_t ret = ESP_OK;

    ws2812b_handle_t ws2812b[WS2812B_NUM];

    ESP_LOGI(TAG_WS_TEST, "Initializing %d WS2812B Strips...", WS2812B_NUM);

    for(int idx = 0; idx < WS2812B_NUM; idx++) {
        gpio_num_t pin = BOARD_HW_CONFIG.rmt_pins[idx];

        ret = ws2812b_init(pin, TEST_PIXEL_NUM, &ws2812b[idx]);

        if(ret != ESP_OK) {
            ESP_LOGE(TAG_WS_TEST, "Strip [%d] (GPIO %d) Init Failed!", idx, pin);
            goto error_handling;
        }
        ESP_LOGI(TAG_WS_TEST, "Strip [%d] (GPIO %d) Init Success", idx, pin);
    }

    ESP_LOGI(TAG_WS_TEST, ">>> Pattern 1: Pixel Chaser (Check Data Line)");

    uint8_t chase_colors[3][3] = {{20, 0, 0}, {0, 20, 0}, {0, 0, 20}};

    for(int color = 0; color < 3; color++) {
        uint8_t r = chase_colors[color][0];
        uint8_t g = chase_colors[color][1];
        uint8_t b = chase_colors[color][2];

        for(int p_idx = 0; p_idx < TEST_PIXEL_NUM; p_idx++) {
            for(int s_idx = 0; s_idx < WS2812B_NUM; s_idx++) {
                for(int k = 0; k < TEST_PIXEL_NUM; k++)
                    ws2812b_set_pixel(ws2812b[s_idx], k, 0, 0, 0);

                ws2812b_set_pixel(ws2812b[s_idx], p_idx, r, g, b);

                ws2812b_show(ws2812b[s_idx]);
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    ESP_LOGI(TAG_WS_TEST, ">>> Pattern 2: Full White Blink (Check Power Supply)");

    for(int i = 0; i < 3; i++) {
        for(int s_idx = 0; s_idx < WS2812B_NUM; s_idx++) {
            for(int k = 0; k < TEST_PIXEL_NUM; k++) {
                ws2812b_set_pixel(ws2812b[s_idx], k, 50, 50, 50);
            }
            ws2812b_show(ws2812b[s_idx]);
        }
        vTaskDelay(pdMS_TO_TICKS(500));

        for(int s_idx = 0; s_idx < WS2812B_NUM; s_idx++) {
            for(int k = 0; k < TEST_PIXEL_NUM; k++) {
                ws2812b_set_pixel(ws2812b[s_idx], k, 0, 0, 0);
            }
            ws2812b_show(ws2812b[s_idx]);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG_WS_TEST, "Test Finished. Cleaning up...");

error_handling:
    for(int idx = 0; idx < WS2812B_NUM; idx++) {
        if(ws2812b[idx] != NULL) {
            for(int k = 0; k < TEST_PIXEL_NUM; k++)
                ws2812b_set_pixel(ws2812b[idx], k, 0, 0, 0);
            ws2812b_show(ws2812b[idx]);

            ESP_LOGI(TAG_WS_TEST, "Deleting Strip [%d]", idx);
            ws2812b_del(&ws2812b[idx]);
        }
    }
    ESP_LOGI(TAG_WS_TEST, "WS2812B Test Complete.");
}