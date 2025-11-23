#include "fsm.h"
#include "esp_timer.h"

state_t transition_table[STATE_COUNT][EVENT_COUNT];
static const char* state_name[] = {
    [STATE_NULL] = "STATE_NULL",
    [STATE_STOPPED] = "STATE_STOPPED",
    [STATE_READY] = "STATE_READY",
    [STATE_PLAYING] = "STATE_PLAYING",
    [STATE_PAUSED] = "STATE_PAUSED",
    [STATE_PART_TEST] = "STATE_PART_TEST",
};
static const char* event_name[] = {
    [EVENT_STOP] = "EVENT_STOP",
    [EVENT_PAUSE] = "EVENT_PAUSE",
    [EVENT_PLAY] = "EVENT_PLAY",
    [EVENT_START] = "EVENT_START",
    [EVENT_PART_TEST] = "EVENT_PART_TEST",
    [EVENT_UPDATE_FRAME] = "EVENT_UPDATE_FRAME",
    [EVENT_COUNT] = "EVENT_COUNT",
};

esp_timer_handle_t esp_timer;

static void timer_cb(void* arg) {
    event_t event = EVENT_UPDATE_FRAME;
    xQueueSend(((fsm_handle_t*)arg)->event_queue, &event, 0);
    ESP_LOGI("timer", "queue send event_update_frame!");
};

char* fsm_getStateName(state_t state) {
    return state_name[state];
}

char* fsm_getEventName(event_t event) {
    return event_name[event];
}

void transition_table_config() {
    for(int s = 0; s < STATE_COUNT; s++) {
        for(int e = 0; e < EVENT_COUNT; e++) {
            transition_table[s][e] = STATE_NULL;
        }
    }

    transition_table[STATE_STOPPED][EVENT_START] = STATE_READY;

    transition_table[STATE_READY][EVENT_STOP] = STATE_STOPPED;
    transition_table[STATE_READY][EVENT_PLAY] = STATE_PLAYING;
    transition_table[STATE_READY][EVENT_PART_TEST] = STATE_PART_TEST;

    transition_table[STATE_PLAYING][EVENT_STOP] = STATE_STOPPED;
    transition_table[STATE_PLAYING][EVENT_UPDATE_FRAME] = STATE_PLAYING;
    transition_table[STATE_PLAYING][EVENT_PAUSE] = STATE_PAUSED;

    transition_table[STATE_PAUSED][EVENT_STOP] = STATE_STOPPED;
    transition_table[STATE_PAUSED][EVENT_PLAY] = STATE_PLAYING;

    transition_table[STATE_PART_TEST][EVENT_STOP] = STATE_STOPPED;
}

esp_err_t fsm_init(fsm_handle_t* fsm) {
    transition_table_config();
    fsm->state = STATE_STOPPED;
    fsm->event_queue = xQueueCreate(100, sizeof(event_t));

    const esp_timer_create_args_t timer_args = {
        .callback = &timer_cb,
        .arg = fsm,
    };

    esp_timer_create(&timer_args, &esp_timer);

    return ESP_OK;
}

esp_err_t fsm_transit(event_t event, fsm_handle_t* fsm) {
    esp_err_t ret = ESP_OK;
    state_t next_state = transition_table[fsm->state][event];

    if(next_state == STATE_NULL) {
        ESP_LOGI("fsm", "Invalid argument!");
        return ESP_ERR_INVALID_ARG;
    }

    if(event == EVENT_UPDATE_FRAME) {
        ESP_LOGI("fsm", "Frame update!");
        return ESP_OK;
    }

    fsm_exitState(fsm->state);
    fsm->state = next_state;
    fsm_enterState(fsm->state);

    return ESP_OK;
}

esp_err_t fsm_exitState(state_t state) {
    ESP_LOGI("fsm", "exit state: %s", state_name[state]);
    if(state == STATE_PLAYING) {
        esp_timer_stop(esp_timer);
    }
    return ESP_OK;
}

esp_err_t fsm_enterState(state_t state) {
    ESP_LOGI("fsm", "enter state: %s", state_name[state]);
    if(state == STATE_PLAYING) {
        esp_timer_start_periodic(esp_timer, 1 * 1000 * 1000 / targetFPS);
    }
    return ESP_OK;
}