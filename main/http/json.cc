#include "json.h"

#include <stdio.h>
#include <esp_http_server.h>

void __json_finish(void *http_req_handle) {
    httpd_resp_send_chunk((httpd_req_t *)http_req_handle, NULL, 0);
}

void __json_print(
    void *http_req_handle, const char* obj, size_t len
) {
    if (!len) return;
    httpd_resp_send_chunk((httpd_req_t *)http_req_handle, obj, len);
}

void __json_print(
    void *http_req_handle, const char* obj
) {
    if (!obj[0]) return;
    httpd_resp_sendstr_chunk((httpd_req_t *)http_req_handle, obj);
}

void __json_print(
    void *http_req_handle, char obj
) {
    __json_print(http_req_handle, &obj, 1);
}

void __json_put_object(
    void *http_req_handle, bool obj
) {
    auto val = obj ? "true" : "false";
    __json_print(http_req_handle, val);
}

void __json_put_object(
    void *http_req_handle, const char* obj
) {
    __json_print(http_req_handle, '"');
    __json_print(http_req_handle, obj);
    __json_print(http_req_handle, '"');
}

void __json_put_object(
    void *http_req_handle, int obj
) {
    char buffer[16];
    auto n_printed = snprintf(buffer, sizeof(buffer)-2, "%d", obj);
    if (n_printed < 0 || n_printed >= sizeof(buffer)-2) {
        return;
    }
    __json_print(http_req_handle, buffer, n_printed);
}

void __json_put_object(
    void *http_req_handle, uint32_t obj
) {
    char buffer[16];
    auto n_printed = snprintf(buffer, sizeof(buffer)-2, "%lu", obj);
    if (n_printed < 0 || n_printed >= sizeof(buffer)-2) {
        return;
    }
    __json_print(http_req_handle, buffer, n_printed);
}

void __json_put_object(
    void *http_req_handle, float obj
) {
    char buffer[16];
    auto n_printed = snprintf(buffer, sizeof(buffer)-2, "%f", obj);
    if (n_printed < 0 || n_printed >= sizeof(buffer)-2) {
        return;
    }
    __json_print(http_req_handle, buffer, n_printed);
}

void __json_put_object(
    void *http_req_handle, double obj
) {
    char buffer[32];
    auto n_printed = snprintf(buffer, sizeof(buffer)-2, "%lf", obj);
    if (n_printed < 0 || n_printed >= sizeof(buffer)-2) {
        return;
    }
    __json_print(http_req_handle, buffer, n_printed);
}

void __json_put_object(
    void *http_req_handle, const ipv4_t &obj
) {
    __json_print(http_req_handle, '"');
    __json_put_object(http_req_handle, obj.ip[0]);
    __json_print(http_req_handle, '.');
    __json_put_object(http_req_handle, obj.ip[1]);
    __json_print(http_req_handle, '.');
    __json_put_object(http_req_handle, obj.ip[2]);
    __json_print(http_req_handle, '.');
    __json_put_object(http_req_handle, obj.ip[3]);
    __json_print(http_req_handle, '"');
}
