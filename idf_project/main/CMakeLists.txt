idf_component_register(SRCS "main.c" "LedDriver/pca9955b_hal.c" "LedDriver/ws2812b_hal.c" "LedDriver/ws2812b_encoder.c"
                    INCLUDE_DIRS "."
                    REQUIRES  esp_timer driver
                    )
