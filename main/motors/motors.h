#pragma once

#include "driver/mcpwm_types.h"

void mcpwm_init();
void mcpwm_start_motor(float duty);
void mcpwm_update_motors(float dutyA, float dutyB);
void mcpwm_stop_motor();