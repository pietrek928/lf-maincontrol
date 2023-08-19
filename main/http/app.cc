/*
 * http_app.c
 *
 *  Created on: 3 maj 2021
 *      Author: THINK
 */

#include "app.h"

/* Select sd handlers depending on config in sdkconfig CONFIG_WIFI_MANAGER_SERVE_SD_CARD=1 */
#if CONFIG_WIFI_MANAGER_SERVE_SD_CARD
#include "webpage/sdcard.h"
#include "webpage/sdcard.cc"
constexpr auto register_webpage_handlers = register_webpage_sdcard_http_handlers;
#else /* CONFIG_WIFI_MANAGER_SERVE_SD_CARD = 0 */
#include "webpage/flash.h"
#include "webpage/flash.cc"
constexpr auto register_webpage_handlers = register_webpage_flash_http_handlers;
#endif /* WIFI_MANAGER_SERVE_SD_CARD */

/* @brief tag used for ESP serial console messages */
static const char TAG[] = "http_server";

/* @brief the HTTP server handle */
static httpd_handle_t httpd_handle = NULL;

/* HW API requests handler */
static const httpd_uri_t http_server_hw_api_request = {
    .uri = "/hw_api", .method = HTTP_POST,
    .handler = hw_command_api_handler,
    .user_ctx = NULL
};

void http_app_stop()
{
    if (httpd_handle != NULL)
    {
        /* stop server */
        httpd_stop(httpd_handle);
        httpd_handle = NULL;
    }
}

void http_app_start()
{
    esp_err_t err;

    if (httpd_handle == NULL)
    {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.max_uri_handlers = 20; // Zwiększam ilość możliwych endopointów

        /* this is an important option that isn't set up by default.
         * We could register all URLs one by one, but this would not work while
         * the fake DNS is active */
        config.uri_match_fn = httpd_uri_match_wildcard;

        err = httpd_start(&httpd_handle, &config);

        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Registering URI handlers");
            register_wifi_http_handlers(httpd_handle);
            register_read_files_http_handlers(httpd_handle);
            register_webpage_handlers(httpd_handle);
            register_flasher_http_handlers(httpd_handle);
            ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_handle, &http_server_hw_api_request));
        }
    }
}
