#include "pca9955b_hal.h"

#include "string.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PCA9955B_PWM0_ADDR 0x88  // with auto increment
#define PCA9955B_IREFALL_ADDR 0x45

esp_err_t pca9955b_init(uint8_t i2c_addr, i2c_master_bus_handle_t i2c_bus_handle, pca9955b_handle_t* pca9955b) {
    esp_err_t ret = ESP_OK;

    if(i2c_bus_handle == NULL) {
        return ESP_ERR_INVALID_STATE; /*!< I2C bus must be initialized before adding a PCA9955B device */
    }

    *pca9955b = NULL; /*!< Clear output handle to avoid dangling reference on failure */

    pca9955b_dev_t* dev = (pca9955b_dev_t*)calloc(1, sizeof(pca9955b_dev_t));
    if(dev == NULL) {
        return ESP_ERR_NO_MEM; /*!< Heap allocation failed */
    }

    dev->i2c_addr = i2c_addr;                  /*!< Store device address */
    dev->buffer.pwm_reg0 = PCA9955B_PWM0_ADDR; /*!< Set initial PWM register (header byte of 16-byte frame buffer) */

    dev->need_reset_IREF = true;              /*!< Mark IREF reset as pending */
    dev->IREF_cmd[0] = PCA9955B_IREFALL_ADDR; /*!< IREFALL register address */
    dev->IREF_cmd[1] = 0xff;                  /*!< Default IREF reset value */

    i2c_device_config_t i2c_dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, /*!< 7-bit address mode */
        .device_address = i2c_addr,            /*!< Target device address */
        .scl_speed_hz = I2C_FREQ,              /*!< Bus clock frequency */
    };

    ret = i2c_master_bus_add_device(i2c_bus_handle, &i2c_dev_config, &dev->i2c_dev_handle);
    if(ret != ESP_OK) {
        free(dev);
        return ret; /*!< Propagate I2C device creation error */
    }

    *pca9955b = dev; /*!< Return created device handle */

    ret = i2c_master_transmit(dev->i2c_dev_handle, dev->IREF_cmd, 2, I2C_TIMEOUT_MS);
    if(ret == ESP_OK) {
        dev->need_reset_IREF = false; /*!< IREF reset completed */
    }

    return ESP_OK;
}

esp_err_t pca9955b_set_pixel(pca9955b_handle_t pca9955b, uint8_t pixel_idx, uint8_t red, uint8_t green, uint8_t blue) {
    pca9955b->buffer.ch[pixel_idx][0] = red;   /*!< Write R byte */
    pca9955b->buffer.ch[pixel_idx][1] = green; /*!< Write G byte */
    pca9955b->buffer.ch[pixel_idx][2] = blue;  /*!< Write B byte */

    return ESP_OK;
}
esp_err_t pca9955b_write(pca9955b_handle_t pca9955b, uint8_t* _buffer) {
    memcpy(pca9955b->buffer.data, _buffer, 15 * sizeof(uint8_t));

    return ESP_OK;
}

esp_err_t pca9955b_show(pca9955b_handle_t pca9955b) {
    esp_err_t ret = ESP_OK;

    if(pca9955b == NULL) {
        return ESP_ERR_INVALID_ARG; /*!< Handle must not be NULL */
    }

    /* Send IREF reset command if needed */
    if(pca9955b->need_reset_IREF) {
        ret = i2c_master_transmit(pca9955b->i2c_dev_handle, pca9955b->IREF_cmd, 2, I2C_TIMEOUT_MS);
        if(ret == ESP_OK) {
            pca9955b->need_reset_IREF = false; /*!< IREF reset completed */
        } else {
            return ret; /*!< Propagate I2C transmit error */
        }
    }

    /* Transmit full 16-byte frame buffer */
    ret = i2c_master_transmit(pca9955b->i2c_dev_handle, (uint8_t*)&pca9955b->buffer, 16, I2C_TIMEOUT_MS);
    if(ret != ESP_OK) {
        pca9955b->need_reset_IREF = true; /*!< Re-arm IREF reset on failure */
        return ret;
    }
    return ESP_OK;
}

