#include "inc/pca9955b_hal.h"

static const uint8_t PCA9955B_PWM_addr[5] = {0x88, 0x8B, 0x8E, 0x91, 0x94};

static i2c_dev_entry_t i2c_dev_list[PCA9955B_MAXIMUM_COUNT];

int8_t find_i2c_dev_list_idx(uint8_t addr) {
    for(int idx = 0; idx < PCA9955B_MAXIMUM_COUNT; idx++) {
        if(i2c_dev_list[idx].addr == addr) {
            return idx;
        }
    }
    return -1;
}

esp_err_t i2c_bus_init(gpio_num_t i2c_gpio_sda, gpio_num_t i2c_gpio_scl, i2c_master_bus_handle_t* ret_i2c_bus_handle) {
    esp_err_t ret = ESP_OK;

    if(i2c_gpio_sda == i2c_gpio_scl) {
        return ESP_ERR_INVALID_ARG;
    }

    *ret_i2c_bus_handle = NULL;

    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = i2c_gpio_sda,
        .scl_io_num = i2c_gpio_scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ret = i2c_new_master_bus(&i2c_bus_config, ret_i2c_bus_handle);
    if(ret != ESP_OK) {
        *ret_i2c_bus_handle = NULL;
        return ret;
    }

    return ESP_OK;
}

