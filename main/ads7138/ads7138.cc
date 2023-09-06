#include "ads7138.h"

#include <driver/i2c.h>

#include "registers.h"

/* @brief tag used for ESP serial console messages */
static const char TAG[] = "ADS7138";

TaskHandle_t ads7138_task_handle = NULL;

constexpr auto ads7138_i2c_num = I2C_NUM_1;
constexpr uint8_t ads7138_i2c_address = 0x11;  // R2 11k to GND
constexpr uint32_t ads7138_timeout_ms = 100;

constexpr i2c_config_t ads7138_i2c_config = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = 26,
    .scl_io_num = 27,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master =
        {
            .clk_speed = 400000,
        },
    .clk_flags = 0, // optional; you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here
};

void ads7138_init()
{
    ESP_LOGI(TAG, "Init start!");

    ESP_ERROR_CHECK(i2c_param_config(ads7138_i2c_num, &ads7138_i2c_config));
    ESP_ERROR_CHECK(i2c_driver_install(ads7138_i2c_num, ads7138_i2c_config.mode, 0, 0, 0));

    vTaskDelay(pdMS_TO_TICKS(100));

    /* All chanels as a analog input */
    ADS7138_DATA_CFG_regw_t w_register = {.CH_RST = 1};
    ads7138_write_data((uint8_t*)&w_register, sizeof(w_register));

    /* Set oversampling 3=4samples */
    ADS7138_OSR_CFG_regw_t w_register1 = {.OSR = 3};
    ads7138_write_data((uint8_t*)&w_register1, sizeof(w_register1));

    /* Channel auto sequencing */
    ADS7138_SEQUENCE_CFG_regw_t w_register2 = {.SEQ_START = 1, .SEQ_MODE = 1};
    ads7138_write_data((uint8_t*)&w_register2, sizeof(w_register2));

    /* start wifi manager task */
    ESP_LOGI(TAG, "IMU Task started!");

    xTaskCreate(&ads7138_task, "ads7138_task", 4096, NULL, 10, &ads7138_task_handle);
}

/**
 * @brief Main ads7138 Task - read output data from IMU
 *
 * @param pvParameters
 */
void ads7138_task(void* pvParameters)
{
    ads7138_struct data;

    while (1)
        {
            ads7138_read_data(RECENT_CH0_LSB, (uint8_t*)&data, sizeof(data));
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

    vTaskDelete(NULL);
}

/**
 * @brief Write data to ADS7138 register
 *
 * @param register_address
 * @param data
 * @param length
 */
void ads7138_write_data(const uint8_t* data, uint32_t length)
{
    auto cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ads7138_i2c_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, SINGLE_REG_WRITE, true);
    i2c_master_write_byte(cmd, data[0], true);
    i2c_master_write(cmd, data + 1, length - 1, true);
    i2c_master_stop(cmd);
    ESP_ERROR_CHECK(i2c_master_cmd_begin(ads7138_i2c_num, cmd, pdMS_TO_TICKS(ads7138_timeout_ms)));
    i2c_cmd_link_delete(cmd);
}

/**
 * @brief Read data from ADS7138 register
 *
 * @param register_address
 * @param data
 * @param length
 */
void ads7138_read_data(uint8_t register_address, uint8_t* data, uint32_t length)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ads7138_i2c_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, READ_CONTINOUS, true);
    i2c_master_write_byte(cmd, register_address, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ads7138_i2c_address << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, length, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    ESP_ERROR_CHECK(i2c_master_cmd_begin(ads7138_i2c_num, cmd, pdMS_TO_TICKS(ads7138_timeout_ms)));
    i2c_cmd_link_delete(cmd);
}
