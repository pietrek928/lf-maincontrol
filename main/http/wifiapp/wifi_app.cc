#include "wifi_app.h"

/* @brief tag used for ESP serial console messages */
static constexpr char TAG[] = "wifi_app";

esp_err_t wifi_manager_api_handler(httpd_req_t* req)
{
    HTTP_HANDLER_GUARD(
        uint32_t resp_len = 0; constexpr static uint32_t buffor_size = 1024;
        uint8_t data[buffor_size];

        auto post_size = get_post_data(req, data, buffor_size);

        /* List of all access points in JSON format */
        if (!strcmp(req->uri, "/wifi/list_aps")) {
            wifi_manager_list_aps(req);
            return ESP_OK;
        }
        /* Status of the internet connection */
        else if (!strcmp(req->uri, "/wifi/status")) {
            wifi_manager_get_status(req);
            return ESP_OK;
        }
        /* Send comand to setup wifi connection */
        else if (!strcmp(req->uri, "/wifi/send_command")) {
            const wifi_send_command_t& cmd =
                extract_struct<wifi_send_command_t>(data, post_size);
            if (cmd.command >= WIFI_MANAGER_COMMAND_MAX)
            {
                throw HttpException(
                    HTTPD_400_BAD_REQUEST, "Za duży numer komendy");
            }
            wifi_manager_send_command(cmd);
        }
        /* Set creditionals for wifi logging */
        else if (!strcmp(req->uri, "/wifi/set_credentials")) {
            wifi_manager_set_credentials(
                extract_struct<wifi_credentials_t>(data, post_size));
        } else {
            snprintf((char*)data, sizeof(data), "%s not found", req->uri);
            resp_len = strlen((char*)data);
        }

        httpd_resp_set_status(req, http_200_hdr);
        if (resp_len) {
            httpd_resp_set_type(req, http_content_type_json);
        } httpd_resp_set_hdr(req, http_cache_control_hdr, http_cache_control_no_cache);
        httpd_resp_set_hdr(req, http_pragma_hdr, http_pragma_no_cache);
        httpd_resp_send(req, (char*)data, resp_len);)
}

/* Wifi API requests handler for connection with internet */
static constexpr httpd_uri_t http_server_wifi_request = {
    .uri = "/wifi/*",
    .method = HTTP_POST,
    .handler = wifi_manager_api_handler,
    .user_ctx = NULL};

void register_wifi_http_handlers(httpd_handle_t httpd_handle)
{
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(httpd_handle, &http_server_wifi_request));
}
