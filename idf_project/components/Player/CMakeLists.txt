idf_component_register(
    SRCS "src/player.cpp" "src/state.cpp" "src/color.cpp" "src/framebuffer.cpp"

    INCLUDE_DIRS "include"

    REQUIRES LedController_v2 driver esp_driver_gptimer 
)