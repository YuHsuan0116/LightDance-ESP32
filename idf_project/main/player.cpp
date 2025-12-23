#include "player.h"
#include "esp_log.h"
#include "state.h"

Player& Player::getInstance() {
    static Player player;
    return player;
}

void Player::init() {
    eventQueue = xQueueCreate(50, sizeof(Event));
    currentState = &ResetState::getInstance();

    createTask();
}

esp_err_t Player::createTask() {
    BaseType_t res = xTaskCreate(Player::taskEntry, "PlayerTask", 8192, this, 5, &taskHandle);
    return (res == pdPASS) ? ESP_OK : ESP_FAIL;
}

void Player::taskEntry(void* pvParameters) {
    Player::getInstance().Loop();

    vTaskDelete(NULL);
}

void Player::Loop() {
    currentState->enter(*this);

    Event event;
    while(1) {
        if(xQueueReceive(eventQueue, &event, 1000)) {
            ESP_LOGI("player.cpp", "Received!");
            handleEvent(event);
            if(event.type == EVENT_KILL) {
                break;
            }
        } else {
            ESP_LOGI("player.cpp", "xQueueReceive Timeout!");
        }
    }
}

void Player::changeState(State& newState) {
    currentState->exit(*this);
    currentState = &newState;
    currentState->enter(*this);
}

void Player::handleEvent(Event& event) {
    currentState->handleEvent(*this, event);
}

Player::Player() {}
