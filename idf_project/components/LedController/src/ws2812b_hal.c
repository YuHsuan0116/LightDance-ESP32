#include "ws2812b_hal.h"

#include "string.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WS2812B_RESOLUTION 10000000

/**
 * @brief Create a new RMT TX channel for WS2812B LED driving.
 *
 * Configures the channel to generate WS2812B-compatible timing on a single GPIO.
 *
 * @param i2c_gpio_sda  GPIO pin used for SDA (must differ from SCL)
 * @param pixel_num     Number of pixels the channel must support
 * @param channel       Pointer to receive created RMT channel handle
 * @return ESP_OK on success, or RMT channel creation error
 */
static esp_err_t ws2812b_init_channel(gpio_num_t gpio_num, uint16_t pixel_num, rmt_channel_handle_t* channel) {
    rmt_tx_channel_config_t rmt_tx_channel_config = {
        .gpio_num = gpio_num,                   /*!< WS2812B signal output GPIO */
        .clk_src = RMT_CLK_SRC_DEFAULT,         /*!< RMT clock source */
        .resolution_hz = WS2812B_RESOLUTION,    /*!< Tick resolution in Hz */
        .mem_block_symbols = 512 / WS2812B_NUM, /*!< Symbol memory blocks assigned to this channel */
        .trans_queue_depth = 8,                 /*!< Hardware TX queue depth */
    };

    return rmt_new_tx_channel(&rmt_tx_channel_config, channel);
}

esp_err_t ws2812b_init(gpio_num_t gpio_num, uint16_t pixel_num, ws2812b_handle_t* ws2812b) {
    esp_err_t ret = ESP_OK;

    if(ws2812b == NULL || pixel_num == 0) {
        return ESP_ERR_INVALID_ARG; /*!< Handle must not be NULL and pixel count must be non-zero */
    }

    *ws2812b = NULL; /*!< Clear output before allocation */

    /* Allocate WS2812B device container */
    ws2812b_dev_t* dev = (ws2812b_dev_t*)calloc(1, sizeof(ws2812b_dev_t));
    if(dev == NULL) {
        return ESP_ERR_NO_MEM; /*!< Heap allocation failed */
    }

    dev->pixel_num = pixel_num; /*!< Store total pixel count */

    /* Allocate 3 bytes per pixel (GRB order recommended) */
    dev->buffer = calloc(3 * dev->pixel_num, sizeof(uint8_t));
    if(dev->buffer == NULL) {
        free(dev);
        return ESP_ERR_NO_MEM; /*!< Pixel buffer allocation failed */
    }

    /* Create composite RMT encoder instance */
    ret = rmt_new_encoder(&dev->rmt_encoder);
    if(ret != ESP_OK) {
        free(dev->buffer);
        free(dev);
        return ret; /*!< Propagate encoder creation error */
    }

    /* Initialize RMT TX channel */
    ret = ws2812b_init_channel(gpio_num, pixel_num, &dev->rmt_channel);
    if(ret != ESP_OK) {
        if(dev->rmt_encoder) {
            rmt_del_encoder(dev->rmt_encoder);
        }
        free(dev->buffer);
        free(dev);
        return ret; /*!< Propagate channel creation error */
    }

    /* Enable RMT channel */
    ret = rmt_enable(dev->rmt_channel);
    if(ret != ESP_OK) {
        rmt_del_channel(dev->rmt_channel);
        if(dev->rmt_encoder) {
            rmt_del_encoder(dev->rmt_encoder);
        }
        free(dev->buffer);
        free(dev);
        return ret; /*!< Propagate RMT enable error */
    }

    *ws2812b = dev; /*!< Return created device handle */

    return ESP_OK;
}

esp_err_t ws2812b_set_pixel(ws2812b_handle_t ws2812b, int pixel_idx, uint8_t red, uint8_t green, uint8_t blue) {
    ws2812b->buffer[3 * pixel_idx + 0] = green; /*!< G (T0) */
    ws2812b->buffer[3 * pixel_idx + 1] = red;   /*!< R (T1) */
    ws2812b->buffer[3 * pixel_idx + 2] = blue;  /*!< B (T2) */
    return ESP_OK;
}

esp_err_t ws2812b_write(ws2812b_handle_t ws2812b, uint8_t* _buffer) {
    memcpy(ws2812b->buffer, _buffer, 3 * ws2812b->pixel_num * sizeof(uint8_t));
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
        return ESP_ERR_INVALID_ARG; /*!< Device handle must not be NULL */
    }

    if(ws2812b->rmt_channel == NULL || ws2812b->rmt_encoder == NULL) {
        return ESP_ERR_INVALID_STATE; /*!< RMT channel and encoder must be initialized */
    }

    /*!< Transmit 3 bytes per pixel */
    return rmt_transmit(ws2812b->rmt_channel, ws2812b->rmt_encoder, ws2812b->buffer, 3 * ws2812b->pixel_num * sizeof(uint8_t), &rmt_tx_config);
}

esp_err_t ws2812b_del(ws2812b_handle_t* ws2812b) {
    if(ws2812b == NULL || *ws2812b == NULL) {
        return ESP_ERR_INVALID_ARG; /*!< Nothing to delete or invalid handle */
    }

    ws2812b_dev_t* dev = *ws2812b; /*!< Local pointer to device container */

    /* Clear and flush LED buffer if channel is enabled */
    if(dev->rmt_channel) {
        memset(dev->buffer, 0, 3 * dev->pixel_num * sizeof(uint8_t)); /*!< Fill GRB payload with 0 (off) */
        ws2812b_show(dev);                                            /*!< Flush cleared frame (ignore errors) */
    }

    /* Disable and delete RMT TX channel */
    if(dev->rmt_channel) {
        rmt_tx_wait_all_done(dev->rmt_channel, RMT_TIMEOUT_MS); /*!< Wait pending TX */
        rmt_disable(dev->rmt_channel);                          /*!< Disable channel */
        rmt_del_channel(dev->rmt_channel);                      /*!< Release RMT channel memory blocks */
    }

    /* Delete RMT encoder */
    if(dev->rmt_encoder) {
        rmt_del_encoder(dev->rmt_encoder); /*!< Free encoder and sub-encoders */
    }

    /* Free pixel buffer */
    if(dev->buffer) {
        free(dev->buffer); /*!< Release GRB pixel array */
    }

    /* Free device container and invalidate caller handle */
    free(dev);
    *ws2812b = NULL;

    return ESP_OK;
}

void ws2812b_fill(ws2812b_handle_t ws2812b, uint8_t red, uint8_t green, uint8_t blue) {
    for(int i = 0; i < ws2812b->pixel_num; i++) {
        ws2812b->buffer[3 * i] = green;
        ws2812b->buffer[3 * i + 1] = red;
        ws2812b->buffer[3 * i + 2] = blue;
    }
}

esp_err_t ws2812b_wait_done(ws2812b_handle_t ws2812b) {
    return rmt_tx_wait_all_done(ws2812b->rmt_channel, RMT_TIMEOUT_MS);
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

void ws2812b_print_buffer(ws2812b_handle_t ws2812b) {
    for(int i = 0; i < ws2812b->pixel_num; i++) {
        printf("%d %d %d ", ws2812b->buffer[3 * i], ws2812b->buffer[3 * i + 1], ws2812b->buffer[3 * i + 2]);
    }
    printf("\n");
}