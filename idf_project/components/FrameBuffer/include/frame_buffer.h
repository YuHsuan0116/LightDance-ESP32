#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // for size_t

// ==========================================
// 資料結構定義 (Data Structures)
// ==========================================

// 定義單一顆燈的顏色 (GRB)
typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} pixel_t;

// 定義一個影格 (Keyframe)
typedef struct {
    uint64_t timestamp; // 相對時間 (ms)
    bool fade;          // 是否需要漸變到下一幀
    pixel_t** data;     // 顏色資料指標 data[ch][pixel]
} table_frame_t;

// ==========================================
// FrameBuffer 類別定義
// ==========================================

class FrameBuffer {
public:
    FrameBuffer();
    ~FrameBuffer();
    void allocateBuffers(const uint16_t* pixel_counts, int num_channels);
    void freeBuffers();
    void generateTestPattern();
    bool compute(uint64_t relative_time_ms);
    uint8_t** getRawBuffers() const { return buffers; }
    void reset();

private:
    // --- 內部變數 ---
    uint8_t** buffers = nullptr; // 最終輸出的緩衝區 [ch][byte_index]
    int total_channels = 0;
    const uint16_t* ch_pixel_counts = nullptr; // 儲存外部傳入的像素數量資訊

    // --- 播放狀態 ---
    int cur_frame_idx = -1;
    table_frame_t* current = nullptr;
    table_frame_t* next = nullptr;

    // --- 內部工具函式 ---
    table_frame_t* readFrame();     // 讀取下一幀
    void currentToBuffers();        // 將 current 填入 buffers
    void freeFrames();              // 釋放靜態測試資料
};