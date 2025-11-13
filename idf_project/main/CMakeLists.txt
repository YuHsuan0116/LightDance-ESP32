idf_component_register(SRCS "main.cpp" "sdcard.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES LedDriver sdmmc fatfs esp_timer
                    )
