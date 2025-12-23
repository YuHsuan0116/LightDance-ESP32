#include "state.h"
#include "esp_log.h"

// ================= ResetState =================
ResetState& ResetState::getInstance() {
    static ResetState s;
    return s;
}

void ResetState::enter(Player& player) {
    ESP_LOGI("state.cpp", "Enter Reset!");
    player.changeState(ReadyState::getInstance());
}
void ResetState::exit(Player& player) {
    ESP_LOGI("state.cpp", "Exit Reset!");
}
void ResetState::handleEvent(Player& player, Event& event) {}
void ResetState::update(Player& player) {}

// ================= ReadyState =================
ReadyState& ReadyState::getInstance() {
    static ReadyState s;
    return s;
}

void ReadyState::enter(Player& player) {
    ESP_LOGI("state.cpp", "Enter Ready!");
}
void ReadyState::exit(Player& player) {
    ESP_LOGI("state.cpp", "Exit Ready!");
}

void ReadyState::handleEvent(Player& player, Event& event) {
    if(event.type == EVENT_PLAY) {
        player.changeState(PlayingState::getInstance());
    }
    if(event.type == EVENT_TEST) {
        player.changeState(TestState::getInstance());
    }
}
void ReadyState::update(Player& player) {}

// ================= PlayingState =================
PlayingState& PlayingState::getInstance() {
    static PlayingState s;
    return s;
}

void PlayingState::enter(Player& player) {
    ESP_LOGI("state.cpp", "Enter Playing!");
}
void PlayingState::exit(Player& player) {
    ESP_LOGI("state.cpp", "Exit Playing!");
}

void PlayingState::handleEvent(Player& player, Event& event) {
    if(event.type == EVENT_PAUSE) {
        player.changeState(PauseState::getInstance());
    }
    if(event.type == EVENT_RESET) {
        player.changeState(ResetState::getInstance());
    }
}
void PlayingState::update(Player& player) {}

// ================= PauseState =================
PauseState& PauseState::getInstance() {
    static PauseState s;
    return s;
}

void PauseState::enter(Player& player) {
    ESP_LOGI("state.cpp", "Enter Pause!");
}
void PauseState::exit(Player& player) {
    ESP_LOGI("state.cpp", "Exit Pause!");
}

void PauseState::handleEvent(Player& player, Event& event) {
    if(event.type == EVENT_PLAY) {
        player.changeState(PlayingState::getInstance());
    }
    if(event.type == EVENT_RESET) {
        player.changeState(ResetState::getInstance());
    }
}
void PauseState::update(Player& player) {}

// ================= TestState =================
TestState& TestState::getInstance() {
    static TestState s;
    return s;
}

void TestState::enter(Player& player) {
    ESP_LOGI("state.cpp", "Enter Test!");
}
void TestState::exit(Player& player) {
    ESP_LOGI("state.cpp", "Exit Test!");
}

void TestState::handleEvent(Player& player, Event& event) {
    if(event.type == EVENT_RESET) {
        player.changeState(ResetState::getInstance());
    }
}
void TestState::update(Player& player) {}
