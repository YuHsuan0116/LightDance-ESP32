#include "ws2812driver.hpp"

static const char* TAG = "ws2812driver";

static rmt_transmit_config_t tx_config = {
    .loop_count = 0,
    .flags =
        {
            .eot_level = 0,
            .queue_nonblocking = false,
        },
};

ws2812Driver::ws2812Driver(): rmt_activate(false), led_count(0), channel(nullptr), encoder(nullptr) {}

esp_err_t ws2812Driver::config(const led_config_t config) {
    // basic parameter checks
    if(config.led_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    led_count = config.led_count;

    // create channel
    esp_err_t err = new_channel(config.gpio_or_addr, &channel);
    ESP_RETURN_ON_ERROR(err, TAG, "new_channel failed");

    // create encoder
    err = new_encoder(&encoder);
    if(err != ESP_OK) {
        // cleanup channel if encoder creation fails
        (void)rmt_disable(channel);
        (void)rmt_del_channel(channel);
        channel = nullptr;
        return err;
    }

    rmt_activate = true;
    return ESP_OK;
}

esp_err_t ws2812Driver::write(const color_t* colors) {
    if(!rmt_activate || channel == nullptr || encoder == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    if(!colors || led_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // WS2812 expects packed GRB bytes; color_t
    // data_size in bytes = led_count * 3
    esp_err_t err = rmt_transmit(channel, encoder, colors, led_count * 3, &tx_config);
    return err;
}

esp_err_t ws2812Driver::detach() {
    if(!rmt_activate) {
        return ESP_ERR_INVALID_STATE;  // already detached / not configured
    }

    // Try to ensure ongoing tx is done (ignore error on purpose)
    (void)rmt_tx_wait_all_done(channel, -1);

    // Encoder deletion (if your encoder implements del())
    if(encoder) {
        // If you have a custom del function: ((rmt_encoder_t*)encoder)->del(encoder);
        if(encoder->del) {
            (void)encoder->del(encoder);
        }
        encoder = nullptr;
    }

    // Disable & delete channel
    esp_err_t err1 = rmt_disable(channel);
    esp_err_t err2 = rmt_del_channel(channel);
    channel = nullptr;

    rmt_activate = false;

    // Prefer returning the first error encountered, or ESP_OK
    if(err1 != ESP_OK)
        return err1;
    if(err2 != ESP_OK)
        return err2;
    return ESP_OK;
}

esp_err_t ws2812Driver::wait_done() {
    if(!rmt_activate || channel == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    return rmt_tx_wait_all_done(channel, -1);
}

esp_err_t ws2812Driver::set_GRB(const color_t& GRB) {
    color_t data[led_count];
    for(int i = 0; i < led_count; i++) {
        data[i] = GRB;
    }
    esp_err_t ret1 = write((const color_t*)data);
    esp_err_t ret2 = wait_done();
    if(ret1 != ESP_OK) {
        return ret1;
    }
    if(ret2 != ESP_OK) {
        return ret2;
    }
    return ESP_OK;
}