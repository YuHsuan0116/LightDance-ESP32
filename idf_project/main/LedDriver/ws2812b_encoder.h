#pragma once

#include "driver/rmt_encoder.h"

#define WS2812B_RESOLUTION 10000000

esp_err_t rmt_new_encoder(rmt_encoder_handle_t* ret_encoder);
