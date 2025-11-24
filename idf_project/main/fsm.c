#include "fsm.h"
#include "esp_timer.h"

const char* state_name[] = {
    [STATE_NULL] = "STATE_NULL",
    [STATE_STOPPED] = "STATE_STOPPED",
    [STATE_READY] = "STATE_READY",
    [STATE_PLAYING] = "STATE_PLAYING",
    [STATE_PAUSED] = "STATE_PAUSED",
    [STATE_PART_TEST] = "STATE_PART_TEST",
};

const char* event_name[] = {
    [EVENT_STOP] = "EVENT_STOP",
    [EVENT_PAUSE] = "EVENT_PAUSE",
    [EVENT_PLAY] = "EVENT_PLAY",
    [EVENT_START] = "EVENT_START",
    [EVENT_PART_TEST] = "EVENT_PART_TEST",
    [EVENT_UPDATE_FRAME] = "EVENT_UPDATE_FRAME",
    [EVENT_COUNT] = "EVENT_COUNT",
};
