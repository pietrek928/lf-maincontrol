/*
 * Wifi_manage.h
 *
 *  Created on: 3 maj 2021
 *      Author: THINK
 */

#pragma once

#include <driver/gpio.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <lwip/api.h>
#include <lwip/err.h>
#include <lwip/ip4_addr.h>
#include <lwip/netdb.h>
#include <mdns.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdint>
#include <cstring>
#include <algorithm>

/**
 * @brief Defines the maximum size of a SSID name. 32 is IEEE standard.
 * @warning limit is also hard coded in wifi_config_t. Never extend this value.
 */
#define MAX_SSID_SIZE 32

/**
 * @brief Defines the maximum size of a WPA2 passkey. 64 is IEEE standard.
 * @warning limit is also hard coded in wifi_config_t. Never extend this value.
 */
#define MAX_PASSWORD_SIZE 64

/**
 * @brief Defines the maximum number of access points that can be scanned.
 *
 * To save memory and avoid nasty out of memory errors,
 * we can limit the number of APs detected in a wifi scan.
 */
#define MAX_AP_NUM 15

/**
 * @brief Defines the complete list of all messages that the wifi_manager can
 * process.
 *
 * Some of these message are events ("EVENT"), and some of them are action
 * ("ORDER") Each of these messages can trigger a callback function and each
 * callback function is stored in a function pointer array for convenience.
 * Because of this behavior, it is extremely important to maintain a strict
 * sequence and the top level special element 'MESSAGE_CODE_COUNT'
 *
 * @see wifi_manager_set_callback
 */
typedef enum message_code_t {
    NONE = 0,
    WM_ORDER_START_HTTP_SERVER = 1,
    WM_ORDER_STOP_HTTP_SERVER = 2,
    WM_ORDER_START_DNS_SERVICE = 3,
    WM_ORDER_STOP_DNS_SERVICE = 4,
    WM_ORDER_START_WIFI_SCAN = 5,
    WM_ORDER_LOAD_AND_RESTORE_STA = 6,
    WM_ORDER_CONNECT_STA = 7,
    WM_ORDER_DISCONNECT_STA = 8,
    WM_ORDER_START_AP = 9,
    WM_EVENT_STA_DISCONNECTED = 10,
    WM_EVENT_SCAN_DONE = 11,
    WM_EVENT_STA_GOT_IP = 12,
    WM_ORDER_STOP_AP = 13,
    WM_MESSAGE_CODE_COUNT = 14 /* important for the callback array */

} message_code_t;

typedef enum {
    WIFI_MANAGER_SCAN_START  = 0,
    WIFI_MANAGER_SCAN_FETCH  = 1,
    WIFI_MANAGER_SCAN_STOP   = 2,
    WIFI_MANAGER_DHCP_STOP   = 3,
    WIFI_MANAGER_DHCP_START  = 4,
    WIFI_MANAGER_AP_START    = 5,
    WIFI_MANAGER_CONNECT     = 6,
    WIFI_MANAGER_DISCONNECT  = 7,
    WIFI_MANAGER_SAVE_CONFIG = 8,
    WIFI_MANAGER_LOAD_CONFIG = 9,
    WIFI_MANAGER_ETHERNET_STOP = 10,
    WIFI_MANAGER_ETHERNET_CONNECT = 11,
    WIFI_MANAGER_COMMAND_MAX,
} wifi_manager_command_t;

/**
 * The actual WiFi settings in use
 */
struct wifi_settings_t {
    wifi_ap_config_t  ap;  /**< configuration of AP */
    wifi_sta_config_t sta; /**< configuration of STA */
    wifi_bandwidth_t bandwith;
    wifi_ps_type_t power_save;
};

class wifi_credentials_t {
    public:
        char ssid[MAX_SSID_SIZE];
        char pwd[MAX_PASSWORD_SIZE];
};

class wifi_send_command_t {
   public:
    wifi_manager_command_t command;
    /* delay executing command by esp by some ms */
    uint32_t delay_ms;
};

/**
 * @brief Structure used to store one message in the queue.
 */
typedef struct {
    message_code_t code;
    void* param;
} queue_message;

/**
 * @brief Copy string field end
 * ensure it's zero-terminated - SECURITY!
 */
#define COPY_STRING_FIELD(dst, src) { \
    std::memcpy(dst, src, std::min(sizeof(dst), sizeof(src))); \
    dst[sizeof(dst)-1] = 0;\
}

/**
 * @brief returns the current esp_netif object for the STAtion
 */
esp_netif_t* wifi_manager_get_esp_netif_sta();

void wifi_manager_start();
void wifi_manager_task(void* pvParameters);

void wifi_manager_list_aps(void* http_handle);
void wifi_manager_get_status(void* http_handle);
void wifi_manager_set_credentials(const wifi_credentials_t& creds);
void wifi_manager_send_command(const wifi_send_command_t& cmd);
