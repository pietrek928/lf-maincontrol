#pragma once

#include <dirent.h>
#include <esp_http_server.h>
#include <esp_vfs_fat.h>
#include <http_parser.h>
#include <stdio.h>

#include <cstdint>
#include <cstring>

#include "../utils.h"

void register_webpage_sdcard_http_handlers(httpd_handle_t httpd_handle);
