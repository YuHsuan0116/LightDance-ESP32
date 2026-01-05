#include "player.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "state.h"
#include "BoardConfig.h"

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
            uint64_t start = esp_timer_get_time();
            if(ulNotifiedValue == NOTIFICATION_UPDATE) {
                // ESP_LOGI("player.cpp", "Notified!");
                update();
                uint64_t end = esp_timer_get_time();
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
    cur_frame_idx = -1;
    current = nullptr;
    next = nullptr;
    playing_start_time = 0;
}

static constexpr int MAX_FRAMES = 61;
static constexpr uint64_t FRAME_PERIOD = 500; // ms
static constexpr uint8_t BRIGHTNESS = 255;
static constexpr int TOTAL_CH = WS2812B_NUM + PCA9955B_CH_NUM;
static pixel_t* s_color_data[4][TOTAL_CH];     // [color][ch] -> pixel_t[n] or nullptr
static table_frame_t s_frame_pool[MAX_FRAMES];

void Player::allocateBuffers() {
    buffers = (uint8_t**)calloc(TOTAL_CH, sizeof(uint8_t*));
    if (buffers == nullptr) {
        ESP_LOGE("player.cpp", "allocateBuffer(): failed to allocate buffers pointer array (TOTAL_CH=%d)", TOTAL_CH);
        return;
    }
    for (int ch = 0; ch < TOTAL_CH; ch++) {
        const uint16_t n_pix = ch_info.pixel_counts[ch];
        if (n_pix == 0) {
            buffers[ch] = nullptr;
            continue;
        }
        const size_t bytes = (size_t)3 * (size_t)n_pix; // GRB per pixel
        buffers[ch] = (uint8_t*)calloc(bytes, 1); 
        if (buffers[ch] == nullptr) {
            ESP_LOGE("player.cpp",
                     "allocateBuffer(): failed at ch=%d (pixels=%u, bytes=%u)",
                     ch, (unsigned)n_pix, (unsigned)bytes);
            freeBuffers();
            return;
        }
    }
    ESP_LOGI("player.cpp", "allocateBuffer(): allocated buffers for %d channels", TOTAL_CH);
}

void Player::freeBuffers() {
    if (buffers == nullptr) return;
    for (int ch = 0; ch < TOTAL_CH; ch++) {
        if (buffers[ch] != nullptr) {
            free(buffers[ch]);
            buffers[ch] = nullptr;
        }
    }
    free(buffers);
    buffers = nullptr;
    ESP_LOGI("player.cpp", "freeBuffers(): buffers freed");
}

void Player::freeFrames() {
    for (int c = 0; c < 4; c++) {
        for (int ch = 0; ch < TOTAL_CH; ch++) {
            if (s_color_data[c][ch] != nullptr) {
                std::free(s_color_data[c][ch]);
                s_color_data[c][ch] = nullptr;
            }
        }
    }
    std::memset(s_frame_pool, 0, sizeof(s_frame_pool));
}

void Player::generateFrames() {
    auto is_fade_frame = [](int i0) -> bool {
        switch (i0) {
            case 4:  case 5:  case 6:   // 5,6,7
            case 9:                     // 10
            case 12: case 13: case 14:  // 13,14,15
            case 15:                    // 16
            case 22: case 23:           // 23,24
                return true;
            default:
                return false;
        }
    };
    // protection
    for (int c = 0; c < 4; c++) {
        for (int ch = 0; ch < TOTAL_CH; ch++) {
            s_color_data[c][ch] = nullptr;
        }
    }
    for (int ch = 0; ch < TOTAL_CH; ch++) {
        const uint16_t n = ch_info.pixel_counts[ch];
        if (n == 0) continue;
        for (int c = 0; c < 4; c++) {
            s_color_data[c][ch] = (pixel_t*)std::malloc((size_t)n * sizeof(pixel_t));
            if (!s_color_data[c][ch]) {
                freeFrames();
                return;
            }
            for (int p = 0; p < n; p++) {
                if (c == 0) { // Red
                    s_color_data[c][ch][p] = pixel_t{ .green = 0, .red = BRIGHTNESS, .blue = 0 };
                } else if (c == 1) { // Green
                    s_color_data[c][ch][p] = pixel_t{ .green = BRIGHTNESS, .red = 0, .blue = 0 };
                } else if (c == 2) { // Blue
                    s_color_data[c][ch][p] = pixel_t{ .green = 0, .red = 0, .blue = BRIGHTNESS };
                } else { // Black
                    s_color_data[c][ch][p] = pixel_t{ .green = 0, .red = 0, .blue = 0 };
                }
            }
        }
    }
    for (int i = 0; i < MAX_FRAMES; i++) {
        const bool last_is_black = (i == (MAX_FRAMES - 1));
        const int color = last_is_black ? 3 : (i % 3); // 3 = black
        s_frame_pool[i].timestamp = (uint64_t)i * FRAME_PERIOD; // ms
        s_frame_pool[i].fade = is_fade_frame(i); 
        s_frame_pool[i].data = (pixel_t**)s_color_data[color];
    }
}


