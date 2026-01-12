#pragma once

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "LedController_v2.hpp"
#include "framebuffer.h"
#include "player_clock.h"
#include "player_protocal.h"

class State;

class Player {
  public:
    static Player& getInstance();

    esp_err_t init();
    esp_err_t deinit();

    esp_err_t play();
    esp_err_t pause();
    esp_err_t reset();
    esp_err_t test(uint8_t, uint8_t, uint8_t);

    void startPlayback();
    void pausePlayback();
    void resetPlayback();
    void updatePlayback();
    void testPlayback(uint8_t, uint8_t, uint8_t);

    void changeState(State& newState);

  private:
    Player();
    ~Player();

    // ===== Resources =====

    State* currentState;

    bool resources_acquired;
    PlayerClock clock;
    LedController controller;
    FrameBuffer fb;

    // ===== RTOS =====

    bool taskCreated = false;
    TaskHandle_t taskHandle;
    QueueHandle_t eventQueue;

    esp_err_t createTask();
    static void taskEntry(void* pvParameters);
    void Loop();

    // ===== FSM  =====

    void handleEvent(Event& e);
    void sendEvent(Event& e);

    // ===== Lifecycle =====

    esp_err_t acquireResources();
    esp_err_t releaseResources();
};