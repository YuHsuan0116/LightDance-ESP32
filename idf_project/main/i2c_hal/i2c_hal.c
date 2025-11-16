#include "i2c_hal.h"

static const uint8_t PWM_addr[5] = {0x88, 0x8B, 0x8E, 0x91, 0x94};

static i2c_dev_entry_t i2c_dev_list[8];
static int dev_count = 0;

i2c_master_dev_handle_t find_i2c_handle(uint8_t addr) {
    for(int i = 0; i < dev_count; i++) {
        if(i2c_dev_list[i].addr == addr) {
            return i2c_dev_list[i].i2c_dev_handle;
        }
    }
    return NULL;
}

esp_err_t i2c_bus_init(gpio_num_t i2c_gpio_sda, gpio_num_t i2c_gpio_scl, i2c_master_bus_handle_t* ret_i2c_bus_handle) {

    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = i2c_gpio_sda,
        .scl_io_num = i2c_gpio_scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_new_master_bus(&i2c_bus_config, ret_i2c_bus_handle);
    return ESP_OK;
}

esp_err_t i2c_write_hal(i2c_master_dev_handle_t i2c_dev_handle, uint8_t* const buffer, size_t const buffer_size, size_t const i2c_timeout_ms) {

    return i2c_master_transmit(i2c_dev_handle, buffer, buffer_size, i2c_timeout_ms);
}

esp_err_t pca9955b_write_IREFALL(uint8_t IREF_val, pca9955b_handle_t* pca9955b) {

    uint8_t cmd[2];
    uint8_t cmd_size = 2;

    cmd[0] = IREFALL_ADDR;
    cmd[1] = IREF_val;

    esp_err_t ret = i2c_write_hal(pca9955b->i2c_dev_handle, cmd, cmd_size, I2C_TIMEOUT_MS);
    if(ret == ESP_OK) {
        pca9955b->need_reset_IREF = false;
    }

    return ESP_OK;
}

esp_err_t pca9955b_write_rgb(uint8_t const r, uint8_t const g, uint8_t const b, pca9955b_handle_t* pca9955b) {

    if(pca9955b->need_reset_IREF == true) {
        printf("need reset!\n");
        pca9955b_write_IREFALL(0xff, pca9955b);
    }

    uint8_t cmd[4];
    uint8_t cmd_size = 4;

    cmd[0] = pca9955b->pca_channel;
    cmd[1] = r;
    cmd[2] = g;
    cmd[3] = b;

    esp_err_t ret = i2c_write_hal(pca9955b->i2c_dev_handle, cmd, cmd_size, I2C_TIMEOUT_MS);

    if(ret != ESP_OK) {
        pca9955b->need_reset_IREF = true;
    }

    return ESP_OK;
}

esp_err_t pca9955b_config(led_config_t* led_config, pca9955b_handle_t* pca9955b) {
    pca9955b->addr = led_config->i2c_addr;
    pca9955b->pca_channel = PWM_addr[led_config->pca_channel];
    pca9955b->need_reset_IREF = false;
    pca9955b->i2c_dev_handle = find_i2c_handle(pca9955b->addr);
    if(pca9955b->i2c_dev_handle == NULL) {
        i2c_master_bus_handle_t i2c_bus_handle;
        i2c_master_get_bus_handle(I2C_NUM_0, &i2c_bus_handle);

        i2c_device_config_t i2c_dev_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = pca9955b->addr,
            .scl_speed_hz = I2C_FREQ,
            .scl_wait_us = I2C_TIMEOUT_MS,
            .flags = {0},
        };

        i2c_master_bus_add_device(i2c_bus_handle, &i2c_dev_config, &i2c_dev_list[dev_count++].i2c_dev_handle);
        pca9955b->i2c_dev_handle = i2c_dev_list[dev_count - 1].i2c_dev_handle;
    }
    pca9955b_write_IREFALL(0xff, pca9955b);

    return ESP_OK;
}

esp_err_t pca9955b_del(pca9955b_handle_t* pca9955b) {
    return ESP_OK;
}