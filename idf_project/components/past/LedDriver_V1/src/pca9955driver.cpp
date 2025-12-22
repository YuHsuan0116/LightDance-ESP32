#include "pca9955driver.hpp"

static const char* TAG = "pca9955driver";
static uint8_t pca_addr[MAX_PCA9955_NUM];
static uint8_t use_count[MAX_PCA9955_NUM] = {0};
static i2c_master_dev_handle_t dev_handles[MAX_PCA9955_NUM];
static int registered = 0;

// 1xxxxxxx: auto increment
static const uint8_t PWM_addr[5] = {0x88, 0x8B, 0x8E, 0x91, 0x94};

pca9955Driver::pca9955Driver() {}

esp_err_t pca9955Driver::config(const led_config_t config) {
    if(config.pca_channel > 4) {  // we only have 5 base addresses in PWM_addr[]
        return ESP_ERR_INVALID_ARG;
    }

    pca_channel = config.pca_channel;
    addr = config.gpio_or_addr;

    ESP_RETURN_ON_ERROR(i2c_master_get_bus_handle(I2C_NUM_0, &bus_handle), TAG, "get_bus_handle failed");

    int found = get_or_register_device(addr);
    if(found < 0) {
        return ESP_FAIL;  // already logged inside find() if you add logs
    }

    use_count[found]++;
    dev_handle = dev_handles[found];
    idx = found;
    return ESP_OK;
}

esp_err_t pca9955Driver::write(const color_t* colors) {
    if(need_reset_IREF) {
        need_reset_IREF = false;
        send_IREF();
    }
    cmd[0] = PWM_addr[pca_channel];
    cmd[1] = colors[0].red;
    cmd[2] = colors[0].green;
    cmd[3] = colors[0].blue;
    esp_err_t ret = i2c_master_transmit(dev_handle, cmd, sizeof(uint8_t) * 4, 100);
    if(ret != ESP_OK) {
        need_reset_IREF = true;
        return ret;
    }
    return ESP_OK;
}

esp_err_t pca9955Driver::detach() {
    if(idx < 0) {
        return ESP_ERR_INVALID_STATE;
    }

    if(--use_count[idx] == 0) {
        registered--;
        esp_err_t rm_err = i2c_master_bus_rm_device(dev_handles[idx]);
        // clear local registry slot regardless of rm_err, but still return error if any
        pca_addr[idx] = 0;
        dev_handles[idx] = NULL;
        idx = -1;
        dev_handle = nullptr;
        return rm_err;
    }

    idx = -1;
    dev_handle = nullptr;
    return ESP_OK;
}

esp_err_t pca9955Driver::wait_done() {
    return ESP_OK;
}

int pca9955Driver::get_or_register_device(uint8_t addr) {
    // 1) Already registered?
    for(int i = 0; i < MAX_PCA9955_NUM; i++) {
        if(pca_addr[i] == addr) {
            return i;
        }
    }

    // 2) Capacity check
    if(registered >= MAX_PCA9955_NUM) {
        return -1;
    }

    // 3) Add new device on the bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 100000,  // or 1000000 for Fm+
        .scl_wait_us = 0,        // add: silence -Wmissing-field-initializers
        .flags = {0},            // add: zero all flag bits
    };

    i2c_master_dev_handle_t dev = NULL;
    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev);
    if(err != ESP_OK || dev == NULL) {
        return -1;
    }

    // 4) Initialize device (set IREFALL)
    uint8_t local_cmd[2] = {IREFALL_addr, (uint8_t)OF_MAXIMUM_BRIGHTNESS};
    err = i2c_master_transmit(dev, local_cmd, sizeof(local_cmd), 10);
    if(err != ESP_OK) {
        (void)i2c_master_bus_rm_device(dev);  // best-effort cleanup
        return -1;
    }

    // 5) Commit to tables only after success
    int index = registered;
    pca_addr[index] = addr;
    dev_handles[index] = dev;
    registered++;
    return index;
}

esp_err_t pca9955Driver::set_GRB(const color_t& GRB) {
    esp_err_t ret = write(&GRB);
    return ret;
}

esp_err_t pca9955Driver::send_IREF() {
    uint8_t local_cmd[2] = {IREFALL_addr, (uint8_t)OF_MAXIMUM_BRIGHTNESS};
    esp_err_t err = i2c_master_transmit(dev_handle, local_cmd, sizeof(local_cmd), 10);
    return err;
}