idf_component_register(
    SRCS "src/player.cpp" "src/state.cpp"

    INCLUDE_DIRS "include"

    REQUIRES LedController driver esp_driver_gptimer
)