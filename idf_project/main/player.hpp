#pragma once

#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "LedDriver.h"
#include "fsm.h"

typedef struct {
    int fps;
    bool wait_done;

    uint8_t ws2812b_count;
    uint8_t pca9955b_count;
    uint8_t ws2812b_gpio[WS2812B_MAXIMUM_COUNT];
    uint8_t pca9955b_addresses[PCA9955B_MAXIMUM_COUNT];

} player_config_t;

class Player {
  public:
    Player();
    Player(player_config_t player_config_);
    ~Player();

    esp_err_t init();

    esp_err_t queueEvent(const event_handle_t* event);

  private:
    player_config_t player_config;

    TaskHandle_t taskHandle;
    esp_err_t create_task();
    static void taskEntry(void* pvParameters);
    void taskLoop();

    LedDriver_handle_t LedDriver;
    int frame;
    color_t strip_frame[WS2812B_MAXIMUM_COUNT][WS2812B_MAXIMUM_LED_COUNT];
    color_t of_frame[PCA9955B_MAXIMUM_COUNT][1];
    color_t* cplt_frame[WS2812B_MAXIMUM_COUNT + PCA9955B_MAXIMUM_COUNT];
    void config_LedDriver();
    void updateFrame();

    esp_timer_handle_t esp_timer;
    static void timer_callback(void* arg);
    void esp_timer_init(esp_timer_handle_t* esp_timer);
    void esp_timer_deinit(esp_timer_handle_t* esp_timer);

    event_handle_t event;
    QueueHandle_t event_queue;
    esp_err_t handleEvent(event_handle_t* event);

    player_state_t cur_state;
    player_state_t transition_table[STATE_COUNT][EVENT_COUNT];
    bool fsm_checkEventValid(player_state_t state, event_handle_t* event);

    void transition_table_config();

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
};

esp_err_t player_sendEvent(Player* player, event_handle_t* event);
