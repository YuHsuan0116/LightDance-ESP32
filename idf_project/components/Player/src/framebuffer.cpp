#include "framebuffer.h"

#include "algorithm"
#include "esp_log.h"

static const char* TAG = "fb";

void swap(table_frame_t* a, table_frame_t* b) {
    table_frame_t tmp = *a;
    *a = *b;
    *b = tmp;
}

FrameBuffer::FrameBuffer() {}
FrameBuffer::~FrameBuffer() {}

void FrameBuffer::init() {
    test_read_frame(&current);
    // print_table_frame(current);

    test_read_frame(&next);
    // print_table_frame(next);

    compute(0);
}

void FrameBuffer::deinit() {
    memset(&current, 0, sizeof(table_frame_t));
    memset(&next, 0, sizeof(table_frame_t));
    memset(&buffer, 0, sizeof(frame_data));
}

void FrameBuffer::compute(uint64_t time_ms) {
    // ESP_LOGI("fb", "current: %llu, now: %llu, next: %llu", current.timestamp, time_ms, next.timestamp);
    if(time_ms >= next.timestamp) {
        swap(&current, &next);
        test_read_frame(&next);
        // print_table_frame(next);
    }

    const uint64_t t1 = current.timestamp;
    const uint64_t t2 = next.timestamp;
    uint8_t p = 0;

    if(current.fade && t2 > t1) {
        uint64_t dt = time_ms - t1;
        uint64_t dur = t2 - t1;
        if(dt >= dur) {
            p = 255;
        } else {
            p = (uint8_t)(dt * 255 / dur);
        }
    }

    for(int ch_idx = 0; ch_idx < WS2812B_NUM; ch_idx++) {
        for(int pixel_idx = 0; pixel_idx < ch_info.rmt_strips[ch_idx]; pixel_idx++) {
            buffer.ws2812b[ch_idx][pixel_idx] = grb_lerp_hsv_u8(current.data.ws2812b[ch_idx][pixel_idx], next.data.ws2812b[ch_idx][pixel_idx], p);
        }
    }
    for(int ch_idx = 0; ch_idx < PCA9955B_CH_NUM; ch_idx++) {
        buffer.pca9955b[ch_idx] = grb_lerp_hsv_u8(current.data.pca9955b[ch_idx], next.data.pca9955b[ch_idx], p);
    }

    // print_frame_data(buffer);
}

void FrameBuffer::render(LedController& controller) {
    for(int i = 0; i < WS2812B_NUM; i++) {
        controller.write_buffer(i, (uint8_t*)buffer.ws2812b[i]);
    }

    for(int i = 0; i < PCA9955B_CH_NUM; i++) {
        controller.write_buffer(i + WS2812B_NUM, (uint8_t*)&buffer.pca9955b[i]);
    }
}

void FrameBuffer::get_buffer(frame_data* p) {
    memcpy(p, &buffer, sizeof(frame_data));
}

void print_table_frame(const table_frame_t& frame) {
    ESP_LOGI(TAG, "=== table_frame_t ===");
    ESP_LOGI(TAG, "timestamp : %" PRIu64 " ms", frame.timestamp);
    ESP_LOGI(TAG, "fade      : %s", frame.fade ? "true" : "false");
    print_frame_data(frame.data);
    ESP_LOGI(TAG, "=====================");
}

void print_frame_data(const frame_data& data) {
    ESP_LOGI(TAG, "[WS2812]");
    for(int ch = 0; ch < WS2812B_NUM; ch++) {
        int len = ch_info.rmt_strips[ch];

        if(len < 0)
            len = 0;
        if(len > 100)
            len = 100;

        int dump = (len > 5) ? 5 : len;

        ESP_LOGI(TAG, "  CH %d (len=%d):", ch, len);
        for(int i = 70; i < 70 + dump; i++) {
            const grb8_t& p = data.ws2812b[ch][i];
            ESP_LOGI(TAG, "    [%d] G=%u R=%u B=%u", i, p.g, p.r, p.b);
        }
        if(dump < len) {
            ESP_LOGI(TAG, "    ...");
        }
    }

    ESP_LOGI(TAG, "[PCA9955]");
    for(int i = 0; i < PCA9955B_CH_NUM; i++) {
        const grb8_t& p = data.pca9955b[i];
        ESP_LOGI(TAG, "  CH %2d: G=%u R=%u B=%u", i, p.g, p.r, p.b);
    }
}

void FrameBuffer::print_buffer() {
    print_frame_data(buffer);
}

static uint8_t brightness = 63;

static grb8_t red = {.g = 0, .r = brightness, .b = 0};
static grb8_t green = {.g = brightness, .r = 0, .b = 0};
static grb8_t blue = {.g = 0, .r = 0, .b = brightness};
static grb8_t color_pool[3] = {red, green, blue};

static int count = 0;

void test_read_frame(table_frame_t* p) {
    p->timestamp = count * 2000;
    p->fade = true;
    for(int ch_idx = 0; ch_idx < WS2812B_NUM; ch_idx++) {
        for(int i = 0; i < ch_info.rmt_strips[ch_idx]; i++) {
            p->data.ws2812b[ch_idx][i] = color_pool[count % 3];
        }
    }
    for(int ch_idx = 0; ch_idx < PCA9955B_CH_NUM; ch_idx++) {
        if(ch_info.i2c_leds[ch_idx]) {
            p->data.pca9955b[ch_idx] = color_pool[count % 3];
        }
    }
    count++;
}