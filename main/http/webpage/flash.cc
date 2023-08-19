#include "flash.h"
#include "../utils.h"

#include <esp_err.h>

/* wifi manager webpage sattic files built into flash */
#include "web.h"

const char* TAG_flash = "Webpage Flash server";

/**
 * GET requests a representation of the specified resource. Note that GET should
 * not be used for operations that cause side-effects, such as using it for
 * taking actions in web applications. One reason for this is that GET may be
 * used arbitrarily by robots or crawlers, which should not need to consider the
 * side effects that a request should cause.
 * @param req
 * @return
 */
esp_err_t static_files_get_handler(httpd_req_t *req)
{
    esp_err_t ret = ESP_OK;

    const char *uri = req->uri;

    ESP_LOGI(TAG_flash, "Requested static file %s", uri);

    auto found = find_file((const uint8_t *)uri);
    if (found.data != NULL)
    {
        httpd_resp_set_status(req, http_200_hdr);
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        httpd_resp_set_type(req, (char *)found.mime_type);
        httpd_resp_send(req, (char *)found.data, found.len);
        ESP_LOGI(TAG_flash, "Found: %lu, %s", found.len, found.mime_type);
    }
    else
    {
        httpd_resp_set_status(req, http_404_hdr);
        httpd_resp_send(req, NULL, 0);
        ESP_LOGE(TAG_flash, "GET not found");
    }

    return ret;
}

/* Main endpoint for HTTP page view */
/* There are 3 endpoints for access web page from diffrent device /, /static, /manifest.json */
static constexpr httpd_uri_t http_server_static_files = {
    .uri = "/static/*", .method = HTTP_GET,
    .handler = static_files_get_handler, 
    .user_ctx = NULL
};
/* Main endpoint for HTTP page view */
static constexpr httpd_uri_t http_server_root_handler = {
    .uri = "/", .method = HTTP_GET,
    .handler = static_files_get_handler, 
    .user_ctx = NULL
};
/* Main endpoint for HTTP page view */
static constexpr httpd_uri_t http_server_manifest_handler = {
    .uri = "/manifest.json", .method = HTTP_GET,
    .handler = static_files_get_handler, 
    .user_ctx = NULL
};

void register_webpage_flash_http_handlers(httpd_handle_t httpd_handle) {
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_handle, &http_server_root_handler));
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_handle, &http_server_manifest_handler));
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_handle, &http_server_static_files));
}
