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
    EVENT_DELETE,
    EVENT_COUNT,
} event_type_t;

typedef struct {
    event_type_t type;
    uint64_t master_timer;
    void* data;
} event_handle_t;

extern const char* state_name[];
extern const char* event_name[];

#ifdef __cplusplus
}
#endif