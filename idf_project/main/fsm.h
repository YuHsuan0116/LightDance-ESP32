#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define targetFPS 30

typedef enum {
    STATE_NULL = 0,
    STATE_STOPPED,
    STATE_READY,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_PART_TEST,
    STATE_COUNT,
} player_state_t;

typedef enum {
    EVENT_STOP = 0,
    EVENT_PAUSE,
    EVENT_PLAY,
    EVENT_START,
    EVENT_PART_TEST,
    EVENT_UPDATE_FRAME,
    EVENT_COUNT,
} event_type_t;

typedef struct {
    event_type_t type;
    uint64_t master_timer;
    void* data;
} event_handle_t;

extern player_state_t transition_table[STATE_COUNT][EVENT_COUNT];
extern const char* state_name[];
extern const char* event_name[];

char* fsm_getStateName(player_state_t state);
char* fsm_getEventName(event_handle_t* event);

bool fsm_checkEventValid(player_state_t state, event_handle_t* event);

#ifdef __cplusplus
}
#endif