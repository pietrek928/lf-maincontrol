#pragma once

struct mpu6500_data {
    uint16_t accel_x_be, accel_y_be, accel_z_be;
    uint16_t temp;
    uint16_t gyro_x_be, gyro_y_be, gyro_z_be;
};

struct mpu6500_calc {
    float positionX, positionY, positionZ;
    float speedX, speedY, speedZ;
};

float tempCelsius(uint16_t& temp);
void mpu6500_init();
void mpu6500_task(void* pvParameters);
void mpu6500_set_bits(uint8_t register_address, uint8_t start_bit, uint8_t bit_length, uint8_t value);
void mpu6500_write_data(uint8_t register_address, const uint8_t *data, uint32_t length);
void mpu6500_read_data(uint8_t register_address, uint8_t *data, uint32_t length);
uint16_t mpu6500_read_data_ACCEL_X();
uint16_t mpu6500_read_data_ACCEL_Y();
uint16_t mpu6500_read_data_ACCEL_Z();
uint16_t mpu6500_read_data_TEMP();
uint16_t mpu6500_read_data_GYRO_X();
uint16_t mpu6500_read_data_GYRO_Y();
uint16_t mpu6500_read_data_GYRO_Z();
void mpu6500_calculate_results(float tsACCEL, float tsGYRO, mpu6500_data& data, uint16_t gravityAcc);



