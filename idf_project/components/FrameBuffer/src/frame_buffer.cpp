#include "frame_buffer.h"
#include "esp_log.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm> // for std::max, std::min

// =========================================================
// 1. 引用硬體設定 
// =========================================================

#include "BoardConfig.h" 

// 定義 TAG 供 Log 使用
static const char* TAG = "FrameBuffer";

// 假設 BoardConfig.h 裡有定義 WS2812B_NUM 和 PCA9955B_CH_NUM
static constexpr int TOTAL_CH = WS2812B_NUM + PCA9955B_CH_NUM;

// 其他與硬體無關的常數
static constexpr int MAX_FRAMES = 61;
static constexpr uint64_t FRAME_PERIOD = 500; // ms
static constexpr uint8_t BRIGHTNESS = 255;

// =========================================================
// 2. 靜態變數 (內部資料池)
// =========================================================
static pixel_t* s_color_data[4][TOTAL_CH];     // [color][ch] -> pixel_t[n] or nullptr
static table_frame_t s_frame_pool[MAX_FRAMES]; // 預先配置好的影格池

// =========================================================
// 3. 數學運算工具 (Helper Functions)
// =========================================================

// 限制範圍 0.0 ~ 1.0
static inline float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

// 浮點數轉 0-255 整數
static inline uint8_t f_to_u8(float x01) {
    x01 = clamp01(x01);
    float y = x01 * 255.0f + 0.5f;
    if (y < 0.0f) y = 0.0f;
    if (y > 255.0f) y = 255.0f;
    return (uint8_t)y;
}

// RGB 轉 HSV
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

// HSV 轉 RGB
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

// 色相(Hue)最短路徑插值
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

// =========================================================
// 4. FrameBuffer 類別實作
// =========================================================

FrameBuffer::FrameBuffer() {
    reset();
}

FrameBuffer::~FrameBuffer() {
    freeBuffers();
    freeFrames(); // 也要記得釋放測試資料
}

void FrameBuffer::reset() {
    cur_frame_idx = -1;
    current = nullptr;
    next = nullptr;
}

void FrameBuffer::allocateBuffers(const uint16_t* pixel_counts, int num_channels) {
    // 保存外部傳入的像素計數陣列 (Player::ch_info.pixel_counts)
    this->ch_pixel_counts = pixel_counts;
    this->total_channels = num_channels;

    // 防呆檢查：如果 Config 和傳入參數不一致，印出警告
    if (num_channels != TOTAL_CH) {
        ESP_LOGW(TAG, "Channel count mismatch! Config: %d, Arg: %d", TOTAL_CH, num_channels);
    }

    buffers = (uint8_t**)calloc(TOTAL_CH, sizeof(uint8_t*));
    if (buffers == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate buffers pointer array");
        return;
    }

    for (int ch = 0; ch < TOTAL_CH; ch++) {
        const uint16_t n_pix = ch_pixel_counts[ch];
        if (n_pix == 0) {
            buffers[ch] = nullptr;
            continue;
        }
        const size_t bytes = (size_t)3 * (size_t)n_pix; // GRB
        buffers[ch] = (uint8_t*)calloc(bytes, 1);
        if (buffers[ch] == nullptr) {
            ESP_LOGE(TAG, "Failed to allocate buffer for ch=%d", ch);
            freeBuffers();
            return;
        }
    }
    ESP_LOGI(TAG, "Allocated buffers for %d channels", TOTAL_CH);
}

void FrameBuffer::freeBuffers() {
    if (buffers == nullptr) return;
    for (int ch = 0; ch < TOTAL_CH; ch++) {
        if (buffers[ch] != nullptr) {
            free(buffers[ch]);
            buffers[ch] = nullptr;
        }
    }
    free(buffers);
    buffers = nullptr;
    ESP_LOGI(TAG, "Buffers freed");
}

void FrameBuffer::freeFrames() {
    for (int c = 0; c < 4; c++) {
        for (int ch = 0; ch < TOTAL_CH; ch++) {
            if (s_color_data[c][ch] != nullptr) {
                free(s_color_data[c][ch]);
                s_color_data[c][ch] = nullptr;
            }
        }
    }
    memset(s_frame_pool, 0, sizeof(s_frame_pool));
}

table_frame_t* FrameBuffer::readFrame() {
    cur_frame_idx++;
    if (cur_frame_idx < 0) cur_frame_idx = 0;
    if (cur_frame_idx >= MAX_FRAMES) return nullptr;
    return &s_frame_pool[cur_frame_idx];
}

