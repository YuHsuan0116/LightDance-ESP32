idf_component_register(
    SRCS "src/frame_buffer.cpp"

    INCLUDE_DIRS "include"

    PRIV_INCLUDE_DIRS "src"

    REQUIRES LedController log

)