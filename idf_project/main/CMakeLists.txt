idf_component_register(SRCS "main.c" "i2c_hal/i2c_hal.c" "rmt_hal/rmt_hal.c" "channel_handle/channel_hanlde.c" "led_driver/led_driver.c"
                    INCLUDE_DIRS "."
                    REQUIRES LedDriver sdmmc fatfs esp_timer
                    )
