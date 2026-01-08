#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // for size_t

// ============= Data Structures ===============

// GRB
typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} pixel_t;

// Keyframe
typedef struct {
    uint64_t timestamp; // ms
    bool fade;         
    pixel_t** data;     // data[ch][pixel]
} table_frame_t;

// ============= FrameBuffer Class ===============

class FrameBuffer {
public:
    FrameBuffer();
    ~FrameBuffer();
    void allocateBuffers(const uint16_t* pixel_counts, int num_channels);
    void freeBuffers();
    bool compute(uint64_t relative_time_ms);
    uint8_t** getRawBuffers() const { return buffers; }
    void reset();

private:
    // --- 內部變數 ---
    uint8_t** buffers = nullptr; // [ch][byte_index]
    int total_channels = 0;
    const uint16_t* ch_pixel_counts = nullptr; // 儲存外部傳入的像素數量

    // --- 播放狀態 ---
    int cur_frame_idx = -1;
    table_frame_t* current = nullptr;
    table_frame_t* next = nullptr;

    // --- 內部工具函式 ---
    table_frame_t* readFrame();   
    void freeFrames();
    void currentToBuffers();       

    void generateTestPattern();     // 產生測試光表
    
};