#include "cam_i2c_recv.h"

static const char* TAG = "CAMERA_I2C_RECV";

TaskHandle_t cam_i2c_task_handle = NULL;

constexpr static uint8_t ESP_CAM_I2C_ADDR = 0x54;
constexpr auto master_i2c_num = I2C_NUM_0;

constexpr static i2c_config_t conf_master = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = 32,  // !!!!!!!!!!
    .scl_io_num = 33,  // !!!!!!!!!!
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master =
        {
            .clk_speed = 500000,
        },
};

void cam_i2c_init()
{
    ESP_ERROR_CHECK(i2c_param_config(master_i2c_num, &conf_master));
    ESP_ERROR_CHECK(
        i2c_driver_install(master_i2c_num, conf_master.mode, 0, 0, 0));

    /* Create can task*/
    // xTaskCreate(&can_task, "can_task", 4096, NULL, 10, &can_task_handle);
    xTaskCreate(
        &cam_client_i2c_task, "can_task", 4096, NULL, 10, &cam_i2c_task_handle);
}

esp_err_t cam_i2c_receive_data(uint8_t* buf, uint32_t read_size)
{
    if (read_size == 0)
    {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_CAM_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    if (read_size > 1)
    {
        i2c_master_read(cmd, buf, read_size - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, buf + read_size - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret =
        i2c_master_cmd_begin(master_i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void cam_client_i2c_task(void* pvParameters)
{
    // cam_i2c_init();

    uint8_t buf[64] = {};

    while (1)
    {
        esp_err_t result = cam_i2c_receive_data(buf, sizeof(buf));
        ESP_LOGI(TAG, "I2C recv result: %d", result);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}
