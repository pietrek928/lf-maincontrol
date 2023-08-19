/*
 * nvs_sync.c
 *
 *  Created on: 3 maj 2021
 *      Author: THINK
 */

#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_err.h>

#include "nvs_sync.h"

static SemaphoreHandle_t nvs_sync_mutex = NULL;

esp_err_t nvs_sync_create()
{
    if(nvs_sync_mutex == NULL){

        nvs_sync_mutex = xSemaphoreCreateMutex();

		if(nvs_sync_mutex){
			return ESP_OK;
		}
		else{
			return ESP_FAIL;
		}
    }
	else{
		return ESP_OK;
	}
}

void nvs_sync_free()
{
    if(nvs_sync_mutex != NULL){
        vSemaphoreDelete( nvs_sync_mutex );
        nvs_sync_mutex = NULL;
    }
}

bool nvs_sync_lock(TickType_t xTicksToWait)
{
	if(nvs_sync_mutex){
		if( xSemaphoreTake( nvs_sync_mutex, xTicksToWait ) == pdTRUE ) {
			return true;
		}
		else{
			return false;
		}
	}
	else{
		return false;
	}
}

void nvs_sync_unlock()
{
	xSemaphoreGive( nvs_sync_mutex );
}
