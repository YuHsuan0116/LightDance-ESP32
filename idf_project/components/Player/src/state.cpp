#include "state.h"
#include "esp_log.h"

// ================= ResetState =================

ResetState& ResetState::getInstance() {
    static ResetState s;
    return s;
}

void ResetState::enter(Player& player) {
#if SHOW_TRANSITION
    ESP_LOGI("state.cpp", "Enter Reset!");
#endif
    player.deinitTimer();
    player.deinitDrivers();
    // player.freeBuffers()

    player.changeState(ReadyState::getInstance());
}

void ResetState::exit(Player& player) {
    // Do nothing

#if SHOW_TRANSITION
    ESP_LOGI("state.cpp", "Exit Reset!");
#endif
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
#if SHOW_TRANSITION
    ESP_LOGI("state.cpp", "Enter Ready!");
#endif

    player.initTimer();
    player.initDrivers();
    // player.allocateBuffers();
    player.resetFrameIndex();
    vTaskDelay(pdMS_TO_TICKS(1));
}

void ReadyState::exit(Player& player) {
    // Do nothing

#if SHOW_TRANSITION
    ESP_LOGI("state.cpp", "Exit Ready!");
#endif
}

void ReadyState::handleEvent(Player& player, Event& event) {
    if(event.type == EVENT_PLAY) {
        player.changeState(PlayingState::getInstance());
    }
    if(event.type == EVENT_TEST) {
        TestState::getInstance().setTestMode((TEST_MODE_t)event.mode);
        player.controller.fill(event.red, event.green, event.blue);
        player.changeState(TestState::getInstance());
    }
    if(event.type == EVENT_RESET && event.data == 0) {
        player.changeState(ResetState::getInstance());
    }
    if(event.type == EVENT_RESET && event.data == 1) {
        player.deinitTimer();
        player.deinitDrivers();
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

#if SHOW_TRANSITION
    ESP_LOGI("state.cpp", "Enter Playing!");
#endif

    player.startTimer(10);
    player.update();
}

void PlayingState::exit(Player& player) {
    player.stopTimer();

#if SHOW_TRANSITION
    ESP_LOGI("state.cpp", "Exit Playing!");
#endif
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

    player.computeTestFrame(player.cur_frame_idx++);
    player.controller.show();
#if SHOW_TRANSITION
    ESP_LOGI("state.cpp", "Update!");
#endif
}

// ================= PauseState =================

PauseState& PauseState::getInstance() {
    static PauseState s;
    return s;
}

void PauseState::enter(Player& player) {
#if SHOW_TRANSITION
    ESP_LOGI("state.cpp", "Enter Pause!");
#endif

    // Do nothing
}
void PauseState::exit(Player& player) {
    // Do nothing
#if SHOW_TRANSITION
    ESP_LOGI("state.cpp", "Exit Pause!");
#endif
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
#if SHOW_TRANSITION
    ESP_LOGI("state.cpp", "Enter Test!");
#endif
    if(mode == TEST_MODE_SET_RGB) {
        player.update();
    }
    if(mode == TEST_MODE_BREATHING) {
        player.startTimer(1);
        player.update();
    }
}

void TestState::exit(Player& player) {
    if(mode == TEST_MODE_BREATHING) {
        player.stopTimer();
    }

#if SHOW_TRANSITION
    ESP_LOGI("state.cpp", "Exit Test!");
#endif
}

void TestState::handleEvent(Player& player, Event& event) {
    if(event.type == EVENT_RESET) {
        player.changeState(ResetState::getInstance());
    }

    if(event.type == EVENT_TEST && event.mode == TEST_MODE_SET_RGB && mode == TEST_MODE_SET_RGB) {
        // ESP_LOGI("handle_event", "r: %d, g: %d, b: %d", event.red, event.green, event.blue);
        player.controller.fill(event.red, event.green, event.blue);
        update(player);
    }
}

void TestState::update(Player& player) {
    if(mode == TEST_MODE_BREATHING) {
        player.computeTestFrame(player.cur_frame_idx++);
    }

    // player.controller.print_buffer();
    player.controller.show();

#if SHOW_TRANSITION
    ESP_LOGI("state.cpp", "Update!");
#endif
}

void TestState::setTestMode(TEST_MODE_t _mode) {
    mode = _mode;
}