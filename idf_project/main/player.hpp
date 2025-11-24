#pragma once

#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "LedDriver.h"
#include "fsm.h"

#define period 100000

class Player {
  public:
    Player();
    ~Player();

    esp_err_t init();
    esp_err_t sendEvent(event_handle_t* event);
    esp_err_t handleEvent(event_handle_t* event);

    void getEventQueue(QueueHandle_t* ret_queue);

  private:
    int frame;
    player_state_t cur_state;
    LedDriver_handle_t LedDriver;
    esp_timer_handle_t esp_timer;
    event_handle_t event;
    QueueHandle_t event_queue;

    void handleStateTransition(event_handle_t* event);
    void onEnter(player_state_t state);
    void onExit(player_state_t state);

    void onEnterStopped();
    void onEnterStoppedFromInit();
    void onExitStopped();

    void onEnterReady();
    void onExitReady();

    void onEnterPlaying();
    void onExitPlaying();

    void onEnterPaused();
    void onExitPaused();

    void onEnterPartTest();
    void onExitPartTest();

    void esp_timer_init(esp_timer_handle_t* esp_timer);
    void esp_timer_deinit(esp_timer_handle_t* esp_timer);

    void updateFrame();
};

esp_err_t player_sendEvent(Player* player, event_handle_t* event);

static void periodic_timer_callback(void* arg);

void transition_table_config();
void player_task(void* pvParameters);