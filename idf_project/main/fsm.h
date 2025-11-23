#pragma once

#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define targetFPS 30

typedef enum {
    STATE_NULL,
    STATE_STOPPED,
    STATE_READY,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_PART_TEST,
    STATE_COUNT,
} state_t;

typedef enum {
    EVENT_STOP,
    EVENT_PAUSE,
    EVENT_PLAY,
    EVENT_START,
    EVENT_PART_TEST,
    EVENT_UPDATE_FRAME,
    EVENT_COUNT,
} event_t;

typedef struct {
    state_t state;
    QueueHandle_t event_queue;
} fsm_handle_t;

char* fsm_getStateName(state_t state);
char* fsm_getEventName(event_t event);

void transition_table_config();
esp_err_t fsm_init(fsm_handle_t* fsm);

esp_err_t fsm_transit(event_t event, fsm_handle_t* fsm);
esp_err_t fsm_exitState(state_t state);
esp_err_t fsm_enterState(state_t state);