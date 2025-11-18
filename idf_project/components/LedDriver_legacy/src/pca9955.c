#include "pca9955.h"

esp_err_t i2c_bus_init(int SCL_GPIO, int SDA_GPIO, i2c_master_bus_handle_t* ret_i2c_bus) {
    i2c_master_bus_config_t config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = SCL_GPIO,
        .sda_io_num = SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_new_master_bus(&config, ret_i2c_bus);
    return ESP_OK;
}
