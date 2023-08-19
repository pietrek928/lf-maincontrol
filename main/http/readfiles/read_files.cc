#include "read_files.h"

const char *TAG = "read_files.cc";

constexpr static const char base_path[] = "/sdcard";

/* Data structure what i would like to receive from the client */
struct FilePosDescr {
    uint32_t start = 0;
    uint32_t max_len = -1;
};

/* To use POST method, client has to send FilePosDescr structure and define data start and length*/
esp_err_t read_file_http_handler(httpd_req_t* req) {
    HTTP_HANDLER_GUARD(
        constexpr static uint32_t buffer_size = 1024;
        uint8_t data[buffer_size];

        auto post_size = get_post_data(req, data, buffer_size);
        FilePosDescr file_pos;
        if (post_size) {
            file_pos = extract_struct<FilePosDescr>( data, post_size);
        }

        // uri starts with `/read_file` -> length 10
        snprintf((char*)data, buffer_size - 2, "%s%s", base_path, req->uri + 10);
        ESP_LOGI(TAG, "Opening file %s", (char*)data);
        auto F = fopen((char*)data, "rb");
        if (unlikely(!F)) {
            throw HttpException(HTTPD_400_BAD_REQUEST, "Opening file failed.");
        }

        if (file_pos.start) {
            fseek(F, file_pos.start, SEEK_SET);
        }
        size_t pos = 0;
        while (pos < file_pos.max_len) {
            size_t to_read = buffer_size;
            if (pos + to_read >= file_pos.max_len) {
                to_read = file_pos.max_len - pos;
            }
            auto data_read = fread(data, 1, to_read, F);
            if (data_read <= 0) break;

            httpd_resp_send_chunk(req, (char*)data, data_read);
            pos += data_read;
        }

        httpd_resp_send_chunk(req, NULL, 0);
        fclose(F);
    );
}

esp_err_t list_files_http_handler(httpd_req_t* req) {
    HTTP_HANDLER_GUARD(
        constexpr static uint32_t buffer_size = 256;
        char data[buffer_size];

        // uri starts with `/list_files` -> length 11
        snprintf((char*)data, buffer_size - 2, "%s%s", base_path, req->uri + 11);
        auto dir = opendir(data);
        if (unlikely(!dir)) {
            throw HttpException(HTTPD_404_NOT_FOUND, "Opening directory failed.");
        }

        httpd_resp_set_type(req, http_content_type_json);

        JSON_TO_HTTP(req,
            struct dirent *entry;
            JSON_LIST(
                while ((entry = readdir(dir)) != NULL) {
                    JSON_SUBELEM(JSON_DICT(
                        JSON_KEY(name, entry->d_name);
                        JSON_KEY(type, entry->d_type);
                    ))
                }
            )
        );
    );
}

/* Get all data from the file */
static constexpr httpd_uri_t read_file_request_get_descr = {
    .uri = "/read_file/*", .method = HTTP_GET,
    .handler = read_file_http_handler, 
    .user_ctx = NULL
};
/* Read files in post manear with from-to function*/
static constexpr httpd_uri_t read_file_request_post_descr = {
    .uri = "/read_file/*", .method = HTTP_POST,
    .handler = read_file_http_handler, 
    .user_ctx = NULL
};
/* Basic list of files name */
static constexpr httpd_uri_t list_files_request_get_descr = {
    .uri = "/list_files/*", .method = HTTP_GET,
    .handler = list_files_http_handler, 
    .user_ctx = NULL
};

void register_read_files_http_handlers(httpd_handle_t httpd_handle) {
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_handle, &read_file_request_get_descr));
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_handle, &read_file_request_post_descr));
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_handle, &list_files_request_get_descr));
}
