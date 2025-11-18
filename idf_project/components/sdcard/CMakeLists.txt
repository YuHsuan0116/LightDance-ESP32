idf_component_register(
    SRCS
        "src/sdcard.cpp"
    INCLUDE_DIRS
        "include"
    REQUIRES
        vfs sdmmc fatfs esp_timer
)
