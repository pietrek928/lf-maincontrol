#pragma once

#include <driver/sdmmc_host.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <cstdint>

#include "binary_logging.h"
#include "hal/gpio_types.h"

#define SD_MOUNT "/sdcard"
#define MAX_RECORD_SIZE 1024

void sd_card_init();
void save_data(const char* fname, const char* data);
void save_logs(const char* fname, void* data);
void read_data_to_logs(const char* fname);
void SDcard_task(void* pvParameters);
