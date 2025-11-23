idf_component_register(SRCS "main.c" "fsm.c"
                    INCLUDE_DIRS "."
                    REQUIRES LedDriver esp_timer esp_driver_gptimer vfs sdmmc fatfs
                    )