esp_err_t i2c_write_hal(i2c_master_dev_handle_t i2c_dev_handle, uint8_t* const buffer, size_t const buffer_size, size_t const i2c_timeout_ms) {
    esp_err_t ret = ESP_OK;

    if(i2c_dev_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    ret = i2c_master_transmit(i2c_dev_handle, buffer, buffer_size, i2c_timeout_ms);
    if(ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

esp_err_t pca9955b_write_IREFALL(uint8_t IREF_val, pca9955b_handle_t* pca9955b) {
    esp_err_t ret = ESP_OK;

    if(pca9955b == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(pca9955b->i2c_dev_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t cmd[2];
    uint8_t cmd_size = 2;
    cmd[0] = PCA9955B_IREFALL_ADDR;
    cmd[1] = IREF_val;

    ret = i2c_write_hal(pca9955b->i2c_dev_handle, cmd, cmd_size, I2C_TIMEOUT_MS);
    if(ret != ESP_OK) {
        return ret;
    }

    pca9955b->need_reset_IREF = false;

    return ESP_OK;
}

esp_err_t pca9955b_write_rgb(uint8_t const r, uint8_t const g, uint8_t const b, pca9955b_handle_t* pca9955b) {
    esp_err_t ret = ESP_OK;

    if(pca9955b == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(pca9955b->i2c_dev_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if(pca9955b->pca_channel >= 5) {
        return ESP_ERR_INVALID_ARG;
    }

    if(pca9955b->need_reset_IREF) {
        ret = pca9955b_write_IREFALL(0xff, pca9955b);
        if(ret != ESP_OK) {
            return ret;
        }
    }

    uint8_t cmd[4];
    uint8_t cmd_size = 4;
    cmd[0] = pca9955b->reg_addr;
    cmd[1] = r;
    cmd[2] = g;
    cmd[3] = b;

    ret = i2c_write_hal(pca9955b->i2c_dev_handle, cmd, cmd_size, I2C_TIMEOUT_MS);
    if(ret != ESP_OK) {
        pca9955b->need_reset_IREF = true;
        return ret;
    }

    return ESP_OK;
}

esp_err_t pca9955b_config(led_config_t* led_config, pca9955b_handle_t* pca9955b) {
    esp_err_t ret = ESP_OK;

    if(led_config == NULL || pca9955b == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if(led_config->pca_channel >= 5) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(pca9955b, 0, sizeof(pca9955b_handle_t));
    pca9955b->i2c_addr = led_config->i2c_addr;
    pca9955b->pca_channel = led_config->pca_channel;
    pca9955b->reg_addr = PCA9955B_PWM_addr[led_config->pca_channel];
    pca9955b->need_reset_IREF = true;
    pca9955b->dev_list_idx = find_i2c_dev_list_idx(pca9955b->i2c_addr);

    ret = pca9955b_add_i2c_dev_list(pca9955b);
    if(ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

esp_err_t pca9955b_del(pca9955b_handle_t* pca9955b) {
    esp_err_t ret = ESP_OK;
    if(pca9955b == NULL) {
        return ESP_OK;
    }

    if(pca9955b->i2c_dev_handle != NULL) {
        (void)pca9955b_write_rgb(0, 0, 0, pca9955b);
    }

    if(pca9955b->dev_list_idx >= 0 && pca9955b->dev_list_idx < PCA9955B_MAXIMUM_COUNT) {
        if(pca9955b->pca_channel < 5) {
            if(i2c_dev_list[pca9955b->dev_list_idx].used[pca9955b->pca_channel]) {
                i2c_dev_list[pca9955b->dev_list_idx].used[pca9955b->pca_channel] = false;
                i2c_dev_list[pca9955b->dev_list_idx].used_count--;

                if(i2c_dev_list[pca9955b->dev_list_idx].used_count == 0) {
                    if(i2c_dev_list[pca9955b->dev_list_idx].i2c_dev_handle != NULL) {
                        (void)i2c_master_bus_rm_device(i2c_dev_list[pca9955b->dev_list_idx].i2c_dev_handle);
                    }

                    memset(&i2c_dev_list[pca9955b->dev_list_idx], 0, sizeof(i2c_dev_entry_t));
                }
            }
        }
    }

    memset(pca9955b, 0, sizeof(pca9955b_handle_t));
    pca9955b->dev_list_idx = -1;

    return ESP_OK;
}

esp_err_t pca9955b_add_i2c_dev_list(pca9955b_handle_t* pca9955b) {
    esp_err_t ret = ESP_OK;

    if(pca9955b == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if(pca9955b->dev_list_idx != -1) {
        if(!i2c_dev_list[pca9955b->dev_list_idx].used[pca9955b->pca_channel]) {
            pca9955b->i2c_dev_handle = i2c_dev_list[pca9955b->dev_list_idx].i2c_dev_handle;
            i2c_dev_list[pca9955b->dev_list_idx].used[pca9955b->pca_channel] = true;
            i2c_dev_list[pca9955b->dev_list_idx].used_count++;
            return ESP_OK;
        } else {
            return ESP_ERR_INVALID_ARG;
        }

    } else {
        int8_t idx = -1;
        for(int i = 0; i < PCA9955B_MAXIMUM_COUNT; i++) {
            if(i2c_dev_list[i].used_count == 0) {
                idx = i;
                break;
            }
        }
        if(idx == -1) {
            return ESP_ERR_NO_MEM;
        }

        ret = i2c_dev_list_add(pca9955b->i2c_addr, idx);
        if(ret != ESP_OK) {
            return ret;
        }

        pca9955b->dev_list_idx = idx;
        pca9955b->i2c_dev_handle = i2c_dev_list[pca9955b->dev_list_idx].i2c_dev_handle;

        i2c_dev_list[idx].addr = pca9955b->i2c_addr;
        i2c_dev_list[pca9955b->dev_list_idx].used[pca9955b->pca_channel] = true;
        i2c_dev_list[pca9955b->dev_list_idx].used_count++;

        return ESP_OK;
    }
}

esp_err_t i2c_dev_list_add(uint8_t i2c_addr, int8_t i2c_dev_list_idx) {
    esp_err_t ret = ESP_OK;
    i2c_master_bus_handle_t i2c_bus_handle = NULL;

    if(i2c_dev_list_idx < 0 || i2c_dev_list_idx >= PCA9955B_MAXIMUM_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    ret = i2c_master_get_bus_handle(I2C_NUM_0, &i2c_bus_handle);
    if(ret != ESP_OK) {
        return ret;
    }
    if(i2c_bus_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t i2c_dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_addr,
        .scl_speed_hz = I2C_SCL_FREQ,
    };

    ret = i2c_master_bus_add_device(i2c_bus_handle, &i2c_dev_config, &i2c_dev_list[i2c_dev_list_idx].i2c_dev_handle);
    if(ret != ESP_OK) {
        return ret;
    }

    if(i2c_dev_list[i2c_dev_list_idx].i2c_dev_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}
