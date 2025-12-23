#pragma once

#include "driver/rmt_encoder.h"

#include "BoardConfig.h"

esp_err_t rmt_new_encoder(rmt_encoder_handle_t* ret_encoder);
