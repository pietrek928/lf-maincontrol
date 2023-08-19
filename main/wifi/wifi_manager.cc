/*
 * Wifi_manage.c
 *
 *  Created on: 3 maj 2021
 *      Author: THINK
 */

#include "wifi_manager.h"

#include "../http/app.h"
#include "../http/json.h"
#include "dns_server.h"
#include "ethernet.h"
#include "nvs_sync.h"

/* objects used to manipulate the main queue of events */
QueueHandle_t wifi_manager_queue;

/* @brief software timer to wait between each connection retry.
 * There is no point hogging a hardware timer for a functionality like this
 * which only needs to be 'accurate enough' */
TimerHandle_t wifi_manager_retry_timer = NULL;

/* @brief software timer that will trigger shutdown of the AP after a succesful
 * STA connection
 * There is no point hogging a hardware timer for a functionality like this
 * which only needs to be 'accurate enough' */
TimerHandle_t wifi_manager_shutdown_ap_timer = NULL;

SemaphoreHandle_t wifi_manager_json_mutex = NULL;
SemaphoreHandle_t wifi_manager_sta_ip_mutex = NULL;
char* wifi_manager_sta_ip = NULL;
uint16_t ap_num = MAX_AP_NUM;
wifi_ap_record_t* accessp_records;
char* accessp_json = NULL;
char* ip_info_json = NULL;
wifi_config_t* wifi_manager_config_sta = NULL;

/* @brief tag used for ESP serial console messages */
static const char TAG[] = "ESP_wifi_Manager";

const char wifi_manager_nvs_namespace[] = "Esp32_Wifi_Manager";

/* @brief task handle for the main wifi_manager task */
static TaskHandle_t task_wifi_manager = NULL;

/* @brief netif object for the STATION */
static esp_netif_t* esp_netif_sta = NULL;

/* @brief netif object for the ACCESS POINT */
static esp_netif_t* esp_netif_ap = NULL;

/* class declaration before function */
class ESPWifiManager;

typedef enum
{
    STATUS_AP_ON,
    STATUS_DHCP_ON,
    STATUS_SCAN_ACTIVE,
    STATUS_GOT_IP,
    STATUS_ETHERNET_STARTED,
} wifi_manager_status_t;

class ESPWifiManager
{
    QueueHandle_t command_queue;
    TimerHandle_t command_timer;
    wifi_manager_command_t delayed_command = WIFI_MANAGER_COMMAND_MAX;
    esp_netif_ip_info_t last_sta_ip_info;

    wifi_settings_t wifi_settings = {
        .ap =
            {
                .ssid = CONFIG_DEFAULT_AP_SSID,
                .password = CONFIG_DEFAULT_AP_PASSWORD,
                .ssid_len = sizeof(CONFIG_DEFAULT_AP_SSID) - 1,
                .channel = CONFIG_DEFAULT_AP_CHANNEL,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK,
                .ssid_hidden = CONFIG_DEFAULT_AP_SSID_HIDDEN,
                .max_connection = CONFIG_DEFAULT_AP_MAX_CONNECTIONS,
                .beacon_interval = CONFIG_DEFAULT_AP_BEACON_INTERVAL,
                .pairwise_cipher = WIFI_CIPHER_TYPE_UNKNOWN,
                .ftm_responder = false,
                .pmf_cfg = {.capable = true, .required = false},
            },
        .sta =
            {
                .ssid = CONFIG_DEFAULT_STA_SSID,
                .password = CONFIG_DEFAULT_STA_PASSWORD,
                .scan_method = WIFI_FAST_SCAN,
                .bssid_set = false,
                .bssid = {0},
                .channel = CONFIG_DEFAULT_STA_CHANNEL,
                .listen_interval = 0,
                .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                .threshold = {.rssi = 0, .authmode = WIFI_AUTH_OPEN},
                .pmf_cfg = {.capable = true, .required = false},
                .rm_enabled = 0,
                .btm_enabled = 0,
                .mbo_enabled = 0,
                .ft_enabled = 0,
                .owe_enabled = 0,
                .transition_disable = 0,
                .reserved = 0,
                .sae_pwe_h2e = WPA3_SAE_PWE_UNSPECIFIED,
                .failure_retry_cnt = 0,
            },
        .bandwith = WIFI_BW_HT20,
        .power_save = WIFI_PS_NONE,  // Lower Power Consumption
    };

