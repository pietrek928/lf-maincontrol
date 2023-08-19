#pragma once

#include <esp_http_server.h>

#include "../../wifi/wifi_manager.h"
#include "../utils.h"

void register_wifi_http_handlers(httpd_handle_t httpd_handle);
