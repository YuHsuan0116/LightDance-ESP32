idf_component_register(SRCS "main.c" "src/pca9955b_hal.c" "src/ws2812b_hal.c" "src/channel_handle.c" "src/led_driver.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver sdmmc fatfs esp_timer
                    )
