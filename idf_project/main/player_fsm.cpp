#include "esp_log.h"
#include "player.hpp"

#define N_PCA9955B 0
#define N_STRIP_CH 8
#define N_OF_CH 0
#define N_COLOR 7

#define MAXIMUM_BRIGHTNESS 15

led_config_t ch_configs[N_STRIP_CH + N_OF_CH];

uint8_t pca9955b_addresses[N_PCA9955B] = {};
uint8_t ws2812b_gpio[N_STRIP_CH] = {16, 17, 18, 19, 21, 22, 23, 25};

uint8_t r[N_COLOR] = {255, 255, 255, 0, 0, 0, 139};
uint8_t g[N_COLOR] = {0, 127, 255, 255, 255, 0, 0};
uint8_t b[N_COLOR] = {0, 0, 0, 0, 255, 255, 255};

color_t strip_frame[N_STRIP_CH][WS2812B_MAXIMUM_LED_COUNT];
color_t of_frame[N_OF_CH][1];
color_t* cplt_frame[N_STRIP_CH + N_OF_CH];

color_t GREEN = {255, 0, 0};
color_t RED = {0, 255, 0};
color_t BLUE = {0, 0, 255};

color_t colors[N_COLOR];

void config_led_driver(LedDriver_handle_t* LedDriver) {
    for(int i = 0; i < N_STRIP_CH; i++) {
        ch_configs[i].type = LED_TYPE_STRIP;
        ch_configs[i].led_count = 100;
        ch_configs[i].rmt_gpio = ws2812b_gpio[i];
    }

    for(int i = 0; i < N_OF_CH; i++) {
        ch_configs[N_STRIP_CH + i].type = LED_TYPE_OF;
        ch_configs[N_STRIP_CH + i].i2c_addr = pca9955b_addresses[i / 5];
        ch_configs[N_STRIP_CH + i].pca_channel = i % 5;
    }

    LedDriver_config(ch_configs, N_STRIP_CH + N_OF_CH, LedDriver);

    for(int i = 0; i < N_STRIP_CH; i++) {
        cplt_frame[i] = strip_frame[i];
    }
    for(int i = 0; i < N_OF_CH; i++) {
        cplt_frame[N_STRIP_CH + i] = of_frame[i];
    }

    for(int i = 0; i < N_COLOR; i++) {
        colors[i].green = (g[i] * MAXIMUM_BRIGHTNESS) / 255;
        colors[i].red = (r[i] * MAXIMUM_BRIGHTNESS) / 255;
        colors[i].blue = (b[i] * MAXIMUM_BRIGHTNESS) / 255;
    }
}

// ----------------------------------------------------- //

void Player::onExit(player_state_t state) {
    switch(state) {
        case STATE_STOPPED:
            onExitStopped();
            break;
        case STATE_READY:
            onExitReady();
            break;
        case STATE_PLAYING:
            onExitPlaying();
            break;
        case STATE_PAUSED:
            onExitPaused();
            break;
        case STATE_PART_TEST:
            onExitPartTest();
            break;
        default:
            break;
    }
}

void Player::onEnter(player_state_t state) {
    switch(state) {
        case STATE_STOPPED:
            onEnterStopped();
            break;
        case STATE_READY:
            onEnterReady();
            break;
        case STATE_PLAYING:
            onEnterPlaying();
            break;
        case STATE_PAUSED:
            onEnterPaused();
            break;
        case STATE_PART_TEST:
            onEnterPartTest();
            break;
        default:
            break;
    }
}

void Player::onEnterStopped() {
    LedDriver_blackout(&LedDriver);
    LedDriver_del(&LedDriver);
    esp_timer_deinit(&esp_timer);
    frame = 0;
}

void Player::onEnterStoppedFromInit() {
    frame = 0;
}

void Player::onExitStopped() {}

void Player::onEnterReady() {
    config_led_driver(&LedDriver);
    esp_timer_init(&esp_timer);
}
void Player::onExitReady() {}

void Player::onEnterPlaying() {
    esp_timer_start_periodic(esp_timer, period);
}
void Player::onExitPlaying() {
    esp_timer_stop(esp_timer);
}

void Player::onEnterPaused() {}
void Player::onExitPaused() {}

void Player::onEnterPartTest() {}
void Player::onExitPartTest() {}

void Player::updateFrame() {
    ESP_LOGI("player_task", "update frame! Playing frame: %d", frame);
    LedDriver_set_color(colors[frame % N_COLOR], &LedDriver, 1);
}