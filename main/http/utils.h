#pragma once

#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_log.h>

#include <cstdint>

/* const httpd related values stored in flash */
const static char http_200_hdr[] = "200 OK";
const static char http_302_hdr[] = "302 Found";
const static char http_400_hdr[] = "400 Bad Request";
const static char http_404_hdr[] = "404 Not Found";
const static char http_503_hdr[] = "503 Service Unavailable";
const static char http_location_hdr[] = "Location";
const static char http_content_type_html[] = "text/html";
const static char http_content_type_js[] = "text/javascript";
const static char http_content_type_css[] = "text/css";
const static char http_content_type_json[] = "application/json";
const static char http_cache_control_hdr[] = "Cache-Control";
const static char http_cache_control_no_cache[] = "no-store, no-cache, must-revalidate, max-age=0";
const static char http_cache_control_cache[] = "public, max-age=31536000";
const static char http_pragma_hdr[] = "Pragma";
const static char http_pragma_no_cache[] = "no-cache";

class HttpException
{
   public:
    httpd_err_code_t code;
    const char* message;

    HttpException(httpd_err_code_t code, const char* message) : code(code), message(message) {}
};

#define HTTP_HANDLER_GUARD(...)                                             \
    {                                                                       \
        try                                                                 \
        {                                                                   \
            __VA_ARGS__                                                     \
        }                                                                   \
        catch (const HttpException & err)                                   \
        {                                                                   \
            ESP_LOGE(__FILE__, "HTTP error %d %s", err.code, err.message);  \
            httpd_resp_send_err(req, err.code, err.message);                \
            return ESP_FAIL;                                                \
        }                                                                   \
        return ESP_OK;                                                      \
    }

uint32_t get_post_data(httpd_req_t* req, uint8_t* buf, uint32_t buf_size);

template <class Tstruct>
const Tstruct& extract_struct(const uint8_t* data, uint32_t data_size)
{
    if (data_size < sizeof(Tstruct))
    {
        throw HttpException(HTTPD_400_BAD_REQUEST, "Za mało danych!");
    }
    return *(const Tstruct*)data;
}

template <class Texecute>
uint32_t execute_struct(const uint8_t* data, uint32_t data_size)
{
    auto& unpacked_data = extract_struct<Texecute>(data, data_size);
    unpacked_data.execute();

    return sizeof(unpacked_data);
}

