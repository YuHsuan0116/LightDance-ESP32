#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

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

class Player {
  public:
    static Player& getInstance();

    Player(const Player&) = delete;
    void operator=(const Player&) = delete;

    void init();

    void changeState(State& newState);
    void handleEvent(Event& event);

    TaskHandle_t taskHandle;
    QueueHandle_t eventQueue;

  private:
    Player();
    State* currentState;

    esp_err_t createTask();
    static void taskEntry(void* pvParameters);
    void Loop();
};
