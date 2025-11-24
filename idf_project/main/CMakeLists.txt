idf_component_register(SRCS "player.cpp" "main.cpp" "fsm.c" "player_fsm.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES LedDriver esp_timer esp_driver_gptimer vfs sdmmc fatfs
                    )
