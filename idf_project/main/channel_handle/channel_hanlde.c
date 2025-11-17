#include "channel_handle.h"

esp_err_t channel_handle_config(led_config_t* led_config, channel_handle_t* channel_handle) {
    esp_err_t ret = ESP_OK;

    if(led_config == NULL || channel_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(led_config->type != LED_TYPE_OF && led_config->type != LED_TYPE_STRIP) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(channel_handle, 0, sizeof(channel_handle_t));
    channel_handle->type = led_config->type;

    if(channel_handle->type == LED_TYPE_OF) {
        ret = pca9955b_config(led_config, &(channel_handle->pca9955b));
        if(ret != ESP_OK) {
            return ret;
        }
    }
    if(channel_handle->type == LED_TYPE_STRIP) {
        ret = ws2812b_config(led_config, &(channel_handle->ws2812b));
        if(ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}

esp_err_t channel_handle_write(color_t* colors, channel_handle_t* channel_handle) {
    esp_err_t ret = ESP_OK;

    if(colors == NULL || channel_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(channel_handle->type == LED_TYPE_OF) {
        ret = pca9955b_write_rgb(colors[0].red, colors[0].green, colors[0].blue, &channel_handle->pca9955b);
        if(ret != ESP_OK) {
            return ret;
        }
    } else if(channel_handle->type == LED_TYPE_STRIP) {
        ret = ws2812b_write((uint8_t*)colors, channel_handle->ws2812b.led_count * 3, &channel_handle->ws2812b);
        if(ret != ESP_OK) {
            return ret;
        }
    } else {
        return ESP_ERR_NOT_SUPPORTED;
    }

    return ESP_OK;
}

esp_err_t channel_handle_del(channel_handle_t* channel_handle) {
    esp_err_t ret = ESP_OK;

    if(channel_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(channel_handle->type == LED_TYPE_OF) {
        ret = pca9955b_del(&channel_handle->pca9955b);
    } else if(channel_handle->type == LED_TYPE_STRIP) {
        ret = ws2812b_del(&channel_handle->ws2812b);
    } else {
        return ESP_ERR_NOT_SUPPORTED;
    }

    memset(channel_handle, 0, sizeof(channel_handle_t));

    return ESP_OK;
}
