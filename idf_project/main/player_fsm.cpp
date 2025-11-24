#include "esp_log.h"
#include "player.hpp"

void config_led_driver(LedDriver_handle_t* LedDriver);

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

void Player::onEnterStoppedFromInit() {}

void Player::onExitStopped() {}

void Player::onEnterReady() {
    LedDriver_init(&LedDriver);
    config_LedDriver();
    esp_timer_init(&esp_timer);

    LedDriver_blackout(&LedDriver);
}
void Player::onExitReady() {}

void Player::onEnterPlaying() {
    esp_timer_start_periodic(esp_timer, 1 * 1000 * 1000 / player_config.fps);
}
void Player::onExitPlaying() {
    esp_timer_stop(esp_timer);
}

void Player::onEnterPaused() {}
void Player::onExitPaused() {}

void Player::onEnterPartTest() {}
void Player::onExitPartTest() {}
