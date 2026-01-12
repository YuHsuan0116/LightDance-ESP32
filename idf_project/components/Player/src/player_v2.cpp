#include "player_v2.hpp"
#include "player_state.hpp"

#include "esp_check.h"
#include "esp_log.h"

static const char* TAG = "Player";

Player::Player() {}
Player::~Player() {}

Player& Player::getInstance() {
    static Player player;
    return player;
}

esp_err_t Player::init() {
    if(taskCreated) {
        return ESP_ERR_INVALID_STATE;
    }

    currentState = &ReadyState::getInstance();

    createTask();

    return ESP_OK;
}

esp_err_t Player::createTask() {
    BaseType_t res = xTaskCreatePinnedToCore(Player::taskEntry, "PlayerTask", 8192, NULL, 5, &taskHandle, 0);
    return (res == pdPASS) ? ESP_OK : ESP_FAIL;
}

void Player::taskEntry(void* pvParameters) {
    if(Player::getInstance().acquireResources() != ESP_OK) {
        vTaskDelete(NULL);
    }

    Player::getInstance().taskCreated = true;
    Player::getInstance().Loop();
}

void Player::Loop() {

    currentState->enter(*this);

    Event e;
    uint32_t ulNotifiedValue;

    bool running = true;

    while(running) {
        xTaskNotifyWait(0, UINT32_MAX, &ulNotifiedValue, portMAX_DELAY);

        if(ulNotifiedValue & NOTIFICATION_EVENT) {
            while(xQueueReceive(eventQueue, &e, 0) == pdTRUE) {
                if(e.type == EVENT_EXIT) {
                    running = false;
                    break;
                }
                currentState->handleEvent(*this, e);
            }
        }

        if(running && (ulNotifiedValue & NOTIFICATION_UPDATE)) {
            currentState->update(*this);
        }
    }

    releaseResources();
    ESP_LOGI("main", "delete task!");
    Player::getInstance().taskCreated = false;
    vTaskDelete(NULL);
}

void Player::sendEvent(Event& event) {
    if(taskCreated) {
        xQueueSend(eventQueue, &event, 1000);
        xTaskNotify(taskHandle, NOTIFICATION_EVENT, eSetBits);
    }
}

void Player::handleEvent(Event& event) {
    currentState->handleEvent(*this, event);
}

void Player::changeState(State& newState) {
    currentState->exit(*this);
    currentState = &newState;
    currentState->enter(*this);
}

esp_err_t Player::acquireResources() {
    if(resources_acquired) {
        return ESP_OK;
    }

    for(int i = 0; i < WS2812B_NUM; i++) {
        ch_info.rmt_strips[i] = WS2812B_MAX_PIXEL_NUM;
    }
    for(int i = 0; i < PCA9955B_CH_NUM; i++) {
        ch_info.i2c_leds[i] = 1;
    }

    ESP_RETURN_ON_ERROR(controller.init(), TAG, "controller init failed");
    ESP_LOGI("player", "controller ok");

    ESP_RETURN_ON_ERROR(fb.init(), TAG, "fb init failed");
    ESP_LOGI("player", "fb ok");

    ESP_RETURN_ON_ERROR(clock.init(true, taskHandle, 1000000 / 40), TAG, "clock init failed");
    ESP_LOGI("player", "clock ok");

    eventQueue = xQueueCreate(50, sizeof(Event));

    resources_acquired = true;

    return ESP_OK;
}

esp_err_t Player::releaseResources() {
    if(!resources_acquired) {
        return ESP_ERR_INVALID_STATE;
    }

    vQueueDelete(eventQueue);
    eventQueue = NULL;

    controller.deinit();
    fb.deinit();
    clock.deinit();

    resources_acquired = false;

    return ESP_OK;
}

static esp_err_t render(FrameBuffer& fb, LedController& controller) {
    frame_data* buffer = fb.get_buffer();

    for(int i = 0; i < WS2812B_NUM; i++) {
        controller.write_buffer(i, (uint8_t*)buffer->ws2812b[i]);
    }

    for(int i = 0; i < PCA9955B_CH_NUM; i++) {
        controller.write_buffer(i + WS2812B_NUM, (uint8_t*)&buffer->pca9955b[i]);
    }

    return ESP_OK;
}

void Player::startPlayback() {
    clock.start();
}

void Player::pausePlayback() {
    clock.pause();
}

void Player::resetPlayback() {
    clock.pause();
    clock.reset();
    fb.reset();

    controller.fill(0, 0, 0);
    controller.show();
}

void Player::updatePlayback() {
    fb.compute(clock.now_us() / 1000);
    render(fb, controller);
    controller.show();
}

void Player::testPlayback(uint8_t r, uint8_t g, uint8_t b) {
    controller.fill(r, g, b);
    controller.show();
}

static Event e;

esp_err_t Player::deinit() {
    e.type = EVENT_EXIT;
    sendEvent(e);

    return ESP_OK;
}

esp_err_t Player::play() {
    e.type = EVENT_PLAY;
    sendEvent(e);

    return ESP_OK;
}

esp_err_t Player::pause() {
    e.type = EVENT_PAUSE;
    sendEvent(e);

    return ESP_OK;
}

esp_err_t Player::reset() {
    e.type = EVENT_RESET;
    sendEvent(e);

    return ESP_OK;
}

esp_err_t Player::test(uint8_t r, uint8_t g, uint8_t b) {
    e.type = EVENT_TEST;
    e.test_data.r = r;
    e.test_data.g = g;
    e.test_data.b = b;

    sendEvent(e);

    return ESP_OK;
}
