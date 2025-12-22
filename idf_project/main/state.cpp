#include "state.h"
#include "esp_log.h"

// ================= ResetState =================
ResetState& ResetState::getInstance() {
    static ResetState s;
    return s;
}

void ResetState::enter() {
    ESP_LOGI("state.cpp", "Enter Reset!");
    Player::getInstance().changeState(ReadyState::getInstance());
}
void ResetState::exit() {
    ESP_LOGI("state.cpp", "Exit Reset!");
}
void ResetState::handleEvent(Event& event) {}
void ResetState::update() {}

// ================= ReadyState =================
ReadyState& ReadyState::getInstance() {
    static ReadyState s;
    return s;
}

void ReadyState::enter() {
    ESP_LOGI("state.cpp", "Enter Ready!");
}
void ReadyState::exit() {
    ESP_LOGI("state.cpp", "Exit Ready!");
}

void ReadyState::handleEvent(Event& event) {
    if(event.type == EVENT_PLAY) {
        Player::getInstance().changeState(PlayingState::getInstance());
    }
    if(event.type == EVENT_TEST) {
        Player::getInstance().changeState(TestState::getInstance());
    }
}
void ReadyState::update() {}

// ================= PlayingState =================
PlayingState& PlayingState::getInstance() {
    static PlayingState s;
    return s;
}

void PlayingState::enter() {
    ESP_LOGI("state.cpp", "Enter Playing!");
}
void PlayingState::exit() {
    ESP_LOGI("state.cpp", "Exit Playing!");
}

void PlayingState::handleEvent(Event& event) {
    if(event.type == EVENT_PAUSE) {
        Player::getInstance().changeState(PauseState::getInstance());
    }
    if(event.type == EVENT_RESET) {
        Player::getInstance().changeState(ResetState::getInstance());
    }
}
void PlayingState::update() {}

// ================= PauseState =================
PauseState& PauseState::getInstance() {
    static PauseState s;
    return s;
}

void PauseState::enter() {
    ESP_LOGI("state.cpp", "Enter Pause!");
}
void PauseState::exit() {
    ESP_LOGI("state.cpp", "Exit Pause!");
}

void PauseState::handleEvent(Event& event) {
    if(event.type == EVENT_PLAY) {
        Player::getInstance().changeState(PlayingState::getInstance());
    }
    if(event.type == EVENT_RESET) {
        Player::getInstance().changeState(ResetState::getInstance());
    }
}
void PauseState::update() {}

// ================= TestState =================
TestState& TestState::getInstance() {
    static TestState s;
    return s;
}

void TestState::enter() {
    ESP_LOGI("state.cpp", "Enter Test!");
}
void TestState::exit() {
    ESP_LOGI("state.cpp", "Exit Test!");
}

void TestState::handleEvent(Event& event) {
    if(event.type == EVENT_RESET) {
        Player::getInstance().changeState(ResetState::getInstance());
    }
}
void TestState::update() {}
