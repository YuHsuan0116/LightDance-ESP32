#include "player.hpp"
#include "esp_log.h"

#define N_COLOR 100
#define MAXIMUM_BRIGHTNESS 15

uint8_t r[N_COLOR] = {255, 255, 255, 0, 0, 0, 139};
uint8_t g[N_COLOR] = {0, 127, 255, 255, 255, 0, 0};
uint8_t b[N_COLOR] = {0, 0, 0, 0, 255, 255, 255};

color_t GREEN = {255, 0, 0};
color_t RED = {0, 255, 0};
color_t BLUE = {0, 0, 255};

color_t rainbow[N_COLOR] = {{255, 0, 0},   {255, 13, 0},  {255, 27, 0},  {255, 40, 0},  {255, 53, 0},  {255, 67, 0},  {255, 80, 0},  {255, 93, 0},
                            {255, 107, 0}, {255, 120, 0}, {255, 133, 0}, {255, 147, 0}, {255, 160, 0}, {255, 173, 0}, {255, 187, 0}, {255, 200, 0},
                            {255, 213, 0}, {255, 227, 0}, {255, 240, 0}, {247, 255, 0}, {233, 255, 0}, {220, 255, 0}, {207, 255, 0}, {193, 255, 0},
                            {180, 255, 0}, {167, 255, 0}, {153, 255, 0}, {140, 255, 0}, {127, 255, 0}, {113, 255, 0}, {100, 255, 0}, {87, 255, 0},
                            {73, 255, 0},  {60, 255, 0},  {47, 255, 0},  {33, 255, 0},  {20, 255, 0},  {7, 255, 0},   {0, 255, 7},   {0, 255, 20},
                            {0, 255, 33},  {0, 255, 47},  {0, 255, 60},  {0, 255, 73},  {0, 255, 87},  {0, 255, 100}, {0, 255, 113}, {0, 255, 127},
                            {0, 255, 140}, {0, 255, 153}, {0, 255, 167}, {0, 255, 180}, {0, 255, 193}, {0, 255, 207}, {0, 255, 220}, {0, 255, 233},
                            {0, 255, 247}, {0, 247, 255}, {0, 233, 255}, {0, 220, 255}, {0, 207, 255}, {0, 193, 255}, {0, 180, 255}, {0, 167, 255},
                            {0, 153, 255}, {0, 140, 255}, {0, 127, 255}, {0, 113, 255}, {0, 100, 255}, {0, 87, 255},  {0, 73, 255},  {0, 60, 255},
                            {0, 47, 255},  {0, 33, 255},  {0, 20, 255},  {0, 7, 255},   {7, 0, 255},   {20, 0, 255},  {33, 0, 255},  {47, 0, 255},
                            {60, 0, 255},  {73, 0, 255},  {87, 0, 255},  {100, 0, 255}, {113, 0, 255}, {127, 0, 255}, {140, 0, 255}, {153, 0, 255},
                            {167, 0, 255}, {180, 0, 255}, {193, 0, 255}, {207, 0, 255}, {220, 0, 255}, {233, 0, 255}, {247, 0, 255}, {255, 0, 247},
                            {255, 0, 233}, {255, 0, 220}, {255, 0, 207}, {255, 0, 193}};

esp_err_t player_sendEvent(Player* player, event_handle_t* event) {
    return player->queueEvent(event);
}

Player::Player() {}

Player::Player(player_config_t player_config_) {
    transition_table_config();
    player_config = player_config_;
}

Player::~Player() {}

