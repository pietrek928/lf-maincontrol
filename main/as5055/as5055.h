#pragma once

#include <cstdint>
#include "driver/spi_master.h"

void as5055_init();
void as5055_task(void* pvParameters);
void as5055_task_AGC(void* pvParameters);

uint16_t as5055_write_read_simultaneously(spi_device_handle_t spi_handler);
void as5055_write_16bi(spi_device_handle_t spi_handler, uint8_t data[]);
uint16_t swap_bytes(uint16_t data);
uint16_t as5055_read_angle();

uint16_t spiCalcEvenParity(uint16_t value);

void as5055_write_command(unsigned int reg);
unsigned int as5055_read(uint16_t);

uint16_t as5055_send(spi_device_handle_t spi_handler, uint16_t buf);

unsigned int as5055_send_and_read(spi_device_handle_t spi_handler, unsigned int buf);
void as5055_send_16bit(spi_device_handle_t spi_handler, uint16_t buf);