table_frame_t* Player::readFrame() {
    cur_frame_idx++;
    if (cur_frame_idx < 0) cur_frame_idx = 0;
    if (cur_frame_idx >= MAX_FRAMES) return nullptr;
    return &s_frame_pool[cur_frame_idx];
}

static inline float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static inline uint8_t f_to_u8(float x01) {
    x01 = clamp01(x01);
    float y = x01 * 255.0f + 0.5f;
    if (y < 0.0f) y = 0.0f;
    if (y > 255.0f) y = 255.0f;
    return (uint8_t)y;
}

static inline void rgb_to_hsv(float r, float g, float b, float& h_deg, float& s, float& v) {
    float cmax = std::max(r, std::max(g, b));
    float cmin = std::min(r, std::min(g, b));
    float delta = cmax - cmin;
    v = cmax;
    if (cmax <= 0.0f) { s = 0.0f; h_deg = 0.0f; return; }
    s = (delta <= 0.0f) ? 0.0f : (delta / cmax);
    if (delta <= 0.0f) { h_deg = 0.0f; return; }
    if (cmax == r) {
        h_deg = 60.0f * fmodf(((g - b) / delta), 6.0f);
    } else if (cmax == g) {
        h_deg = 60.0f * (((b - r) / delta) + 2.0f);
    } else {
        h_deg = 60.0f * (((r - g) / delta) + 4.0f);
    }
    if (h_deg < 0.0f) h_deg += 360.0f;
    if (h_deg >= 360.0f) h_deg = fmodf(h_deg, 360.0f);
}

static inline void hsv_to_rgb(float h_deg, float s, float v, float& r, float& g, float& b) {
    h_deg = fmodf(h_deg, 360.0f);
    if (h_deg < 0.0f) h_deg += 360.0f;
    s = clamp01(s);
    v = clamp01(v);
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h_deg / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float rp = 0, gp = 0, bp = 0;
    if (h_deg < 60.0f)       { rp = c; gp = x; bp = 0; }
    else if (h_deg < 120.0f) { rp = x; gp = c; bp = 0; }
    else if (h_deg < 180.0f) { rp = 0; gp = c; bp = x; }
    else if (h_deg < 240.0f) { rp = 0; gp = x; bp = c; }
    else if (h_deg < 300.0f) { rp = x; gp = 0; bp = c; }
    else                     { rp = c; gp = 0; bp = x; }
    r = rp + m;
    g = gp + m;
    b = bp + m;
}

static inline float lerp_hue_short(float h1, float h2, float p) {
    h1 = fmodf(h1, 360.0f); if (h1 < 0.0f) h1 += 360.0f;
    h2 = fmodf(h2, 360.0f); if (h2 < 0.0f) h2 += 360.0f;
    float d = h2 - h1;
    if (d > 180.0f)  d -= 360.0f;
    if (d < -180.0f) d += 360.0f;
    float h = h1 + p * d;
    h = fmodf(h, 360.0f);
    if (h < 0.0f) h += 360.0f;
    return h;
}

