#pragma once

#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "LedController.hpp"

typedef enum {
    EVENT_PLAY,
    EVENT_TEST,
    EVENT_PAUSE,
    EVENT_RESET,
    EVENT_READY,
} event_t;

struct Event {
    event_t type;
};

class State;
class ErrorState;
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

    friend class ErrorState;
    friend class ResetState;
    friend class ReadyState;
    friend class PlayingState;
    friend class PauseState;
    friend class TestState;

    State* currentState;
    void update();
    void handleEvent(Event& event);
    void changeState(State& newState);

    struct isHardwareInitialized {
        bool Timer = false; 
        bool Drivers = false;
        bool Buffers = false;
    };
    
    int init_retry_count = 0;

    // ================= Resources =================

    gptimer_handle_t gptimer;

    LedController controller;
    ch_info_t ch_info;
    uint8_t** buffers;

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

    esp_err_t initDrivers();
    esp_err_t deinitDrivers();
    void computeFrame();
    void computeTestFrame();
    void showFrame();

    // ================= Buffer Management =================
    esp_err_t allocateBuffer();
    esp_err_t freeBuffers();

    // ================= Hardware Reset =================
    esp_err_t performHardwareReset();
};
