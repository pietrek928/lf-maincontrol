#pragma once

#include <esp_log.h>

#include <cstdint>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/ledc_types.h"
#include "math.h"

void esc_init();
void esc_task(void* pvParameters);
void escDuty(float duty);
void escSpeed(float speed);