void FrameBuffer::currentToBuffers() {
    if (!buffers || !current) return;
    for (int ch = 0; ch < TOTAL_CH; ch++) {
        const int n = ch_pixel_counts[ch]; // 使用儲存的像素計數
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

// 核心計算：回傳 true 代表繼續播放，false 代表結束
bool FrameBuffer::compute(uint64_t t_ms) {
    // 初始化或第一次讀取
    if (current == nullptr) {
        current = readFrame();
        next = readFrame();
    }

    // 掉幀補償 (Skip frames)
    while (next != nullptr && t_ms >= next->timestamp) {
        current = next;
        next = readFrame();
    }

    // 結束條件
    if (next == nullptr) {
        currentToBuffers();
        return false; // 播放結束
    }

    // 如果不需要漸變，直接填入當前顏色 (fade == 0)
    if (!current->fade) {
        currentToBuffers();
        return true;
    }

    // HSV 漸變計算 (fade == 1)
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

    // 對每個通道、每個燈珠進行插值
    for (int ch = 0; ch < TOTAL_CH; ch++) {
        const int n = ch_pixel_counts[ch];
        uint8_t* out = buffers[ch];
        pixel_t* a = (current->data) ? current->data[ch] : nullptr;
        pixel_t* b = (next->data) ? next->data[ch] : nullptr;

        if (!out || !a || !b || n <= 0) continue;

        for (int pix = 0; pix < n; pix++) {
            // 轉換 current (a)
            const float r1 = a[pix].red   / 255.0f;
            const float g1 = a[pix].green / 255.0f;
            const float b1 = a[pix].blue  / 255.0f;
            float h1, s1, v1;
            rgb_to_hsv(r1, g1, b1, h1, s1, v1);

            // 轉換 next (b)
            const float r2 = b[pix].red   / 255.0f;
            const float g2 = b[pix].green / 255.0f;
            const float b2 = b[pix].blue  / 255.0f;
            float h2, s2, v2;
            rgb_to_hsv(r2, g2, b2, h2, s2, v2);

            // 插值
            const float h = lerp_hue_short(h1, h2, p);
            const float s = (1.0f - p) * s1 + p * s2;
            const float v = (1.0f - p) * v1 + p * v2;

            // 轉回 RGB
            float rr, gg, bb;
            hsv_to_rgb(h, s, v, rr, gg, bb);

            // 寫入 Buffer (注意 GRB 順序)
            out[3 * pix + 0] = f_to_u8(gg);
            out[3 * pix + 1] = f_to_u8(rr);
            out[3 * pix + 2] = f_to_u8(bb);
        }
    }
    
    return true; // 繼續播放
}


// 產生測試資料 (包含 Fade 邏輯設定)
void FrameBuffer::generateTestPattern() {
    auto is_fade_frame = [](int i0) -> bool {
        switch (i0) {
            case 4:  case 5:  case 6:
            case 9:
            case 12: case 13: case 14:
            case 15:
            case 22: case 23:
                return true;
            default:
                return false;
        }
    };

    // 1. 初始化指標
    for (int c = 0; c < 4; c++) {
        for (int ch = 0; ch < TOTAL_CH; ch++) {
            s_color_data[c][ch] = nullptr;
        }
    }

    // 2. 配置顏色資料 (紅/綠/藍/黑)
    for (int ch = 0; ch < TOTAL_CH; ch++) {
        const uint16_t n = ch_pixel_counts[ch];
        if (n == 0) continue;
        
        for (int c = 0; c < 4; c++) {
            s_color_data[c][ch] = (pixel_t*)malloc((size_t)n * sizeof(pixel_t));
            if (!s_color_data[c][ch]) {
                freeFrames();
                return;
            }
            for (int p = 0; p < n; p++) {
                if (c == 0)      s_color_data[c][ch][p] = { .green = 0, .red = BRIGHTNESS, .blue = 0 }; // Red
                else if (c == 1) s_color_data[c][ch][p] = { .green = BRIGHTNESS, .red = 0, .blue = 0 }; // Green
                else if (c == 2) s_color_data[c][ch][p] = { .green = 0, .red = 0, .blue = BRIGHTNESS }; // Blue
                else             s_color_data[c][ch][p] = { .green = 0, .red = 0, .blue = 0 };          // Black
            }
        }
    }

    // 3. 填入 Frame Pool
    for (int i = 0; i < MAX_FRAMES; i++) {
        const bool last_is_black = (i == (MAX_FRAMES - 1));
        const int color = last_is_black ? 3 : (i % 3);
        s_frame_pool[i].timestamp = (uint64_t)i * FRAME_PERIOD;
        s_frame_pool[i].fade = is_fade_frame(i);
        s_frame_pool[i].data = (pixel_t**)s_color_data[color];
    }
    ESP_LOGI(TAG, "Test pattern generated for %d frames", MAX_FRAMES);
}