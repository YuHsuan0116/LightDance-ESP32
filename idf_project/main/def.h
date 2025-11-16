#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} color_t;

typedef enum {
    LED_TYPE_OF = 0,
    LED_TYPE_STRIP = 1,
} led_type_t;

typedef struct {
    led_type_t type;
    uint8_t led_count;
    uint8_t rmt_gpio;
    uint8_t i2c_addr;
    uint8_t pca_channel;
} led_config_t;

#ifdef __cplusplus
}
#endif