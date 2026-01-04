#include "pca9955b_hal.h"

#include "string.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PCA9955B_PWM0_ADDR 0x08  // Address of PWM0 register
#define PCA9955B_AUTO_INC 0x80   // Auto-Increment for all registers
#define PCA9955B_IREFALL_ADDR 0x45

static const char* TAG = "PCA9955B";

esp_err_t pca9955b_init(uint8_t i2c_addr, i2c_master_bus_handle_t i2c_bus_handle, pca9955b_handle_t* pca9955b) {
    esp_err_t ret = ESP_OK;
    pca9955b_dev_t* dev = NULL;

    // 1. Input Validation
    ESP_RETURN_ON_FALSE(i2c_bus_handle, ESP_ERR_INVALID_ARG, TAG, "I2C bus handle is NULL");
    ESP_RETURN_ON_FALSE(pca9955b, ESP_ERR_INVALID_ARG, TAG, "Output handle pointer is NULL");

    *pca9955b = NULL;  // Initialize output to NULL for safety

    dev = (pca9955b_dev_t*)calloc(1, sizeof(pca9955b_dev_t));
    ESP_RETURN_ON_FALSE(dev, ESP_ERR_NO_MEM, TAG, "Failed to allocate memory for PCA9955B context");

    dev->i2c_addr = i2c_addr;
    dev->need_update = true;

    dev->need_reset_IREF = true;
    dev->IREF_cmd[0] = PCA9955B_IREFALL_ADDR;
    dev->IREF_cmd[1] = 0xFF;  // Default Max Current

    dev->buffer.command_byte = PCA9955B_PWM0_ADDR | PCA9955B_AUTO_INC;
    memset(dev->buffer.data, 0, sizeof(dev->buffer.data));

    i2c_device_config_t i2c_dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, /*!< 7-bit address mode */
        .device_address = i2c_addr,            /*!< Target device address */
        .scl_speed_hz = I2C_FREQ,              /*!< Bus clock frequency */
        .flags.disable_ack_check = false,      // We want to ensure device is connected
    };

    ESP_GOTO_ON_ERROR(i2c_master_bus_add_device(i2c_bus_handle, &i2c_dev_config, &dev->i2c_dev_handle), err, TAG, "Failed to add I2C device");

    // 1. Set IREF (Current Gain)
    uint8_t iref_cmd[2] = {PCA9955B_IREFALL_ADDR, 0xFF};
    ESP_GOTO_ON_ERROR(
        i2c_master_transmit(dev->i2c_dev_handle, iref_cmd, sizeof(iref_cmd), I2C_TIMEOUT_MS), err_dev, TAG, "Failed to set default IREF");
    dev->need_reset_IREF = false;

    // 2. Clear LEDs (Set Black)
    ESP_GOTO_ON_ERROR(i2c_master_transmit(dev->i2c_dev_handle, (uint8_t*)&dev->buffer, sizeof(pca9955b_buffer_t), I2C_TIMEOUT_MS),
                      err_dev,
                      TAG,
                      "Failed to clear LEDs (Black)");
    dev->need_update = false;

    ESP_LOGI(TAG, "Device initialized at address 0x%02x", i2c_addr);
    *pca9955b = dev;
    return ESP_OK;

err_dev:
    // If hardware init failed, remove the device from the bus to prevent leaks
    i2c_master_bus_rm_device(dev->i2c_dev_handle);
err:
    // Free the allocated memory
    free(dev);
    return ret;
}

esp_err_t pca9955b_set_pixel(pca9955b_handle_t pca9955b, uint8_t pixel_idx, uint8_t red, uint8_t green, uint8_t blue) {
    // 1. Validate Input
    ESP_RETURN_ON_FALSE(pca9955b, ESP_ERR_INVALID_ARG, TAG, "Handle is NULL");
    ESP_RETURN_ON_FALSE(pixel_idx < 5, ESP_ERR_INVALID_ARG, TAG, "Pixel index out of range (0-4)");

    // 2. Write to Shadow Buffer
    // Assuming RGB order. Adjust indices if your hardware is GRB.
    pca9955b->buffer.ch[pixel_idx][0] = red;   /*!< Red channel */
    pca9955b->buffer.ch[pixel_idx][1] = green; /*!< Green channel */
    pca9955b->buffer.ch[pixel_idx][2] = blue;  /*!< Blue channel */

    // 3. Set Dirty Flag
    // This tells the flush function that there is new data to send.
    pca9955b->need_update = true;

    return ESP_OK;
}

