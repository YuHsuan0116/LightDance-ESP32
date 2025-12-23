#include "ws2812b_hal.h"

#include "string.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WS2812B_RESOLUTION 10000000

static esp_err_t ws2812b_init_channel(gpio_num_t gpio_num, uint16_t pixel_num, rmt_channel_handle_t* channel) {
    rmt_tx_channel_config_t rmt_tx_channel_config = {
        .gpio_num = gpio_num,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = WS2812B_RESOLUTION,
        .mem_block_symbols = 512 / WS2812B_NUM,
        .trans_queue_depth = 8,
    };

    return rmt_new_tx_channel(&rmt_tx_channel_config, channel);
}

esp_err_t ws2812b_init(gpio_num_t gpio_num, uint16_t pixel_num, ws2812b_handle_t* ws2812b) {
    esp_err_t ret = ESP_OK;

    if(ws2812b == NULL || pixel_num == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    *ws2812b = NULL;

    ws2812b_dev_t* dev = (ws2812b_dev_t*)calloc(1, sizeof(ws2812b_dev_t));
    if(dev == NULL) {
        return ESP_ERR_NO_MEM;
    }

    dev->gpio_num = gpio_num;
    dev->pixel_num = pixel_num;

    dev->buffer_size = 3 * pixel_num;
    dev->buffer = (uint8_t*)calloc(dev->buffer_size, sizeof(uint8_t));
    if(dev->buffer == NULL) {
        free(dev);
        return ESP_ERR_NO_MEM;
    }

    ret = rmt_new_encoder(&dev->rmt_encoder);
    if(ret != ESP_OK) {
        free(dev->buffer);
        free(dev);
        return ret;
    }

    ret = ws2812b_init_channel(gpio_num, pixel_num, &dev->rmt_channel);
    if(ret != ESP_OK) {
        if(dev->rmt_encoder) {
            rmt_del_encoder(dev->rmt_encoder);
        }
        free(dev->buffer);
        free(dev);
        return ret;
    }

    ret = rmt_enable(dev->rmt_channel);
    if(ret != ESP_OK) {
        rmt_del_channel(dev->rmt_channel);
        if(dev->rmt_encoder) {
            rmt_del_encoder(dev->rmt_encoder);
        }
        free(dev->buffer);
        free(dev);
        return ret;
    }

    *ws2812b = dev;

    return ESP_OK;
}

static const rmt_transmit_config_t rmt_tx_config = {
    .loop_count = 0,
    .flags =
        {
            .eot_level = 0,
            .queue_nonblocking = true,
        },
};

esp_err_t ws2812b_show(ws2812b_handle_t ws2812b) {

    if(ws2812b == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(ws2812b->rmt_channel == NULL || ws2812b->rmt_encoder == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return rmt_transmit(ws2812b->rmt_channel, ws2812b->rmt_encoder, ws2812b->buffer, ws2812b->buffer_size, &rmt_tx_config);
}

esp_err_t ws2812b_del(ws2812b_handle_t* ws2812b) {
    if(ws2812b == NULL || *ws2812b == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ws2812b_dev_t* dev = *ws2812b;

    if(dev->rmt_channel) {
        memset(dev->buffer, 0, dev->buffer_size * sizeof(uint8_t));
        ws2812b_show(dev);
    }

    if(dev->rmt_channel) {
        rmt_tx_wait_all_done(dev->rmt_channel, RMT_TIMEOUT_MS);
        rmt_disable(dev->rmt_channel);
        rmt_del_channel(dev->rmt_channel);
    }

    if(dev->rmt_encoder) {
        rmt_del_encoder(dev->rmt_encoder);
    }

    if(dev->buffer) {
        free(dev->buffer);
    }

    free(dev);

    *ws2812b = NULL;

    return ESP_OK;
}

void ws2812b_fill(ws2812b_handle_t ws2812b, uint8_t red, uint8_t green, uint8_t blue) {
    for(int i = 0; i < ws2812b->pixel_num; i++) {
        ws2812b->buffer[3 * i + 0] = green;
        ws2812b->buffer[3 * i + 1] = red;
        ws2812b->buffer[3 * i + 2] = blue;
    }
}

esp_err_t ws2812b_wait_done(ws2812b_handle_t ws2812b) {
    return rmt_tx_wait_all_done(ws2812b->rmt_channel, RMT_TIMEOUT_MS);
}

void ws2812b_test() {
    ws2812b_handle_t ws2812b[8];
    uint8_t gpio[8] = {32, 25, 26, 27, 19, 18, 5, 17};

    for(int idx = 0; idx < 8; idx++) {
        ws2812b_init(gpio[idx], 100, &ws2812b[idx]);
    }

    uint8_t r[3] = {15, 0, 0};
    uint8_t g[3] = {0, 15, 0};
    uint8_t b[3] = {0, 0, 15};

    int test_case = 8;

    for(int i = 0; i < 100; i++) {
        for(int idx = 0; idx < test_case; idx++) {
            ws2812b_fill(ws2812b[idx], r[i % 3], g[i % 3], b[i % 3]);
        }

        uint64_t start_time = esp_timer_get_time();
        for(int idx = 0; idx < test_case; idx++) {
            ws2812b_show(ws2812b[idx]);
        }
        uint64_t end_time1 = esp_timer_get_time();

        for(int idx = 0; idx < test_case; idx++) {
            ws2812b_wait_done(ws2812b[idx]);
        }
        uint64_t end_time2 = esp_timer_get_time();

        ESP_LOGI("ws2812b_test", "show: %lld us, wait_done: %lld us", end_time1 - start_time, end_time2 - start_time);

        vTaskDelay(pdMS_TO_TICKS(200));
    }

    for(int idx = 0; idx < test_case; idx++) {
        ws2812b_del(&ws2812b[idx]);
    }
}