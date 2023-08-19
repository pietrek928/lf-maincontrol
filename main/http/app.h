#pragma once

/*
 * http_app.h
 *
 *  Created on: 3 maj 2021
 *      Author: THINK
 */

#include <esp_err.h>

#include "../hardware/Init.h"
#include "../hardware/hardware_command.h"
#include "../wifi/wifi_manager.h"
#include "esp_http_server.h"
#include "flasher/flasher.h"
#include "readfiles/read_files.h"
#include "utils.h"
#include "wifiapp/wifi_app.h"

/**
 * @brief spawns the http server
 */
void http_app_start();

/**
 * @brief stops the http server
 */
void http_app_stop();
