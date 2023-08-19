#pragma once

#include <esp_err.h>
#include <cstdint>
#include <driver/i2c.h>
#include <esp_err.h>
#include <esp_log.h>

void cam_i2c_init();
esp_err_t cam_i2c_receive_data(uint8_t *buf, uint32_t read_size);
void cam_client_i2c_task(void* pvParameters);
