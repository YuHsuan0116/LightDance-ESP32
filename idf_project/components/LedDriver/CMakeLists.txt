idf_component_register(
    SRCS
        "src/channel_handle.c"
        "src/led_driver.c"
        "src/pca9955b_hal.c"
        "src/ws2812b_hal.c"
        "LedDriver_example.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        driver log esp_timer
)
