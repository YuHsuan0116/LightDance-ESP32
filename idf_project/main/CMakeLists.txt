idf_component_register(SRCS "player.cpp" "state.cpp" "main.cpp" 
                    INCLUDE_DIRS "."
                    REQUIRES  esp_timer driver LedController
                    )