esp_err_t Player::init() {
    frame = 0;

    event_queue = xQueueCreate(50, sizeof(event_handle_t));
    cur_state = STATE_STOPPED;

    create_task();
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

void Player::handleStateTransition(event_handle_t* event) {
    player_state_t next_state = transition_table[cur_state][event->type];
    if(next_state == STATE_NULL) {
        ESP_LOGI("player_fsm", "Invalid state transition");
        return;
    }

    ESP_LOGI("player_task", "exit %s!", state_name[cur_state]);
    onExit(cur_state);

    cur_state = next_state;

    ESP_LOGI("player_task", "enter %s!", state_name[cur_state]);
    onEnter(cur_state);
}

void Player::esp_timer_init(esp_timer_handle_t* esp_timer) {
    const esp_timer_create_args_t timer_args = {
        .callback = &Player::timer_callback,
        .arg = this,
    };
    esp_timer_create(&timer_args, esp_timer);
}

void Player::esp_timer_deinit(esp_timer_handle_t* esp_timer) {
    esp_timer_delete(*esp_timer);
}

void Player::timer_callback(void* arg) {
    Player* player = (Player*)arg;
    event_handle_t event;
    event.type = EVENT_UPDATE_FRAME;
    event.master_timer = esp_timer_get_time();
    event.data = nullptr;

    player->queueEvent(&event);
    ESP_LOGI("timer_cb", "sent update frame!");
}

void Player::transition_table_config() {
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

led_config_t ch_configs[WS2812B_MAXIMUM_COUNT + 5 * PCA9955B_MAXIMUM_COUNT];

void Player::config_LedDriver() {
    for(int i = 0; i < player_config.ws2812b_count; i++) {
        ch_configs[i].type = LED_TYPE_STRIP;
        ch_configs[i].led_count = 100;
        ch_configs[i].rmt_gpio = player_config.ws2812b_gpio[i];
    }

    for(int i = 0; i < player_config.pca9955b_ch_count; i++) {
        ch_configs[player_config.ws2812b_count + i].type = LED_TYPE_OF;
        ch_configs[player_config.ws2812b_count + i].i2c_addr = player_config.pca9955b_addresses[i / 5];
        ch_configs[player_config.ws2812b_count + i].pca_channel = i % 5;
    }

    ESP_ERROR_CHECK(LedDriver_config(ch_configs, player_config.ws2812b_count + player_config.pca9955b_ch_count, &LedDriver));

    for(int i = 0; i < player_config.ws2812b_count; i++) {
        cplt_frame[i] = strip_frame[i];
    }
    for(int i = 0; i < player_config.pca9955b_ch_count; i++) {
        cplt_frame[player_config.ws2812b_count + i] = of_frame[i];
    }

    // for(int i = 0; i < N_COLOR; i++) {
    //     colors[i].green = g[i] * MAXIMUM_BRIGHTNESS / 256;
    //     colors[i].red = r[i] * MAXIMUM_BRIGHTNESS / 256;
    //     colors[i].blue = b[i] * MAXIMUM_BRIGHTNESS / 256;
    // }
}

void Player::updateFrame() {
    esp_err_t ret = ESP_OK;
    ESP_LOGI("player_task", "update frame! Playing frame: %d", frame);
    ret = LedDriver_set_rgb(rainbow[frame % N_COLOR].red * player_config.maximum_brightness / 255,
                            rainbow[frame % N_COLOR].green * player_config.maximum_brightness / 255,
                            rainbow[frame % N_COLOR].blue * player_config.maximum_brightness / 255,
                            &LedDriver,
                            player_config.wait_done);
    if(ret != ESP_OK) {
        ESP_LOGE("player_task", "LedDriver_set_color failed!");
    }
}

bool Player::fsm_checkEventValid(player_state_t state, event_handle_t* event) {
    return transition_table[state][event->type] != STATE_NULL;
}

esp_err_t Player::create_task() {
    BaseType_t res = xTaskCreate(Player::taskEntry, "PlayerTask", 8192, this, 5, &taskHandle);
    return (res == pdPASS) ? ESP_OK : ESP_FAIL;
}

void Player::taskEntry(void* pvParameters) {
    Player* player = (Player*)pvParameters;
    player->taskLoop();

    ESP_LOGI("player_task", "task delete!");
    vTaskDelete(NULL);
}

void Player::taskLoop() {
    while(1) {
        if(xQueueReceive(event_queue, &event, 100)) {
            if(event.type == EVENT_DELETE) {
                break;
            }
            handleEvent(&event);
        }
    }
    LedDriver_del(&LedDriver);
    esp_timer_deinit(&esp_timer);
}

esp_err_t Player::queueEvent(const event_handle_t* event) {
    xQueueSend(event_queue, event, 0);
    return ESP_OK;
}