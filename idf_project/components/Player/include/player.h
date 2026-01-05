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
    EVENT_KILL,
} event_t;

struct Event {
    event_t type;
    int data;
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
    uint8_t** buffers = nullptr;

    // Playing State
    table_frame_t* current = nullptr;
    table_frame_t* next = nullptr;
    uint64_t playing_start_time = 0;

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
    void deinitDrivers();

    void computeTestFrame();
    table_frame_t* readFrame();
    void computeFrame();
    void currentToBuffers();
    void buffersToController();
    void showFrame();

    void allocateBuffers();
    void freeBuffers();
    void fillBuffers();

    void generateFrames();
    void freeFrames();

    void resetFrameIndex();
};
