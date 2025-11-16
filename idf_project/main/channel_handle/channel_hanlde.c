#include "channel_handle.h"

esp_err_t channel_handle_config(led_config_t* led_config, channel_handle_t* channel_handle) {
    channel_handle->type = led_config->type;
    if(channel_handle->type == LED_TYPE_OF) {
        pca9955b_config(led_config, &(channel_handle->pca9955b));
    }
    if(channel_handle->type == LED_TYPE_STRIP) {
        ws2812b_config(led_config, &(channel_handle->ws2812b));
    }
    return ESP_OK;
}

esp_err_t channel_handle_write(color_t* colors, channel_handle_t* channel_handle) {
    if(channel_handle->type == LED_TYPE_OF) {
        pca9955b_write_rgb(colors[0].red, colors[0].green, colors[1].blue, &channel_handle->pca9955b);
    }
    if(channel_handle->type == LED_TYPE_STRIP) {
        ws2812b_write((uint8_t*)colors, channel_handle->ws2812b.led_count * 3, &channel_handle->ws2812b);
    }
    return ESP_OK;
}

esp_err_t channel_handle_del(channel_handle_t* channel_handle) {
    if(channel_handle->type == LED_TYPE_OF) {
        pca9955b_del(&channel_handle->pca9955b);
    }
    if(channel_handle->type == LED_TYPE_STRIP) {
        ws2812b_del(&channel_handle->ws2812b);
    }
    return ESP_OK;
}

// esp_err_t channel_handle_write_RGB(const uint8_t r, const uint8_t g, const uint8_t b, channel_handle_t* channel_handle) {
//     if(channel_handle->type == LED_TYPE_OF) {
//         pca9955b_write_rgb(r, g, b, &channel_handle->pca9955b);
//     }
//     if(channel_handle->type == LED_TYPE_STRIP) {
//         ws2812b_write_rgb(r, g, b, &channel_handle->ws2812b);
//     }
//     return ESP_OK;
// }