    uint32_t status = 0;
    uint16_t ap_num = 0;
    uint8_t last_sta_disconnect_reason = 0;
    wifi_ap_record_t ap_records[MAX_AP_NUM];

   public:
    inline void set_status(wifi_manager_status_t s)
    {
        // set status bit
        status |= (((uint32_t)1) << s);
    }

    inline void reset_status(wifi_manager_status_t s)
    {
        // reset status bit
        status &= ~(((uint32_t)1) << s);
    }

    inline bool get_status(wifi_manager_status_t s)
    {
        // read status bit
        return (bool)(status & (((uint32_t)1) << s));
    }

    /**
     * @brief Start scaning the network
     */
    void scan_start()
    {
        wifi_scan_config_t scan_config = {
            .ssid = 0,
            .bssid = 0,
            .channel = 0,
            .show_hidden = true,
            .scan_type = WIFI_SCAN_TYPE_ACTIVE,
            .scan_time = {.active = {.min = 128, .max = 256}, .passive = 0}};
        ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, false));

        set_status(STATUS_SCAN_ACTIVE);
    }

    /**
     * @brief Get AP list found in last scan
     */
    void scan_fetch_aps()
    {
        ap_num = MAX_AP_NUM;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));
        reset_status(STATUS_SCAN_ACTIVE);
    }

    /**
     * @brief Stop scaning the network
     */
    void scan_stop()
    {
        esp_wifi_scan_stop();
        reset_status(STATUS_SCAN_ACTIVE);
    }

    /**
     * @brief List in JSON of the APs found during the scan
     */
    void get_aps_json(void* http_handle)
    {
        JSON_TO_HTTP(
            http_handle, JSON_LIST(for (int i = 0; i < ap_num; i++) {
                auto& ap = ap_records[i];
                JSON_SUBELEM(JSON_DICT(JSON_KEY(ssid, (const char*)ap.ssid)
                                           JSON_KEY(rssi, ap.rssi)
                                               JSON_KEY(authmode, ap.authmode)))
            }));
    }

    /**
     * @brief Get the status json object
     */
    void get_status_json(void* http_handle)
    {
        JSON_TO_HTTP(
            http_handle,
            JSON_DICT(
                JSON_SUBKEY(
                    status,
                    JSON_LIST(
                        if (get_status(STATUS_AP_ON)) {
                            JSON_ELEM("Access Point enabled")
                        } if (get_status(STATUS_DHCP_ON)) {
                            JSON_ELEM("DHCP enabled")
                        } if (get_status(STATUS_SCAN_ACTIVE)) {
                            JSON_ELEM("WIFI scan is running")
                        }
                        if (get_status(STATUS_GOT_IP)) {
                            JSON_ELEM("WIFI scan is running")
                        }
                        if (get_status(STATUS_ETHERNET_STARTED)) {
                            JSON_ELEM("Ethernet started")
                        }
                    ))

                    if (get_status(STATUS_ETHERNET_STARTED)) {
                        JSON_SUBKEY(
                            eth_info,
                            JSON_DICT(
                                esp_netif_ip_info_t ip_info = ethernet_ip_info();
                                JSON_KEY(ip, *(ipv4_t*)&ip_info.ip)
                                JSON_KEY(gw, *(ipv4_t*)&ip_info.gw)
                                JSON_KEY(netmask, *(ipv4_t*)&ip_info.netmask)
                            )
                        )
                    }

                    JSON_SUBKEY(
                        ip_info_ap,
                        JSON_DICT(
                            esp_netif_ip_info_t ip_info; ESP_ERROR_CHECK(
                                esp_netif_get_ip_info(esp_netif_ap, &ip_info));
                            JSON_KEY(ip, *(ipv4_t*)&ip_info.ip)
                                JSON_KEY(gw, *(ipv4_t*)&ip_info.gw) JSON_KEY(
                                    netmask, *(ipv4_t*)&ip_info.netmask)))

                        JSON_SUBKEY(
                            ip_info_sta,
                            JSON_DICT(esp_netif_ip_info_t ip_info;
                                      ESP_ERROR_CHECK(esp_netif_get_ip_info(
                                          esp_netif_sta, &ip_info));
                                      JSON_KEY(ip, *(ipv4_t*)&ip_info.ip)
                                          JSON_KEY(gw, *(ipv4_t*)&ip_info.gw)
                                              JSON_KEY(
                                                  netmask,
                                                  *(ipv4_t*)&ip_info.netmask)))
                            JSON_SUBKEY(
                                last_sta_ip_info,
                                JSON_DICT(
                                    esp_netif_ip_info_t& ip_info =
                                        last_sta_ip_info;

                                    JSON_KEY(ip, *(ipv4_t*)&ip_info.ip)
                                        JSON_KEY(gw, *(ipv4_t*)&ip_info.gw)
                                            JSON_KEY(
                                                netmask,
                                                *(ipv4_t*)&ip_info.netmask)))

                                JSON_KEY(
                                    last_sta_disconnect_reason,
                                    last_sta_disconnect_reason)));
    }

    void dhcp_server_stop()
    {
        // stop DHCP server
        esp_netif_dhcps_stop(esp_netif_ap);
        reset_status(STATUS_DHCP_ON);
    }

    void dhcp_server_start()
    {
        // start DHCP server
        ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap));
        set_status(STATUS_DHCP_ON);
    }

    void dhcp_client_stop()
    {
        // stop DHCP server
        esp_netif_dhcpc_stop(esp_netif_sta);
        reset_status(STATUS_DHCP_ON);
    }

    void dhcp_client_start()
    {
        // start DHCP server
        ESP_ERROR_CHECK(esp_netif_dhcpc_start(esp_netif_sta));
        set_status(STATUS_DHCP_ON);
    }

    void set_esp_server_ip()
    {
        // configure ESP interface ip, netmask and gateway
        // those settings are required to operate in the network
        esp_netif_ip_info_t ap_ip_info;
        memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));
        inet_pton(AF_INET, CONFIG_DEFAULT_AP_IP, &ap_ip_info.ip);
        inet_pton(AF_INET, CONFIG_DEFAULT_AP_GATEWAY, &ap_ip_info.gw);
        inet_pton(AF_INET, CONFIG_DEFAULT_AP_NETMASK, &ap_ip_info.netmask);
        ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info));

        set_status(STATUS_GOT_IP);
    }

    void clear_esp_ip()
    {
        // ESP IP to 0.0.0.0
        esp_netif_ip_info_t ap_ip_info;
        memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));
        ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info));

        reset_status(STATUS_GOT_IP);
    }

    void start_wifi_ap()
    {
        // configure ESP in access point mode
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

        ESP_ERROR_CHECK(esp_wifi_set_config(
            WIFI_IF_AP,
            (wifi_config_t*)&wifi_settings
                .ap));  // trzeba konwertować bo jest unia
        ESP_ERROR_CHECK(
            esp_wifi_set_bandwidth(WIFI_IF_AP, wifi_settings.bandwith));
        ESP_ERROR_CHECK(esp_wifi_set_ps(wifi_settings.power_save));

        ESP_ERROR_CHECK(esp_wifi_start());

        set_esp_server_ip();
        dhcp_server_start();
        http_app_start();
    }

    void connect_STA()
    {
        // Connect ESP to network in STA mode
        ESP_LOGI(TAG, "Wifi connect");

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(
            WIFI_IF_STA, (wifi_config_t*)&wifi_settings.sta));
        ESP_LOGI(
            TAG,
            "Setting WiFi configuration SSID %s...",
            wifi_settings.sta.ssid);
        ESP_LOGI(
            TAG,
            "Setting WiFi configuration PWD %s...",
            wifi_settings.sta.password);

        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_ERROR_CHECK(esp_wifi_connect());

        // Start DHCP
        dhcp_client_start();
        http_app_start();
    }

    void connect_ethernet()
    {
        ESP_LOGI(TAG, "Ethernet connect");
        ethernet_init();
        ethernet_start();

        dhcp_client_start();
        http_app_start();
    }

    /**
     * @brief Set Login and Password for Wifi connection with ESP
     *
     * @param creds
     */
    void set_wifi_credentials(const wifi_credentials_t& creds)
    {
        COPY_STRING_FIELD(wifi_settings.sta.ssid, creds.ssid);
        COPY_STRING_FIELD(wifi_settings.sta.password, creds.pwd);
        ESP_LOGI(
            TAG,
            "Sssid: %s, Password: %s, Length_pwd: %zu",
            wifi_settings.sta.ssid,
            wifi_settings.sta.password,
            strlen((const char*)wifi_settings.sta.password));
    }

    void disconnect()
    {
        // disconnect from network, disable wifi
        http_app_stop();

        /* stop all DHCP */
        dhcp_client_stop();
        dhcp_server_stop();
        scan_stop();

        auto disconnect_result = esp_wifi_disconnect();
        if (disconnect_result != ESP_ERR_WIFI_NOT_INIT &&
            disconnect_result != ESP_ERR_WIFI_NOT_STARTED) {
            ESP_ERROR_CHECK(disconnect_result);
        }

        if (CONFIG_WIFI_MANAGER_ENABLE_ETHERNET
            && get_status(STATUS_ETHERNET_STARTED)) {
            ethernet_stop();
        }

        auto stop_result = esp_wifi_stop();
        if (stop_result != ESP_ERR_WIFI_NOT_INIT) {
            ESP_ERROR_CHECK(stop_result);
        }

        status = 0;

        // Odczekanie na dinit
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    BaseType_t send_command(wifi_manager_command_t cmd)
    {
        // put command on queue, to process by the main loop
        return xQueueSend(command_queue, &cmd, portMAX_DELAY);
    }

    void send_delayd_command()
    {
        // Sending command is delayed
        if (delayed_command < WIFI_MANAGER_COMMAND_MAX)
        {
            send_command(delayed_command);
            delayed_command = WIFI_MANAGER_COMMAND_MAX;
        }
    }

    static void command_timer_cb(TimerHandle_t xTimer)
    {
        // Callback function for timer
        xTimerStop(xTimer, (TickType_t)0);
        ESPWifiManager* self = (ESPWifiManager*)pvTimerGetTimerID(xTimer);
        self->send_delayd_command();
    }

    // TODO: czym się różni od send_delayed_command
    void send_command_delayed(wifi_manager_command_t cmd, uint32_t delay_ms)
    {
        if (xTimerIsTimerActive(command_timer)) return;
        delayed_command = cmd;
        xTimerChangePeriod(
            command_timer, pdMS_TO_TICKS(delay_ms) + 1, (TickType_t)0);
    }

    /**
     * @brief Main WiFI event handler with all current states
     *
     * @param _self
     * @param event_base
     * @param event_id
     * @param event_data
     */
    static void wifi_event_handler(
        void* _self,
        esp_event_base_t event_base,
        int32_t event_id,
        void* event_data)
    {
        ESPWifiManager* self = (ESPWifiManager*)_self;
        switch ((wifi_event_t)event_id)
        {
            case WIFI_EVENT_WIFI_READY:
                ESP_LOGI(TAG, "WIFI_EVENT_WIFI_READY");
                break;
            case WIFI_EVENT_SCAN_DONE:
                ESP_LOGI(TAG, "WIFI_EVENT_SCAN_DONE");
                self->send_command(WIFI_MANAGER_SCAN_FETCH);
                break;
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
                break;
            case WIFI_EVENT_STA_STOP:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_STOP");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
            {
                wifi_event_sta_disconnected_t* event_log =
                    (wifi_event_sta_disconnected_t*)event_data;
                self->last_sta_disconnect_reason = event_log->reason;
                ESP_LOGI(
                    TAG,
                    "MESSAGE: EVENT_STA_DISCONNECTED with Reason code: %d",
                    event_log->reason);
                ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
            }
            break;
            case WIFI_EVENT_STA_AUTHMODE_CHANGE:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_AUTHMODE_CHANGE");
                break;
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_START");
                break;
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_STOP");
                break;
            case WIFI_EVENT_STA_WPS_ER_SUCCESS:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_SUCCESS");
                break;
            case WIFI_EVENT_STA_WPS_ER_FAILED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_FAILED");
                break;
            case WIFI_EVENT_STA_WPS_ER_TIMEOUT:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_TIMEOUT");
                break;
            case WIFI_EVENT_STA_WPS_ER_PIN:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_PIN");
                break;
            case WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP");
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:

                ESP_LOGI(TAG, "WIFI_EVENT_AP_STADISCONNECTED");
                if (CONFIG_WIFI_MANAGER_ENABLE_ETHERNET)
                {
                    self->send_command(WIFI_MANAGER_ETHERNET_CONNECT);
                }
                break;
            case WIFI_EVENT_AP_PROBEREQRECVED:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_PROBEREQRECVED");
                break;
            case WIFI_EVENT_MAX:
                ESP_LOGI(TAG, "WIFI_EVENT_MAX");
                break;
            case WIFI_EVENT_FTM_REPORT:
                ESP_LOGI(TAG, "WIFI_EVENT_FTM_REPORT");
                break;
            case WIFI_EVENT_STA_BSS_RSSI_LOW:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_BSS_RSSI_LOW");
                break;
            case WIFI_EVENT_ACTION_TX_STATUS:
                ESP_LOGI(TAG, "WIFI_EVENT_ACTION_TX_STATUS");
                break;
            case WIFI_EVENT_ROC_DONE:
                ESP_LOGI(TAG, "WIFI_EVENT_ROC_DONE");
                break;
            case WIFI_EVENT_STA_BEACON_TIMEOUT:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_BEACON_TIMEOUT");
                break;
            case WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START:
                ESP_LOGI(
                    TAG,
                    "WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START");
                break;
            case WIFI_EVENT_AP_WPS_RG_SUCCESS:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_WPS_RG_SUCCESS");
                break;
            case WIFI_EVENT_AP_WPS_RG_FAILED:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_WPS_RG_FAILED");
                break;
            case WIFI_EVENT_AP_WPS_RG_TIMEOUT:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_WPS_RG_TIMEOUT");
                break;
            case WIFI_EVENT_AP_WPS_RG_PIN:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_WPS_RG_PIN");
                break;
            case WIFI_EVENT_AP_WPS_RG_PBC_OVERLAP:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_WPS_RG_PBC_OVERLAP");
                break;
            default:
                ESP_LOGI(TAG, "WIFI_EVENT %ld", event_id);
                break;
        }
    }

    /**
     * @brief Main IP handler status
     *
     * @param _self
     * @param event_base
     * @param event_id
     * @param event_data
     */
    static void ip_event_handler(
        void* _self,
        esp_event_base_t event_base,
        int32_t event_id,
        void* event_data)
    {
        ESPWifiManager* self = (ESPWifiManager*)_self;
        ESP_LOGI(TAG, "IP_EVENT");

        switch ((ip_event_t)event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
                ESP_ERROR_CHECK(esp_netif_get_ip_info(
                    esp_netif_sta, &self->last_sta_ip_info));
                self->send_delayd_command();
                break;
            case IP_EVENT_STA_LOST_IP:
                ESP_LOGI(TAG, "IP_EVENT_STA_LOST_IP");
                break;
            case IP_EVENT_AP_STAIPASSIGNED:
                ESP_LOGI(TAG, "IP_EVENT_AP_STAIPASSIGNED");
                break;
            case IP_EVENT_GOT_IP6:
                ESP_LOGI(TAG, "IP_EVENT_GOT_IP6");
                break;
            case IP_EVENT_ETH_GOT_IP:
                ESP_LOGI(TAG, "IP_EVENT_ETH_GOT_IP");
                break;
            case IP_EVENT_PPP_GOT_IP:
                ESP_LOGI(TAG, "IP_EVENT_PPP_GOT_IP");
                break;
            case IP_EVENT_PPP_LOST_IP:
                ESP_LOGI(TAG, "IP_EVENT_PPP_LOST_IP");
                break;
            case IP_EVENT_ETH_LOST_IP:
                ESP_LOGI(TAG, "IP_EVENT_ETH_LOST_IP");
                break;
        }
    }

    void network_init()
    {
        // has to be called only once during esp start
        ESP_ERROR_CHECK(esp_netif_init());  // initialize the tcp stack
        ESP_ERROR_CHECK(
            esp_event_loop_create_default());  // event loop for the wifi driver
    }

    /**
     * @brief Initialisation of APSTA WiFi
     */
    void init()
    {
        network_init();

        ESP_LOGI(TAG, "Initilising TCP and wifi\n");

        if (CONFIG_WIFI_MANAGER_ENABLE_ETHERNET)
        {
            ethernet_init();
        }

        esp_netif_sta = esp_netif_create_default_wifi_sta();
        esp_netif_ap = esp_netif_create_default_wifi_ap();

        /* default wifi config */
        wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));

        /* event handler for the connection */
        esp_event_handler_instance_t instance_ip_event;
        esp_event_handler_instance_t instance_wifi_event;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT,
            ESP_EVENT_ANY_ID,
            &ip_event_handler,
            this,
            &instance_ip_event));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            this,
            &instance_wifi_event));
    }

    /**
     * @brief Main state machine of the WiFi connection
     */
    void run_loop()
    {
        // Main loop, processes wifi manager commands
        while (1)
        {
            wifi_manager_command_t cmd;
            auto xStatus = xQueueReceive(command_queue, &cmd, portMAX_DELAY);
            if (xStatus != pdPASS)
            {
                /* No command received */
                continue;
            }

            switch (cmd)
            {
                case WIFI_MANAGER_SCAN_START:
                    scan_start();
                    break;
                case WIFI_MANAGER_SCAN_FETCH:
                    scan_fetch_aps();
                    break;
                case WIFI_MANAGER_SCAN_STOP:
                    scan_stop();
                    break;
                case WIFI_MANAGER_DHCP_STOP:
                    dhcp_server_stop();
                    break;
                case WIFI_MANAGER_DHCP_START:
                    dhcp_server_start();
                    break;
                case WIFI_MANAGER_AP_START:
                    disconnect();
                    start_wifi_ap();
                    break;
                case WIFI_MANAGER_CONNECT:
                    disconnect();
                    connect_STA();
                    break;
                case WIFI_MANAGER_DISCONNECT:
                    disconnect();
                    break;
                case WIFI_MANAGER_SAVE_CONFIG:
                    config_to_flash();
                    break;
                case WIFI_MANAGER_LOAD_CONFIG:
                    config_from_flash();
                    break;
                case WIFI_MANAGER_ETHERNET_STOP:
                    if (CONFIG_WIFI_MANAGER_ENABLE_ETHERNET
                        && get_status(STATUS_ETHERNET_STARTED)) {
                        ethernet_stop();
                    }
                    break;
                case WIFI_MANAGER_ETHERNET_CONNECT:
                    if (CONFIG_WIFI_MANAGER_ENABLE_ETHERNET)
                    {
                        disconnect();
                        connect_ethernet();
                        set_status(STATUS_ETHERNET_STARTED);
                    }
                    break;
                case WIFI_MANAGER_COMMAND_MAX:
                    break;
            }
        }
    }

    bool config_from_flash()
    {
        // load stored wifi settings from flash
        nvs_handle handle;
        esp_err_t esp_err;
        if (nvs_sync_lock(portMAX_DELAY))
        {
            esp_err =
                nvs_open(wifi_manager_nvs_namespace, NVS_READONLY, &handle);

            if (esp_err != ESP_OK)
            {
                nvs_sync_unlock();
                return false;
            }

            /* load settings from flash */
            size_t size = 0;
            esp_err =
                nvs_get_blob(handle, "wifi_settings", &wifi_settings, &size);
            if (esp_err != ESP_OK)
            {
                nvs_sync_unlock();
                return false;
            }
            if (size)
            {
                ESP_LOGI(TAG, "Loaded %zu config bytes from flash", size);
            }

            nvs_close(handle);
            nvs_sync_unlock();

            return (bool)size;
        }
        else
        {
            ESP_LOGE(TAG, "config_from_flash failed to acquire nvs_sync mutex");
        }
        return false;
    }

    esp_err_t config_to_flash()
    {
        // store wifi settings in flash to use them later
        nvs_handle handle;
        esp_err_t esp_err = ESP_OK;

        ESP_LOGI(TAG, "About to save config to flash!!");

        if (nvs_sync_lock(portMAX_DELAY))
        {
            esp_err =
                nvs_open(wifi_manager_nvs_namespace, NVS_READWRITE, &handle);
            // TODO: if we open maybe close is alse neccessary on error?
            if (esp_err != ESP_OK)
            {  // TODO: use exceptions, maybe nvs class?
                nvs_sync_unlock();
                return esp_err;
            }

            auto sz = sizeof(wifi_settings);

            // TODO: check if changed ?
            // esp_err = nvs_get_blob(handle, "wifi_settings",
            // tmp_conf.sta.ssid, &sz);

            /* save config in flash */
            esp_err = nvs_set_blob(handle, "wifi_settings", &wifi_settings, sz);
            if (esp_err != ESP_OK)
            {
                nvs_sync_unlock();
                return esp_err;
            }

            // TODO: commit only if change
            nvs_commit(handle);
            ESP_LOGI(TAG, "wifi_manager wrote settings");

            if (esp_err != ESP_OK) return esp_err;

            nvs_close(handle);
            nvs_sync_unlock();
        }
        else
        {
            ESP_LOGE(TAG, "config_to_flash failed to acquire nvs_sync mutex");
        }
        return esp_err;
    }

    /**
     * @brief Construct a new ESPWifiManager object
     *
     */
    ESPWifiManager()
    {
        // create comamnd queue
        command_queue = xQueueCreate(3, sizeof(wifi_manager_command_t));
        /* create timer for command delay */
        command_timer =
            xTimerCreate(NULL, 1, pdFALSE, (void*)this, command_timer_cb);
    }

    /**
     * @brief Destroy the ESPWifiManager object
     *
     */
    ~ESPWifiManager()
    {
        vQueueDelete(command_queue);
        xTimerDelete(command_timer, 0);
    }

} wifi_manager;

