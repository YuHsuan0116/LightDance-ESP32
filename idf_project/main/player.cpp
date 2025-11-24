#include "player.hpp"
#include "esp_log.h"

void player_task(void* pvParameters) {
    Player* player = (Player*)pvParameters;
    event_handle_t event;
    QueueHandle_t queue;
    player->getEventQueue(&queue);
    while(true) {
        if(xQueueReceive(queue, &event, 100)) {
            ESP_LOGI("player_task", "received signal: %s", event_name[event.type]);
            player->handleEvent(&event);
        }
    }
    vTaskDelete(NULL);
}

esp_err_t player_sendEvent(Player* player, event_handle_t* event) {
    return player->sendEvent(event);
}

Player::Player() {
    transition_table_config();
}

Player::~Player() {}

esp_err_t Player::init() {
    frame = 0;
    event_queue = xQueueCreate(50, sizeof(event_handle_t));
    cur_state = STATE_STOPPED;

    LedDriver_init(&LedDriver);

    onEnterStoppedFromInit();
    return ESP_OK;
}

esp_err_t Player::sendEvent(event_handle_t* event) {
    xQueueSend(event_queue, event, 0);
    return ESP_OK;
}

esp_err_t Player::handleEvent(event_handle_t* event) {
    if(cur_state == STATE_PLAYING && event->type == EVENT_UPDATE_FRAME) {
        uint64_t start_time = esp_timer_get_time();
        updateFrame();
        uint64_t end_time = esp_timer_get_time();
        ESP_LOGI("player_task", "update Frame %d takes %llu us", frame, end_time - start_time);
        frame++;
        return ESP_OK;
    } else {
        if(fsm_checkEventValid(cur_state, event)) {
            ESP_LOGI("player_task", "event valid!");
            handleStateTransition(event);
            return ESP_OK;
        }
        ESP_LOGI("player_task", "event not valid!");
        return ESP_OK;
    }
}

void Player::getEventQueue(QueueHandle_t* ret_queue) {
    *ret_queue = event_queue;
}

void Player::handleStateTransition(event_handle_t* event) {
    player_state_t next_state = transition_table[cur_state][event->type];
    if(next_state == STATE_NULL) {
        ESP_LOGI("player_fsm", "Invalid state transition");
        return;
    }

    onExit(cur_state);
    ESP_LOGI("player_task", "exit %s!", state_name[cur_state]);

    cur_state = next_state;

    onEnter(cur_state);
    ESP_LOGI("player_task", "enter %s!", state_name[cur_state]);
}

void Player::esp_timer_init(esp_timer_handle_t* esp_timer) {
    const esp_timer_create_args_t timer_args = {
        .callback = &periodic_timer_callback,
        .arg = this,
    };
    esp_timer_create(&timer_args, esp_timer);
}

void Player::esp_timer_deinit(esp_timer_handle_t* esp_timer) {
    esp_timer_delete(*esp_timer);
}

static void periodic_timer_callback(void* arg) {
    Player* player = (Player*)arg;
    event_handle_t event;
    event.type = EVENT_UPDATE_FRAME;
    event.master_timer = esp_timer_get_time();
    event.data = nullptr;

    player->sendEvent(&event);
    ESP_LOGI("timer_cb", "sent update frame!");
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