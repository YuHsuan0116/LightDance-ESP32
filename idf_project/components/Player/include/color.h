#pragma once

#include <stdint.h>

typedef struct {
    uint8_t green, red, blue;
} pixel_t;

typedef struct __attribute__((packed)) {
    uint8_t g, r, b;
} grb8_t;

typedef struct __attribute__((packed)) {
    uint16_t h;  // h: 0..1535
    uint8_t s, v;
} hsv8_t;

// ============= math helper function =============

static inline uint8_t u8_max3(uint8_t a, uint8_t b, uint8_t c) {
    uint8_t m = (a > b) ? a : b;
    return (m > c) ? m : c;
}

static inline uint8_t u8_min3(uint8_t a, uint8_t b, uint8_t c) {
    uint8_t m = (a < b) ? a : b;
    return (m < c) ? m : c;
}

// (x * y) / 255 with rounding, inputs 0..255 => output 0..255
static inline uint8_t mul255_u8(uint8_t x, uint8_t y) {
    return (uint8_t)(((uint16_t)x * (uint16_t)y + 127) / 255);
}

// (x * y) / 255 with rounding, x up to 255, y up to 255, but return 16-bit if needed
static inline uint16_t mul255_u16(uint16_t x, uint16_t y) {
    return (uint16_t)((x * y + 127) / 255);
}

static inline uint8_t lerp_u8(uint8_t start, uint8_t end, uint8_t t) {
    uint16_t val = (uint16_t)start * (255 - t) + (uint16_t)end * t;
    return (uint8_t)((val + 127) / 255);
}

static inline uint16_t wrap_h_1536(int32_t h) {
    // normalize to [0,1535]
    h %= 1536;
    if(h < 0)
        h += 1536;
    return (uint16_t)h;
}

static inline int16_t shortest_dh_1536(int16_t dh) {
    // map to [-768, +767] for shortest path
    if(dh > 768)
        dh -= 1536;
    if(dh < -768)
        dh += 1536;
    return dh;
}

// ============= color function =============

hsv8_t grb_to_hsv_u8(grb8_t in);
grb8_t hsv_to_grb_u8(hsv8_t in);
grb8_t grb_lerp_hsv_u8(grb8_t start, grb8_t end, uint8_t t);
grb8_t grb_lerp_u8(grb8_t start, grb8_t end, uint8_t t);