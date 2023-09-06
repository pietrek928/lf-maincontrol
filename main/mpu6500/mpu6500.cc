#include "mpu6500.h"
#include "registers.h"
#include "types.h"
#include "../utils.h"

#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


/* @brief tag used for ESP serial console messages */
static const char TAG[] = "MPU6500";

TaskHandle_t mpu6500_task_handle = NULL;

const float TsACCEL = 0.001f;  // 1kHz
const float TsGYRO = 0.0029f;  //184 Hz

constexpr auto mpu6500_i2c_num = I2C_NUM_1;
constexpr uint8_t mpu6500_i2c_address = 0x68;
constexpr uint32_t mpu6500_timeout_ms = 100;

constexpr i2c_config_t mpu6500_i2c_config = {
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

struct mpu6500_calc CalcDataMPU6500;

void mpu6500_init()
{
    ESP_LOGI(TAG, "Init start!");

    ESP_ERROR_CHECK(i2c_param_config(mpu6500_i2c_num, &mpu6500_i2c_config));
    ESP_ERROR_CHECK(i2c_driver_install(mpu6500_i2c_num, mpu6500_i2c_config.mode, 0, 0, 0));

    vTaskDelay(pdMS_TO_TICKS(100));

    // set clock source
    mpu6500_set_bits(PWR_MGMT1, PWR1_CLKSEL_BIT, PWR1_CLKSEL_LENGTH, CLOCK_PLL);

    // gyroscope set full scale range - 184 Hz bo filtr na w opcji 1
    mpu6500_set_bits(GYRO_CONFIG, GCONFIG_FS_SEL_BIT, GCONFIG_FS_SEL_LENGTH, GYRO_FS_500DPS);

    // accelerometer - 4G - sampling rate is 1kHz
    mpu6500_set_bits(ACCEL_CONFIG, ACONFIG_FS_SEL_BIT, ACONFIG_FS_SEL_LENGTH, ACCEL_FS_4G);

    // low pass filter - 188Hz
    mpu6500_set_bits(CONFIG, CONFIG_DLPF_CFG_BIT, CONFIG_DLPF_CFG_LENGTH, DLPF_188HZ);
}

/**
 * @brief Main MPU6500 Task - read output data from IMU
 *
 * @param pvParameters
 */
void mpu6500_test_task(void* pvParameters)
{
    mpu6500_init();

    struct mpu6500_data data;
    float temperature;

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        data = mpu6500_read_sensors();

        ESP_LOGI(TAG, "Data form ACCEL XYZ: %u, %u, %u ", data.accel_x_be, data.accel_y_be, data.accel_z_be);
        ESP_LOGI(TAG, "Data form TEMP: %u", data.temp_be);
        ESP_LOGI(TAG, "Data form GYRO XYZ: %u, %u, %u ", data.gyro_x_be, data.gyro_y_be, data.gyro_z_be);

        /* Recalculate raw data to more complex form */
        mpu6500_calculate_results(TsACCEL, TsGYRO, data, 0);

        ESP_LOGI(TAG, "SPEED XYZ: %f, %f, %f ", CalcDataMPU6500.speedX, CalcDataMPU6500.speedY, CalcDataMPU6500.speedZ);
        ESP_LOGI(TAG, "POSITION XYZ: %f, %f, %f ", CalcDataMPU6500.positionX, CalcDataMPU6500.positionY, CalcDataMPU6500.positionZ);

        temperature = mpu6500_temp_to_celsius(data.temp_be);
        ESP_LOGI(TAG, "Temp %f", temperature);
    }

    vTaskDelete(NULL);
}

/**
 * @brief Set bits in MPU6500 register, flag by flag without cleaning already set data
 *
 * @param register_address
 * @param data
 * @param length
 */
void mpu6500_set_bits(uint8_t register_address, uint8_t start_bit, uint8_t bit_length, uint8_t value)
{
    uint8_t mask = ((1 << bit_length) - 1) << start_bit;
    value <<= start_bit;
    value &= mask;

    uint8_t reg_value = 0;
    mpu6500_read_data(register_address, &reg_value, 1);
    reg_value = (reg_value & ~mask) | value;
    mpu6500_write_data(register_address, &reg_value, 1);
}

/**
 * @brief Write data to MPU6500 register
 *
 * @param register_address
 * @param data
 * @param length
 */
