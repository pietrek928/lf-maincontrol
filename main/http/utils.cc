#include "utils.h"

#include <esp_log.h>

static const char TAG[] = "http_post";

uint32_t get_post_data(httpd_req_t* req, uint8_t* buf, uint32_t buf_size)
{
    size_t recv_size = req->content_len;

    if (recv_size) {
        ESP_LOGI(TAG, "POST request %s: %zu", req->uri, recv_size);
    }

    if (recv_size > buf_size)
    {
        throw HttpException(HTTPD_431_REQ_HDR_FIELDS_TOO_LARGE, "error post - za duzo danych");
    }

    if (recv_size)
    {
        int ret = httpd_req_recv(req, (char*)buf, buf_size);
        if (ret <= 0)
        { /* <0 return value indicates connection closed */
            /* Check if timeout occurred */
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                /* In case of timeout one can choose to retry calling
                 * httpd_req_recv(), but to keep it simple, here we
                 * respond with an HTTP 408 (Request Timeout) error */
                throw HttpException(HTTPD_408_REQ_TIMEOUT, "request timeout");
            }
            /* In case of error, returning ESP_FAIL will
             * ensure that the underlying socket is closed */
            throw HttpException(HTTPD_500_INTERNAL_SERVER_ERROR, "connection error");
            // return ESP_FAIL;
        }
        else
        {
            ESP_LOGI(TAG, "Recived %d", ret);
            return ret;
        }
    }

    return 0;
}