esp_err_t pca9955b_write(pca9955b_handle_t pca9955b, const uint8_t* _buffer) {
    // 1. Validate Input
    ESP_RETURN_ON_FALSE(pca9955b, ESP_ERR_INVALID_ARG, TAG, "Handle is NULL");
    ESP_RETURN_ON_FALSE(_buffer, ESP_ERR_INVALID_ARG, TAG, "Input buffer is NULL");

    // 2. Bulk Copy Logic
    // Copy 15 bytes (5 RGB LEDs x 3 colors) directly into the shadow buffer
    memcpy(pca9955b->buffer.data, _buffer, 15 * sizeof(uint8_t));

    // 3. Mark as Dirty
    pca9955b->need_update = true;

    return ESP_OK;
}

esp_err_t pca9955b_show(pca9955b_handle_t pca9955b) {
    esp_err_t ret = ESP_OK;

    // 1. Input Validation
    ESP_RETURN_ON_FALSE(pca9955b, ESP_ERR_INVALID_ARG, TAG, "Handle is NULL");

    // 2. Optimization: Skip if nothing changed AND hardware is healthy
    // If we don't need to update colors AND we don't need to restore IREF, return immediately.
    if(!pca9955b->need_update && !pca9955b->need_reset_IREF) {
        return ESP_OK;
    }

    // 3. IREF Restoration Logic (Recover from previous failure)
    if(pca9955b->need_reset_IREF) {
        ret = i2c_master_transmit(pca9955b->i2c_dev_handle, pca9955b->IREF_cmd, 2, I2C_TIMEOUT_MS);

        if(ret == ESP_OK) {
            pca9955b->need_reset_IREF = false; /*!< IREF reset completed */
            ESP_LOGI(TAG, "PCA9955B IREF recovered");
        } else {
            // If IREF fails, we can't show colors properly anyway.
            ESP_LOGW(TAG, "Failed to restore IREF: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    // 4. Transmit Buffer (Burst Write)
    // Send 16 bytes: Command Byte (PWM0 + AI) + 15 Color Bytes
    ret = i2c_master_transmit(pca9955b->i2c_dev_handle,
                              (uint8_t*)&pca9955b->buffer,
                              sizeof(pca9955b_buffer_t),  // Safer than hardcoding '16'
                              I2C_TIMEOUT_MS);

    if(ret != ESP_OK) {
        // 5. Error Handling & Recovery Prep
        // If transmission failed, assume device might have reset or disconnected.
        // Mark IREF to be re-sent next time we try to show.
        pca9955b->need_reset_IREF = true;
        ESP_LOGE(TAG, "I2C Transmit failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 6. Success: Clear the dirty flag
    pca9955b->need_update = false;

    return ESP_OK;
}

esp_err_t pca9955b_del(pca9955b_handle_t* pca9955b) {
    if(pca9955b == NULL || *pca9955b == NULL) {
        return ESP_OK; /*!< Nothing to delete */
    }

    pca9955b_dev_t* dev = *pca9955b; /*!< Local device pointer */

    // 1. Turn off all LEDs (Safety feature)
    // Clear the data payload
    memset(dev->buffer.data, 0, sizeof(dev->buffer.data));
    dev->need_update = true;
    pca9955b_show(dev);

    // 2. Remove from I2C Bus
    if(dev->i2c_dev_handle) {
        // Log warning if removal fails, but don't stop the deletion process
        esp_err_t err = i2c_master_bus_rm_device(dev->i2c_dev_handle);
        if(err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to remove I2C device: %s", esp_err_to_name(err));
        }
    }

    // 3. Free Memory
    free(dev);

    // 4. Invalidate Handle
    *pca9955b = NULL;

    ESP_LOGI(TAG, "PCA9955B deinitialized");
    return ESP_OK;
}

esp_err_t pca9955b_fill(pca9955b_handle_t pca9955b, uint8_t red, uint8_t green, uint8_t blue) {
    // 1. Input Validation
    ESP_RETURN_ON_FALSE(pca9955b, ESP_ERR_INVALID_ARG, TAG, "Handle is NULL");

    // 2. Loop through all 5 LEDs and set color
    for(int i = 0; i < 5; i++) {
        pca9955b->buffer.ch[i][0] = red;
        pca9955b->buffer.ch[i][1] = green;
        pca9955b->buffer.ch[i][2] = blue;
    }

    // 3. Mark as Dirty
    pca9955b->need_update = true;

    return ESP_OK;
}

esp_err_t i2c_bus_init(gpio_num_t i2c_gpio_sda, gpio_num_t i2c_gpio_scl, i2c_master_bus_handle_t* ret_i2c_bus_handle) {
    esp_err_t ret = ESP_OK;

    // 1. Input Validation
    ESP_RETURN_ON_FALSE(ret_i2c_bus_handle, ESP_ERR_INVALID_ARG, TAG, "Return handle pointer is NULL");

    // Check if pins are distinct
    ESP_RETURN_ON_FALSE(i2c_gpio_sda != i2c_gpio_scl, ESP_ERR_INVALID_ARG, TAG, "SDA and SCL cannot be the same pin");

    *ret_i2c_bus_handle = NULL; /*!< Clear output handle before initialization */

    // 2. Configuration
    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = I2C_NUM_0,                /*!< Use I2C port 0 */
        .sda_io_num = i2c_gpio_sda,           /*!< SDA GPIO pin */
        .scl_io_num = i2c_gpio_scl,           /*!< SCL GPIO pin */
        .clk_source = I2C_CLK_SRC_DEFAULT,    /*!< Select default clock source */
        .glitch_ignore_cnt = 7,               /*!< Glitch filter (typical value) */
        .flags.enable_internal_pullup = true, /*!< Enable internal weak pull-ups */
    };

    // 3. Install Driver
    ret = i2c_new_master_bus(&i2c_bus_config, ret_i2c_bus_handle);

    if(ret != ESP_OK) {
        *ret_i2c_bus_handle = NULL;
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C Bus initialized on SDA:%d SCL:%d", i2c_gpio_sda, i2c_gpio_scl);
    return ESP_OK;
}

void pca9955b_test1() {
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

void pca9955b_test2() {
    static const char* TAG_TEST = "PCA9955B_TEST";
    esp_err_t ret = ESP_OK;

    i2c_master_bus_handle_t bus_handle = NULL;
    ESP_LOGI(TAG_TEST, "Initializing I2C Bus...");

    ret = i2c_bus_init(GPIO_NUM_21, GPIO_NUM_22, &bus_handle);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG_TEST, "I2C Bus Init Failed");
        return;
    }

    pca9955b_handle_t pca9955b[PCA9955B_NUM] = {0};

    ESP_LOGI(TAG_TEST, "Initializing %d PCA9955B devices...", PCA9955B_NUM);

    for(int idx = 0; idx < PCA9955B_NUM; idx++) {
        uint8_t addr = BOARD_HW_CONFIG.i2c_addrs[idx];

        ret = pca9955b_init(addr, bus_handle, &pca9955b[idx]);

        if(ret != ESP_OK) {
            ESP_LOGE(TAG_TEST, "Device [%d] (Addr 0x%02X) Init Failed!", idx, addr);
            goto error_handling;
        }
        ESP_LOGI(TAG_TEST, "Device [%d] (Addr 0x%02X) Init Success", idx, addr);
    }

    ESP_LOGI(TAG_TEST, ">>> Pattern 1: Single Pixel Chaser (Check Addressing)");

    uint8_t test_colors[3][3] = {
        {50, 0, 0},  // R
        {0, 50, 0},  // G
        {0, 0, 50}   // B
    };

    for(int color_idx = 0; color_idx < 3; color_idx++) {
        uint8_t r = test_colors[color_idx][0];
        uint8_t g = test_colors[color_idx][1];
        uint8_t b = test_colors[color_idx][2];

        for(int chip_idx = 0; chip_idx < PCA9955B_NUM; chip_idx++) {
            for(int led_idx = 0; led_idx < 5; led_idx++) {
                pca9955b_fill(pca9955b[chip_idx], 0, 0, 0);

                pca9955b_set_pixel(pca9955b[chip_idx], led_idx, r, g, b);

                ret = pca9955b_show(pca9955b[chip_idx]);
                if(ret != ESP_OK) {
                    ESP_LOGE(TAG_TEST, "Show failed at Chip %d LED %d", chip_idx, led_idx);
                }

                vTaskDelay(pdMS_TO_TICKS(200));
            }
            pca9955b_fill(pca9955b[chip_idx], 0, 0, 0);
            pca9955b_show(pca9955b[chip_idx]);
        }
    }

    ESP_LOGI(TAG_TEST, ">>> Pattern 2: Full Color Cycle");

    uint8_t cycle_colors[3][3] = {{100, 0, 0}, {0, 100, 0}, {0, 0, 100}};

    for(int loop = 0; loop < 6; loop++) {
        int color_idx = loop % 3;

        ESP_LOGI(TAG_TEST, "Color: %s", (color_idx == 0) ? "RED" : (color_idx == 1) ? "GREEN" : "BLUE");

        for(int idx = 0; idx < PCA9955B_NUM; idx++) {
            pca9955b_fill(pca9955b[idx], cycle_colors[color_idx][0], cycle_colors[color_idx][1], cycle_colors[color_idx][2]);
            pca9955b_show(pca9955b[idx]);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG_TEST, "Test Finished. Cleaning up...");

error_handling:

    for(int idx = 0; idx < PCA9955B_NUM; idx++) {
        if(pca9955b[idx] != NULL) {
            ESP_LOGI(TAG_TEST, "Deleting device [%d]", idx);
            pca9955b_del(&pca9955b[idx]);
        }
    }

    if(bus_handle != NULL) {
        ESP_LOGI(TAG_TEST, "Deleting I2C Bus");
        i2c_del_master_bus(bus_handle);
    }

    ESP_LOGI(TAG_TEST, "Test Routine Complete.");
}