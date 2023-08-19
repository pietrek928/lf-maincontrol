#pragma once

#include <dirent.h>
#include <esp_http_server.h>
#include <http_parser.h>
#include <stdio.h>

#include <cstdint>

#include "../json.h"
#include "../utils.h"

void register_read_files_http_handlers(httpd_handle_t httpd_handle);
