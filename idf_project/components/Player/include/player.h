#pragma once

#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "LedController.hpp"

typedef enum {
    EVENT_PLAY,
    EVENT_PAUSE,
    EVENT_TEST,
    EVENT_RESET,
} event_t;

struct Event {
    event_t type;
    uint32_t data;
};

class State;
class ResetState;
class ReadyState;
class PlayingState;
class PauseState;
class TestState;

class Player {
  public:
    static Player& getInstance();

    Player(const Player&) = delete;
    void operator=(const Player&) = delete;

    void start();

    void sendEvent(Event& event);

    TaskHandle_t& getTaskHandle();

  private:
    Player();

    // ================= Finite State Machine =================

    friend class ResetState;
    friend class ReadyState;
    friend class PlayingState;
    friend class PauseState;
    friend class TestState;

    State* currentState;
    void update();
    void handleEvent(Event& event);
    void changeState(State& newState);

    // ================= Resources =================

    gptimer_handle_t gptimer;

    LedController controller;
    ch_info_t ch_info;
    uint8_t** buffers;

    int cur_frame_idx;
    TaskHandle_t taskHandle;
    QueueHandle_t eventQueue;

    // ================= Task Managment =================

    esp_err_t createTask();
    static void taskEntry(void* pvParameters);
    void Loop();

    // ================= Timer Function Implementation =================

    void initTimer();
    void startTimer(int fps);
    void stopTimer();
    void deinitTimer();

    // ================= Driver Function Implementation =================

    void initDrivers();
    void computeTestFrame(int frame_idx);
    void computeFrame();
    void showFrame();
    void deinitDrivers();
    void allocateBuffer();
    void freeBuffers();
    void resetFrameIndex();
};
