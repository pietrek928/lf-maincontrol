#pragma once

#include <driver/i2c.h>
#include <esp_log.h>

#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct __attribute__((packed)) ads7138_struct
{
    uint16_t ain[8];
};

void ads7138_init();
void ads7138_task(void* pvParameters);
void ads7138_write_data(const uint8_t* data, uint32_t length);
void ads7138_read_data(uint8_t register_address, uint8_t* data, uint32_t length);
