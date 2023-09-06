#pragma once

#include <cstdint>

void as5055_init();
void as5055_clear_error();
void as5055_soft_reset();
void as5055_test_task(void* pvParameters);
void as5055_test_task_AGC(void* pvParameters);

uint16_t as5055_write_read_simultaneously();
void as5055_write_16bi(uint8_t data[]);
uint16_t as5055_read_angle();
uint16_t as5055_read_angle_data();
bool as5055_validate_response(uint16_t data);
float as5055_convert_angle(uint16_t data);

uint16_t spiCalcEvenParity(uint16_t value);

void as5055_write_command(unsigned int reg);
unsigned int as5055_read(uint16_t);

uint16_t as5055_send(uint16_t buf);

unsigned int as5055_send_and_read(unsigned int buf);
void as5055_send_16bit(uint16_t buf);