void mpu6500_write_data(uint8_t register_address, const uint8_t *data, uint32_t length)
{
    auto cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (mpu6500_i2c_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, register_address, true);
    i2c_master_write(cmd, data, length, true);
    i2c_master_stop(cmd);
    ESP_ERROR_CHECK(i2c_master_cmd_begin(mpu6500_i2c_num, cmd, pdMS_TO_TICKS(mpu6500_timeout_ms)));
    i2c_cmd_link_delete(cmd);
}

/**
 * @brief Read data from MPU6500 register
 *
 * @param register_address
 * @param data
 * @param length
 */
void mpu6500_read_data(uint8_t register_address, uint8_t *data, uint32_t length)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (mpu6500_i2c_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, register_address, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (mpu6500_i2c_address << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, length, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    ESP_ERROR_CHECK(i2c_master_cmd_begin(mpu6500_i2c_num, cmd, pdMS_TO_TICKS(mpu6500_timeout_ms)));
    i2c_cmd_link_delete(cmd);
}

mpu6500_data mpu6500_read_sensors() {
    mpu6500_data data;
    mpu6500_read_data(ACCEL_XOUT_H, (uint8_t *)&data, sizeof(data));
    return data;
}

uint16_t mpu6500_read_data_ACCEL_X()
{
    uint16_t data;

    mpu6500_read_data(ACCEL_XOUT_H, (uint8_t *)&data, sizeof(data));

    return ( data << 8  ) | ( data >> 8 );
}

uint16_t mpu6500_read_data_ACCEL_Y()
{
    uint16_t data;

    mpu6500_read_data(ACCEL_YOUT_H, (uint8_t *)&data, sizeof(data));

    return ( data << 8  ) | ( data >> 8 );
}

uint16_t mpu6500_read_data_ACCEL_Z()
{
    uint16_t data;

    mpu6500_read_data(ACCEL_ZOUT_H, (uint8_t *)&data, sizeof(data));

    return ( data << 8  ) | ( data >> 8 );
}

uint16_t mpu6500_read_data_TEMP()
{
    uint16_t data;

    mpu6500_read_data(TEMP_OUT_H, (uint8_t *)&data, sizeof(data));

    return ( data << 8  ) | ( data >> 8 );
}

uint16_t mpu6500_read_data_GYRO_X()
{
    uint16_t data;

    mpu6500_read_data(GYRO_XOUT_H, (uint8_t *)&data, sizeof(data));

    return ( data << 8  ) | ( data >> 8 );
}

uint16_t mpu6500_read_data_GYRO_Y()
{
    uint16_t data;

    mpu6500_read_data(GYRO_YOUT_H, (uint8_t *)&data, sizeof(data));

    return ( data << 8  ) | ( data >> 8 );
}

uint16_t mpu6500_read_data_GYRO_Z()
{
    uint16_t data;

    mpu6500_read_data(GYRO_ZOUT_H, (uint8_t *)&data, sizeof(data));

    return ( data << 8  ) | ( data >> 8 );
}

/**
 * @brief Lets make all necessary calculations here
 *
 * @param tsACCEL
 * @param tsGYRO
 * @param data
 * @param gravityAcc
 */
void mpu6500_calculate_results(float tsACCEL, float tsGYRO, mpu6500_data& data, uint16_t gravityAcc)
{
    CalcDataMPU6500.speedX += tsACCEL * (data.accel_x_be - gravityAcc);
    CalcDataMPU6500.positionX += tsACCEL * CalcDataMPU6500.speedX;

    CalcDataMPU6500.speedY += tsACCEL * (data.accel_y_be - gravityAcc);
    CalcDataMPU6500.positionY += tsACCEL * CalcDataMPU6500.speedY;

    CalcDataMPU6500.speedZ += tsACCEL * (data.accel_z_be - gravityAcc);
    CalcDataMPU6500.positionZ += tsACCEL * CalcDataMPU6500.speedZ;
}

/**
 * @brief Show celcius temperature
 *
 */
float mpu6500_temp_to_celsius(uint16_t temp_be) {
    constexpr static float kCelsiusOffset    = 21.f;     // ºC
    constexpr static int16_t kRoomTempOffset = 0;        // LSB
    constexpr static float kTempResolution   = 98.67f / INT16_MAX;
    // constexpr static float kFahrenheitOffset = kCelsiusOffset * 1.8f + 32;  // ºF

    return ((float)(swap_bytes(temp_be) - kRoomTempOffset)* kTempResolution + kCelsiusOffset);
}
