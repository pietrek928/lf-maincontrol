#pragma once

#include <esp_log.h>

#include <cstdint>

#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/spi_types.h"

typedef union
{
    uint8_t value;
    struct
    {
        uint8_t reset_flag : 1, driver_error : 1, sg2 : 1, standstill : 1,
            velocity_reached : 1, position_reached : 1, status_stop_l : 1,
            status_stop_r : 1;
    };
} TMC5160_status_t;

struct __attribute__((packed)) TMC5160_response_t
{
    TMC5160_status_t status;
    uint32_t value;
};

typedef enum
{
    POSITION_MODE = 0,
    VELOCITY_LEFT_MODE = 1,
    VELOCITY_RIGHT_MODE = 2,
    HOLD_MODE = 3,
} RAMP_MODE;

void tmc5160_init();
void tmc5160_init_registers();
void tmc5160_task(void* pvParameters);
TMC5160_response_t tmc5160_send_comand(const uint8_t* data, size_t len);
void tmc5160_enable();
void tmc5160_setStelthChopper();
void TMC5160_clearGStat();
void TMC5160_setCurrent(uint8_t IHOLD, uint8_t IRUN, uint8_t HOLDDELAY);
void TMC5160_setPowerDown(uint8_t TPOWERDOWN);

void tmc5610_setRamp(
    uint8_t RampMode,
    uint32_t VSTART,
    uint32_t A1,
    uint32_t V1,
    uint32_t AMAX,
    uint32_t VMAX,
    uint32_t DMAX,
    uint32_t D1,
    uint32_t VSTOP);

void tmc5160_setPosition(uint32_t position);
void TMC5160_setRampMode(RAMP_MODE mode);
void TMC5160_setAMAX(float AMAX);
void tmc5160_readGStat();
TMC5160_response_t TMC5160_setVelocity(float v);
uint16_t swap_bytes(uint16_t data);
uint32_t swap_bytes(uint32_t data);
