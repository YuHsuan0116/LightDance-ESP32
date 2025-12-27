#include "state.h"
#include "esp_log.h"

// ================= ResetState =================
ResetState& ResetState::getInstance() {
    static ResetState s;
    return s;
}

void ResetState::enter(Player& player) {
    ESP_LOGI("state.cpp", "Enter Reset!");

    player.deinitTimer();
    // player.deinitDrivers();
    // player.freeBuffers()

    player.changeState(ReadyState::getInstance());
}
void ResetState::exit(Player& player) {
    // Do nothing

    ESP_LOGI("state.cpp", "Exit Reset!");
}
void ResetState::handleEvent(Player& player, Event& event) {
    // ignore
}
void ResetState::update(Player& player) {
    // ignore
}

// ================= ReadyState =================
ReadyState& ReadyState::getInstance() {
    static ReadyState s;
    return s;
}

void ReadyState::enter(Player& player) {
    ESP_LOGI("state.cpp", "Enter Ready!");

    player.initTimer();
    // player.initDrivers();
    // player.allocateBuffers();
    // player.resetFrameIndex();
}
void ReadyState::exit(Player& player) {
    // Do nothing

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
void ReadyState::update(Player& player) {
    // ignore
}

// ================= PlayingState =================
PlayingState& PlayingState::getInstance() {
    static PlayingState s;
    return s;
}

void PlayingState::enter(Player& player) {
    ESP_LOGI("state.cpp", "Enter Playing!");
    player.startTimer(10);
    player.update();
}
void PlayingState::exit(Player& player) {
    player.stopTimer();

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
void PlayingState::update(Player& player) {
    // player.computeFrame();
    // player.showFrame();
    ESP_LOGI("state.cpp", "Update!");
}

// ================= PauseState =================
PauseState& PauseState::getInstance() {
    static PauseState s;
    return s;
}

void PauseState::enter(Player& player) {
    ESP_LOGI("state.cpp", "Enter Pause!");

    // Do nothing
}
void PauseState::exit(Player& player) {
    // Do nothing

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
void PauseState::update(Player& player) {
    // ignore
}

// ================= TestState =================
TestState& TestState::getInstance() {
    static TestState s;
    return s;
}

void TestState::enter(Player& player) {
    ESP_LOGI("state.cpp", "Enter Test!");

    player.startTimer(1);
    player.update();
}
void TestState::exit(Player& player) {

    player.stopTimer();
    ESP_LOGI("state.cpp", "Exit Test!");
}

void TestState::handleEvent(Player& player, Event& event) {
    if(event.type == EVENT_RESET) {
        player.changeState(ResetState::getInstance());
    }
}
void TestState::update(Player& player) {
    // player.updateTestFrame();
    // player.showFrame();
    ESP_LOGI("state.cpp", "Update!");
}
