#include "../ads7138/ads7138.h"
#include "../as5055/as5055.h"
#include "../esc/esc.h"
#include "../vl6180/vl6180.h"
#include "../mpu6500/mpu6500.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ADC
void ads7138_task(void* pvParameters) {
    ads7138_init();
    ads7138_struct data;

    while (1) {
        // ads7138_read_data(RECENT_CH0_LSB, (uint8_t*)&data, sizeof(data));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

// Enkoder
void as5055_test_task(void* pvParameters) {
    const char *TAG = "AS5055 task";

    as5055_init();

    float angle = 0;

    // Soft reset
    as5055_soft_reset();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));

        // Czytanie kąta
        auto data = as5055_read_angle_data();

        if (!as5055_validate_response(data)) {
            as5055_clear_error();
        } else {
            angle = as5055_convert_angle(data);
        }

        ESP_LOGI(TAG, "Angle hex %x", data);
        ESP_LOGI(TAG, "Angle: %f", angle);
    }
    vTaskDelete(NULL);
}

// TURBO KURWA
void esc_task(void* pvParameters) {
    esc_init();

    // float speed = 0.0f;
    while (1) {
        // if (speed < 1.0f) speed += 0.1f;
        escSpeed(0.1f);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

// IMU
void mpu6500_task(void* pvParameters)
{
    mpu6500_init();

    mpu6500_data data;
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

// Distance sensor
void vl6180_task(void* pvParameters) {
    vl6180_init();
    uint8_t data;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        vl6180_req_meas();
        vTaskDelay(pdMS_TO_TICKS(10));
        data = vl6180_take_distance();
        ESP_LOGI(TAG, "Distance: %u", data );
    }

    vTaskDelete(NULL);
}
