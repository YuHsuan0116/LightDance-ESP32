#include "player.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "state.h"

#define NOTIFICATION_UPDATE 1
#define NOTIFICATION_EVENT 2

Player::Player() {}

Player& Player::getInstance() {
    static Player player;
    return player;
}

TaskHandle_t& Player::getTaskHandle() {
    return taskHandle;
}

void Player::sendEvent(Event& event) {
    xQueueSend(eventQueue, &event, 1000);
    xTaskNotify(taskHandle, NOTIFICATION_EVENT, eSetValueWithOverwrite);
}

void Player::start() {
    eventQueue = xQueueCreate(50, sizeof(Event));
    currentState = &ReadyState::getInstance();
    // ch_info = get_ch_info();

    createTask();
}

esp_err_t Player::createTask() {
    BaseType_t res = xTaskCreatePinnedToCore(Player::taskEntry, "PlayerTask", 8192, NULL, 5, &taskHandle, 0);
    return (res == pdPASS) ? ESP_OK : ESP_FAIL;
}

void Player::taskEntry(void* pvParameters) {
    Player::getInstance().Loop();

    ESP_LOGI("player.cpp", "Delete Task!");
    vTaskDelete(NULL);
}

void Player::Loop() {
    currentState->enter(*this);

    Event event;
    uint32_t ulNotifiedValue;

    while(1) {
        if(xTaskNotifyWait(0, 0, &ulNotifiedValue, portMAX_DELAY) == pdTRUE) {
            // uint64_t start = esp_timer_get_time();
            if(ulNotifiedValue == NOTIFICATION_UPDATE) {
                // ESP_LOGI("player.cpp", "Notified!");
                update();
                // uint64_t end = esp_timer_get_time();
                // ESP_LOGI("Player_Loop()", "loop takes: %llu us", end - start);
                continue;
            }
            if(ulNotifiedValue == NOTIFICATION_EVENT) {
                if(xQueueReceive(eventQueue, &event, 10)) {
                    // ESP_LOGI("player.cpp", "Received Event!");
                    handleEvent(event);
                    if(event.type == EVENT_RESET && event.data == 1) {
                        break;
                    }
                } else {
                    // ESP_LOGI("player.cpp", "xQueueReceive Timeout!");
                }
            }
            // uint64_t end = esp_timer_get_time();
            // ESP_LOGI("Player_Loop()", "loop takes: %llu us", end - start);
        }
    }

    // ESP_LOGI("player.cpp", "Exit Loop!");
}

void Player::update() {
    currentState->update(*this);
}

void Player::handleEvent(Event& event) {
    currentState->handleEvent(*this, event);
}

void Player::changeState(State& newState) {
    currentState->exit(*this);
    currentState = &newState;
    currentState->enter(*this);
}

static bool timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx) {
    Player& player = Player::getInstance();
    xTaskNotify(player.getTaskHandle(), NOTIFICATION_UPDATE, eSetValueWithOverwrite);
    return false;
}

void Player::initTimer() {
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,  // Select the default clock source
        .direction = GPTIMER_COUNT_UP,       // Counting direction is up
        .resolution_hz = 1 * 1000 * 1000,    // Resolution is 1 MHz, i.e., 1 tick equals 1 microsecond
    };

    gptimer_new_timer(&timer_config, &gptimer);

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_on_alarm_cb,  // Call the user callback function when the alarm event occurs
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    gptimer_enable(gptimer);
}

void Player::startTimer(int fps) {
    uint32_t period = 1 * 1000 * 1000 / fps;

    gptimer_alarm_config_t alarm_config;
    alarm_config.reload_count = 0;
    alarm_config.alarm_count = period;
    alarm_config.flags.auto_reload_on_alarm = true;

    gptimer_set_alarm_action(gptimer, &alarm_config);

    gptimer_start(gptimer);
}

void Player::stopTimer() {
    gptimer_stop(gptimer);
}

void Player::deinitTimer() {
    gptimer_disable(gptimer);
    gptimer_del_timer(gptimer);
}

void Player::initDrivers() {
    for(int i = 0; i < WS2812B_NUM; i++) {
        ch_info.rmt_strips[i] = 100;
    }
    for(int i = 0; i < PCA9955B_CH_NUM; i++) {
        ch_info.i2c_leds[i] = 1;
    }

    controller.init(ch_info);
}

void Player::deinitDrivers() {
    controller.deinit();
    vTaskDelay(pdMS_TO_TICKS(100));
}

void Player::resetFrameIndex() {
    cur_frame_idx = 0;
}

void Player::computeTestFrame(int frame_idx) {
    uint8_t max_brightness = 15;
    if(frame_idx % 3 == 0) {
        controller.fill(max_brightness, 0, 0);
    }
    if(frame_idx % 3 == 1) {
        controller.fill(0, max_brightness, 0);
    }
    if(frame_idx % 3 == 2) {
        controller.fill(0, 0, max_brightness);
    }
}

void Player::showFrame() {
    // controller.print_buffer();
    controller.show();
}