/**
 * @brief Initialization and creation of WiFi
 *
 */
void wifi_manager_start()
{
    /* disable the default wifi logging */
    esp_log_level_set("wifi", ESP_LOG_NONE);

    /* initialize flash memory */
    nvs_flash_init();
    ESP_ERROR_CHECK(nvs_sync_create()); /* semaphore for thread synchronization
                                           on NVS memory */

    /* start wifi manager task */
    ESP_LOGI(TAG, "Wifi task start!");
    xTaskCreate(
        &wifi_manager_task,
        "wifi_manager",
        4096,
        NULL,
        CONFIG_WIFI_MANAGER_TASK_PRIORITY,
        &task_wifi_manager);
}

/**
 * @brief Main WiFi task
 *
 * @param pvParameters
 */
void wifi_manager_task(void* pvParameters)
{
    wifi_manager.init();
    wifi_manager.disconnect();
    wifi_manager.start_wifi_ap();
    // wifi_manager.start_wifi_sta();
    wifi_manager.send_command(WIFI_MANAGER_SCAN_START);

    /* run the main loop */
    wifi_manager.run_loop();

    vTaskDelete(NULL);
}

/**
 * @brief API for http_application
 */
void wifi_manager_list_aps(void* http_handle)
{
    // list access points in json
    wifi_manager.get_aps_json(http_handle);
}

void wifi_manager_get_status(void* http_handle)
{
    // show esp status
    wifi_manager.get_status_json(http_handle);
}

void wifi_manager_send_command(const wifi_send_command_t& cmd)
{
    if (!cmd.delay_ms)
    {
        wifi_manager.send_command(cmd.command);
    }
    else
    {
        wifi_manager.send_command_delayed(cmd.command, cmd.delay_ms);
    }
}

void wifi_manager_set_credentials(const wifi_credentials_t& creds)
{
    // set wifi login and password
    return wifi_manager.set_wifi_credentials(creds);
}
