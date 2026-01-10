#pragma once

#include "esp_err.h"

#include "LedController.hpp"
#include "color.h"

typedef struct __attribute__((packed)) {
    grb8_t ws2812b[WS2812B_NUM][100];
    grb8_t pca9955b[PCA9955B_CH_NUM];
} frame_data;

typedef struct {
    uint64_t timestamp;
    bool fade;
    frame_data data;
} table_frame_t;

class FrameBuffer {
  public:
    FrameBuffer();
    ~FrameBuffer();

    void init();
    void deinit();

    void compute(uint64_t time_ms);
    void render(LedController& controller);

    void print_buffer();
    void get_buffer(frame_data* p);

  private:
    table_frame_t current;
    table_frame_t next;

    frame_data buffer;
};

void test_read_frame(table_frame_t* p);

void print_table_frame(const table_frame_t& frame);
void print_frame_data(const frame_data& data);