void Player::currentToBuffers() {
    if (!buffers || !current) return;
    for (int ch = 0; ch < TOTAL_CH; ch++) {
        const int n = ch_info.pixel_counts[ch];
        uint8_t* out = buffers[ch];
        pixel_t* src = (current->data) ? current->data[ch] : nullptr;
        if (!out || !src || n <= 0) continue;
        for (int pix = 0; pix < n; pix++) {
            out[3 * pix + 0] = src[pix].green;
            out[3 * pix + 1] = src[pix].red;
            out[3 * pix + 2] = src[pix].blue;
        }
    }
}

void Player::buffersToController() {
    if (!buffers) return;
    for (int ch = 0; ch < TOTAL_CH; ch++) {
        if (!buffers[ch]) continue;
        controller.ch_write_buffer(ch, buffers[ch]);
    }
}

static inline void send_reset(Player& player) {
    Event e;
    e.type = EVENT_RESET;
    e.data = 0;
    player.sendEvent(e);
}

void Player::showFrame() {
    controller.show();
}

void Player::computeFrame() {
    uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);
    uint64_t t_ms = (now_ms >= playing_start_time)
                      ? (now_ms - playing_start_time)
                      : 0;
    if (current == nullptr) {
        send_reset(*this);
        return;
    }
    while (next != nullptr && t_ms >= next->timestamp) { // can skip several frames
        current = next;
        next = readFrame();
    }
    // end condition
    if (next == nullptr) { 
        currentToBuffers();
        buffersToController();
        showFrame();
        send_reset(*this);
        return;
    }
    // fade == 0
    if (!current->fade) {
        currentToBuffers();
        buffersToController();
        return;
    }
    // fade == 1
    const uint64_t t1 = current->timestamp;
    const uint64_t t2 = next->timestamp;
    float p = 0.0f;
    if (t2 > t1) {
        double num = (t_ms > t1) ? (double)(t_ms - t1) : 0.0;
        double den = (double)(t2 - t1);
        p = (float)(num / den);
        p = clamp01(p);
    } else {
        p = 1.0f;
    }
    for (int ch = 0; ch < TOTAL_CH; ch++) {
        const int n = ch_info.pixel_counts[ch];
        uint8_t* out = buffers[ch];
        pixel_t* a = (current->data) ? current->data[ch] : nullptr;
        pixel_t* b = (next->data) ? next ->data[ch] : nullptr;
        if (!out || !a || !b || n <= 0) continue;
        for (int pix = 0; pix < n; pix++) {
            const float r1 = a[pix].red   / 255.0f;
            const float g1 = a[pix].green / 255.0f;
            const float b1 = a[pix].blue  / 255.0f;

            const float r2 = b[pix].red   / 255.0f;
            const float g2 = b[pix].green / 255.0f;
            const float b2 = b[pix].blue  / 255.0f;
            float h1, s1, v1;
            float h2, s2, v2;
            rgb_to_hsv(r1, g1, b1, h1, s1, v1);
            rgb_to_hsv(r2, g2, b2, h2, s2, v2);
            const float h = lerp_hue_short(h1, h2, p);
            const float s = (1.0f - p) * s1 + p * s2;
            const float v = (1.0f - p) * v1 + p * v2;
            float rr, gg, bb;
            hsv_to_rgb(h, s, v, rr, gg, bb);
            out[3 * pix + 0] = f_to_u8(gg);
            out[3 * pix + 1] = f_to_u8(rr);
            out[3 * pix + 2] = f_to_u8(bb);
        }
    }
    buffersToController();
}

void Player::fillBuffers() {
    current = readFrame();
    next = readFrame();
    // end condition
    if (next == nullptr) { 
        currentToBuffers();
        buffersToController();
        showFrame();
        send_reset(*this);
        return;
    }
    currentToBuffers();
    buffersToController();
}

void Player::computeTestFrame() {
    uint8_t max_brightness = 15;
    if(cur_frame_idx % 3 == 0) {
        controller.fill(max_brightness, 0, 0);
    }
    if(cur_frame_idx % 3 == 1) {
        controller.fill(0, max_brightness, 0);
    }
    if(cur_frame_idx % 3 == 2) {
        controller.fill(0, 0, max_brightness);
    }
}