esp_err_t pca9955b_del(pca9955b_handle_t* pca9955b) {
    if(pca9955b == NULL || *pca9955b == NULL) {
        return ESP_OK; /*!< Nothing to delete */
    }
    pca9955b_dev_t* dev = *pca9955b; /*!< Local device pointer */

    memset(dev->buffer.data, 0, 15 * sizeof(uint8_t)); /* Clear 15-byte channel color payload (PWM header byte is kept untouched) */
    pca9955b_show(dev);                                /* Flush cleared frame to LEDs (ignore errors) */
    i2c_master_bus_rm_device(dev->i2c_dev_handle);     /* Remove device from I2C bus */

    free(dev); /* Free heap memory and invalidate caller handle */
    *pca9955b = NULL;
    return ESP_OK;
}

esp_err_t pca9955b_fill(pca9955b_handle_t pca9955b, uint8_t red, uint8_t green, uint8_t blue) {
    for(int i = 0; i < 5; i++) {
        pca9955b->buffer.ch[i][0] = red;
        pca9955b->buffer.ch[i][1] = green;
        pca9955b->buffer.ch[i][2] = blue;
    }
    return ESP_OK;
}

esp_err_t i2c_bus_init(gpio_num_t i2c_gpio_sda, gpio_num_t i2c_gpio_scl, i2c_master_bus_handle_t* ret_i2c_bus_handle) {
    esp_err_t ret = ESP_OK;

    if(i2c_gpio_sda == i2c_gpio_scl) {
        return ESP_ERR_INVALID_ARG;
    }

    *ret_i2c_bus_handle = NULL; /*!< Clear output handle before initialization */

    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = I2C_NUM_0,                /*!< I2C hardware port number */
        .sda_io_num = i2c_gpio_sda,           /*!< SDA GPIO pin */
        .scl_io_num = i2c_gpio_scl,           /*!< SCL GPIO pin */
        .clk_source = I2C_CLK_SRC_DEFAULT,    /*!< Clock source selection */
        .glitch_ignore_cnt = 7,               /*!< Glitch filter threshold */
        .flags.enable_internal_pullup = true, /*!< Enable internal pull-ups */
    };

    ret = i2c_new_master_bus(&i2c_bus_config, ret_i2c_bus_handle);
    if(ret != ESP_OK) {
        *ret_i2c_bus_handle = NULL;
        return ret; /*!< Propagate I2C bus creation error */
    }

    return ESP_OK;
}

void pca9955b_test() {
    i2c_master_bus_handle_t bus_handle;
    i2c_bus_init(GPIO_NUM_21, GPIO_NUM_22, &bus_handle);

    pca9955b_handle_t pca9955b[PCA9955B_NUM];

    uint8_t r[3] = {15, 0, 0};
    uint8_t g[3] = {0, 15, 0};
    uint8_t b[3] = {0, 0, 15};

    esp_err_t ret = ESP_OK;
    ESP_LOGI("pca9955b_test", "testing pca9955b_init");

    for(int idx = 0; idx < PCA9955B_NUM; idx++) {
        ret = pca9955b_init(BOARD_HW_CONFIG.i2c_addrs[idx], bus_handle, &pca9955b[idx]);
        if(ret != ESP_OK) {
            ESP_LOGE("pca9955b_test", "pca9955b_init failed");
        }
    }
    ESP_LOGI("pca9955b_test", "pca9955b_init successed");

    ESP_LOGI("pca9955b_test", "testing pca9955b_set_pixel and pca9955b_show");
    for(int i = 0; i < 30; i++) {
        for(int idx = 0; idx < PCA9955B_NUM; idx++) {
            for(int pixel_idx = 0; pixel_idx < 5; pixel_idx++) {
                pca9955b_set_pixel(pca9955b[idx], pixel_idx, r[i % 3], g[i % 3], b[i % 3]);
            }
        }

        for(int idx = 0; idx < PCA9955B_NUM; idx++) {
            pca9955b_show(pca9955b[idx]);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI("pca9955b_test", "pca9955b_set_pixel and pca9955b_show successed");

    ESP_LOGI("pca9955b_test", "testing pca9955b_del");
    for(int idx = 0; idx < PCA9955B_NUM; idx++) {
        pca9955b_del(&pca9955b[idx]);
    }

    ESP_LOGI("pca9955b_test", "pca9955b_del successed");

    i2c_del_master_bus(bus_handle);
}