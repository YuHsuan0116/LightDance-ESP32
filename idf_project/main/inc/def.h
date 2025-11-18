#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RGB color representation.
 */
typedef struct {
    uint8_t green; /*!< Green component */
    uint8_t red;   /*!< Red component */
    uint8_t blue;  /*!< Blue component */
} color_t;

/**
 * @brief Supported LED driver types.
 */
typedef enum {
    LED_TYPE_OF = 0,    /*!< PCA9955B  */
    LED_TYPE_STRIP = 1, /*!< WS2812B LED strip */
} led_type_t;

/**
 * @brief Configuration parameters for an LED channel.
 */
typedef struct {
    led_type_t type;     /*!< LED driver type (PCA9955B or WS2812B) */
    uint8_t led_count;   /*!< Number of LEDs (used for WS2812B strips) */
    uint8_t rmt_gpio;    /*!< GPIO pin for RMT output (WS2812B only) */
    uint8_t i2c_addr;    /*!< I2C address (PCA9955B only) */
    uint8_t pca_channel; /*!< PCA9955B channel index (0-4) */
} led_config_t;

#ifdef __cplusplus
}
#endif