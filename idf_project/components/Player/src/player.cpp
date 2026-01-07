#include "player.h"
#include <math.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "state.h"

#define NOTIFICATION_UPDATE (1 << 0)
#define NOTIFICATION_EVENT (1 << 1)

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
    xTaskNotify(taskHandle, NOTIFICATION_EVENT, eSetBits);
}

void Player::start() {
    eventQueue = xQueueCreate(50, sizeof(Event));
    currentState = &ResetState::getInstance();
    // ch_info = get_ch_info();

    createTask();
}

// ================= Task Managment =================

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
        if(xTaskNotifyWait(0, 0xFFFFFFFF, &ulNotifiedValue, portMAX_DELAY) == pdTRUE) {
            // uint64_t start = esp_timer_get_time();
            if((ulNotifiedValue & NOTIFICATION_UPDATE) != 0) {
                // ESP_LOGI("player.cpp", "Notified!");
                update();
                // uint64_t end = esp_timer_get_time();
                // ESP_LOGI("Player_Loop()", "loop takes: %llu us", end - start);
            }
            if((ulNotifiedValue & NOTIFICATION_EVENT) != 0) {
                if(xQueueReceive(eventQueue, &event, 10)) {
                    // ESP_LOGI("player.cpp", "Received Event!");
                    handleEvent(event);
                    if(event.type == EVENT_RESET) {
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
    xTaskNotify(player.getTaskHandle(), NOTIFICATION_UPDATE, eSetBits);
    return false;
}

esp_err_t Player::initTimer() {
    gptimer_config_t timer_config = {.clk_src = GPTIMER_CLK_SRC_DEFAULT,  // Select the default clock source
                                     .direction = GPTIMER_COUNT_UP,       // Counting direction is up
                                     .resolution_hz = 1 * 1000 * 1000,    // Resolution is 1 MHz, i.e., 1 tick equals 1 microsecond

                                     .intr_priority = 0,
                                     .flags = {.intr_shared = 0, .allow_pd = 0, .backup_before_sleep = 0}};

    gptimer_new_timer(&timer_config, &gptimer);

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_on_alarm_cb,  // Call the user callback function when the alarm event occurs
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    gptimer_enable(gptimer);

    return ESP_OK;
}
esp_err_t Player::clearTimer() {
    gptimer_set_raw_count(gptimer, 0);
    return ESP_OK;
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

esp_err_t Player::deinitTimer() {
    gptimer_disable(gptimer);
    gptimer_del_timer(gptimer);
    return ESP_OK;
}

// ================= Driver Function Implementation =================

esp_err_t Player::initDrivers() {
    for(int i = 0; i < WS2812B_NUM; i++) {
        ch_info.rmt_strips[i] = 100;
    }
    for(int i = 0; i < PCA9955B_CH_NUM; i++) {
        ch_info.i2c_leds[i] = 1;
    }

    controller.init(ch_info);
    return ESP_OK;
}

esp_err_t Player::clearDrivers() {
    controller.black_out();
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

esp_err_t Player::deinitDrivers() {
    controller.deinit();
    vTaskDelay(pdMS_TO_TICKS(100));
}

static int frame_idx = 0;

void Player::computeTestFrame() {

    uint8_t max_brightness = 63;
    float r = 0.0f, g = 0.0f, b = 0.0f;

    float H = (frame_idx * 360 / 360) % 360;
    float S = 1.0f;
    float V = 0.8f + 0.2f * sinf(frame_idx * 0.05f);

    float h = H / 360.0f;
    float s = S;
    float v = V;

    int i = floor(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch(i % 6) {
        case 0:
            r = v, g = t, b = p;
            break;
        case 1:
            r = q, g = v, b = p;
            break;
        case 2:
            r = p, g = v, b = t;
            break;
        case 3:
            r = p, g = q, b = v;
            break;
        case 4:
            r = t, g = p, b = v;
            break;
        case 5:
            r = v, g = p, b = q;
            break;
        default:
            break;
    }

    uint8_t R = (uint8_t)(r * max_brightness);
    uint8_t G = (uint8_t)(g * max_brightness);
    uint8_t B = (uint8_t)(b * max_brightness);

    // ESP_LOGI("test", "R: %d, G: %d, B: %d", R, G, B);

    controller.fill(R, G, B);
}

void Player::showFrame() {
    // controller.print_buffer();
    controller.show();
}

// ================= Buffer Management =================

// ================= Hardware Reset / Clear =================

esp_err_t Player::performHardwareReset() {
    if(isHardwareInitialized.Timer) {
        if(deinitTimer() != ESP_OK)
            ESP_LOGW("Reset", "Deinit Timer Failed");
        isHardwareInitialized.Timer = false;
    }
    if(isHardwareInitialized.Drivers) {
        if(deinitDrivers() != ESP_OK)
            ESP_LOGW("Reset", "Deinit Drivers Failed");
        isHardwareInitialized.Drivers = false;
    }
    if(isHardwareInitialized.Buffers) {
        if(freeBuffers() != ESP_OK)
            ESP_LOGW("Reset", "Free Buffers Failed");
        isHardwareInitialized.Buffers = false;
    }

    if(initTimer() != ESP_OK) {
        ESP_LOGW("Reset", "Init Timer Failed!");
        return ESP_FAIL;
    }
    isHardwareInitialized.Timer = true;

    if(initDrivers() != ESP_OK) {
        ESP_LOGW("Reset", "Init Drivers Failed!");
        return ESP_FAIL;
    }
    isHardwareInitialized.Drivers = true;

    if(allocateBuffer() != ESP_OK) {
        ESP_LOGW("Reset", "Alloc Buffer Failed!");
        return ESP_FAIL;
    }
    isHardwareInitialized.Buffers = true;

    return ESP_OK;
}

esp_err_t Player::performHardwareClear() {
    if(clearTimer() != ESP_OK) {
        ESP_LOGW("Reset", "Clear Timer Failed!");
        return ESP_FAIL;
    }

    if(clearDrivers() != ESP_OK) {
        ESP_LOGW("Reset", "Clear Drivers Failed!");
        return ESP_FAIL;
    }

    if(clearBuffers() != ESP_OK) {
        ESP_LOGW("Reset", "Clear Buffer Failed!");
        return ESP_FAIL;
    }

    return ESP_OK;
}