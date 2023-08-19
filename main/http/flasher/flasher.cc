#include "flasher.h"

#include <esp_err.h>
#include <esp_flash.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>

#include <cstdint>

#include "../json.h"
#include "../utils.h"

const char* TAG_flasher = "Flasher";

esp_err_t partition_info_http_handler(httpd_req_t* req)
{
    HTTP_HANDLER_GUARD(
        httpd_resp_set_type(req, http_content_type_json);

        JSON_TO_HTTP(
            req,
            JSON_DICT(
                JSON_SUBKEY(
                    partitions,
                    auto partition_it = esp_partition_find(
                        ESP_PARTITION_TYPE_ANY,
                        ESP_PARTITION_SUBTYPE_ANY,
                        NULL);
                    JSON_LIST(while (partition_it) {
                        auto partition = esp_partition_get(partition_it);
                        JSON_SUBELEM(JSON_DICT(
                            JSON_KEY(type, partition->type);
                            JSON_KEY(subtype, partition->subtype);
                            JSON_KEY(label, partition->label);
                            JSON_KEY(encrypted, partition->encrypted);
                            JSON_KEY(address, partition->address);
                            JSON_KEY(size, partition->size);
                            JSON_KEY(erase_size, partition->erase_size);))
                        partition_it = esp_partition_next(partition_it);
                    }) esp_partition_iterator_release(partition_it);)

                    JSON_SUBKEY(
                        app_info, auto ota_info = esp_app_get_description();
                        JSON_DICT(
                            JSON_KEY(magic_word, ota_info->magic_word);
                            JSON_KEY(secure_version, ota_info->secure_version);
                            JSON_KEY(version, ota_info->version);
                            JSON_KEY(project_name, ota_info->project_name);
                            JSON_KEY(time, ota_info->time);
                            JSON_KEY(date, ota_info->date);
                            JSON_KEY(idf_ver, ota_info->idf_ver);
                            JSON_KEY(
                                app_elf_sha256, ota_info->app_elf_sha256);))

                        JSON_SUBKEY(
                            ota,
                            auto boot_partition = esp_ota_get_boot_partition();
                            auto running_partition =
                                esp_ota_get_running_partition();
                            auto next_partition =
                                esp_ota_get_next_update_partition(NULL);
                            JSON_DICT(
                                if (boot_partition) {
                                    JSON_KEY(
                                        boot_partition, boot_partition->label);
                                } if (running_partition) {
                                    JSON_KEY(
                                        running_partition,
                                        running_partition->label);
                                } if (next_partition) {
                                    JSON_KEY(
                                        next_partition, next_partition->label);
                                })))););
}

/* transfer post data from request to flash */
void transfer_post_data_flash(
    httpd_req_t* req, uint32_t flash_offset, uint32_t data_size)
{
    constexpr static uint32_t buffer_size = 1024;
    uint8_t data[buffer_size];

    do
    {
        auto post_received = httpd_req_recv(req, (char*)data, buffer_size);
        if (post_received <= 0)
        {
            throw HttpException(
                HTTPD_500_INTERNAL_SERVER_ERROR, "Could not receive all data.");
        }

        auto write_result =
            esp_flash_write(NULL, data, flash_offset, post_received);

        ESP_LOGI(
            TAG_flasher, "Offset: %lX, length %u", flash_offset, post_received);

        if (write_result != ESP_OK)
        {
            throw HttpException(
                HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write flash.");
        }

        flash_offset += post_received;
        if (post_received <= data_size)
        {
            data_size -= post_received;
        }
        else
        {
            break;
        }
    } while (data_size > 0);
}

typedef struct
{
    uint32_t flash_offset;
    uint32_t data_size;
} FlashWrite;

esp_err_t write_flash_http_handler(httpd_req_t* req)
{
    HTTP_HANDLER_GUARD(
        FlashWrite write_data;
        if (req->content_len < sizeof(write_data) ||
            httpd_req_recv(req, (char*)&write_data, sizeof(write_data)) <
                sizeof(write_data)) {
            throw HttpException(
                HTTPD_400_BAD_REQUEST, "Flash offset not received.");
        }

        transfer_post_data_flash(req, write_data.flash_offset, write_data.data_size);

        httpd_resp_set_status(req, http_200_hdr);
        httpd_resp_send(req, NULL, 0);
    )
}

class EspOtaHandler {
    esp_ota_handle_t ota_handle = -1;
    public:
    EspOtaHandler(const esp_partition_t *partition, uint32_t size) {
        auto begin_result = esp_ota_begin(
            partition,
            size,
            &ota_handle
        );
        if (begin_result != ESP_OK) {
            throw HttpException(
                HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to begin ota.");
        }
    }
    operator esp_ota_handle_t() {
        return ota_handle;
    }
    ~EspOtaHandler() {
        if (ota_handle != -1) {
            auto end_result = esp_ota_end(ota_handle);
            if (end_result != ESP_OK) {
                ESP_LOGE(TAG_flasher, "Failed to end ota. code: %d", end_result);
            }
        }
    }
};

/* transfer post data from request to ota */
void transfer_post_data_ota(
    httpd_req_t* req, const esp_partition_t *partition, uint32_t data_size)
{
    EspOtaHandler handler(partition, data_size);

    constexpr static uint32_t buffer_size = 1024;
    uint8_t data[buffer_size];

    do
    {
        auto post_received = httpd_req_recv(req, (char*)data, buffer_size);
        if (post_received <= 0)
        {
            throw HttpException(
                HTTPD_500_INTERNAL_SERVER_ERROR, "Could not receive all data.");
        }

        auto write_result = esp_ota_write(handler, data, post_received);
        if (write_result != ESP_OK)
        {
            throw HttpException(
                HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write flash.");
        }

        if (post_received <= data_size)
        {
            data_size -= post_received;
        }
        else
        {
            break;
        }
    } while (data_size > 0);
}

typedef struct
{
    char partition_name[20];
    uint32_t data_size;
} OTAWrite;

esp_err_t write_ota_http_handler(httpd_req_t* req)
{
    HTTP_HANDLER_GUARD(
        OTAWrite write_data;
        if (req->content_len < sizeof(write_data) ||
            httpd_req_recv(req, (char*)&write_data, sizeof(write_data)) <
                sizeof(write_data)) {
            throw HttpException(
                HTTPD_400_BAD_REQUEST, "Flash offset not received.");
        }

        write_data.partition_name[sizeof(write_data.partition_name) - 1] = 0;
        auto partition = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_ANY,
            write_data.partition_name);
        if (!partition) {
            throw HttpException(HTTPD_404_NOT_FOUND, "Partition not found.");
        }

        transfer_post_data_ota(req, partition, write_data.data_size);

        httpd_resp_set_status(req, http_200_hdr);
        httpd_resp_send(req, NULL, 0);)
}

struct __attribute__((packed)) PartitionName
{
    char partition_name[20];
};

esp_err_t set_boot_partition_http_handler(httpd_req_t* req)
{
    HTTP_HANDLER_GUARD(
        PartitionName partition_name;
        if (req->content_len < sizeof(partition_name) ||
            httpd_req_recv(
                req, (char*)&partition_name, sizeof(partition_name)) <
                sizeof(partition_name)) {
            throw HttpException(
                HTTPD_400_BAD_REQUEST, "Partition name not received.");
        }

        partition_name.partition_name[sizeof(partition_name.partition_name) - 1] = 0;
        auto partition = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_ANY,
            partition_name.partition_name);
        if (!partition) {
            throw HttpException(HTTPD_404_NOT_FOUND, "Partition not found.");
        }

        auto set_result = esp_ota_set_boot_partition(partition);
        if (set_result != ESP_OK) {
            throw HttpException(
                HTTPD_500_INTERNAL_SERVER_ERROR,
                "Failed to set boot partition.");
        }

        httpd_resp_set_status(req, http_200_hdr);
        httpd_resp_send(req, NULL, 0);)
}

esp_err_t reboot_http_handler(httpd_req_t* req)
{
    HTTP_HANDLER_GUARD(
        esp_restart();

        httpd_resp_set_status(req, http_200_hdr);
        httpd_resp_send(req, NULL, 0);
    )
}

static constexpr httpd_uri_t http_server_partition_info = {
    .uri = "/flasher/partition_info",
    .method = HTTP_GET,
    .handler = partition_info_http_handler,
    .user_ctx = NULL};
static constexpr httpd_uri_t http_server_write_flash = {
    .uri = "/flasher/write_flash",
    .method = HTTP_POST,
    .handler = write_flash_http_handler,
    .user_ctx = NULL};
static constexpr httpd_uri_t http_server_write_ota = {
    .uri = "/flasher/write_ota",
    .method = HTTP_POST,
    .handler = write_ota_http_handler,
    .user_ctx = NULL};
static constexpr httpd_uri_t http_server_set_boot_partition = {
    .uri = "/flasher/set_boot_partition",
    .method = HTTP_POST,
    .handler = set_boot_partition_http_handler,
    .user_ctx = NULL};
static constexpr httpd_uri_t http_server_reboot = {
    .uri = "/flasher/reboot",
    .method = HTTP_POST,
    .handler = reboot_http_handler,
    .user_ctx = NULL};

void register_flasher_http_handlers(httpd_handle_t httpd_handle)
{
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(httpd_handle, &http_server_partition_info));
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(httpd_handle, &http_server_write_flash));
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(httpd_handle, &http_server_write_ota));
    ESP_ERROR_CHECK(httpd_register_uri_handler(
        httpd_handle, &http_server_set_boot_partition));
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(httpd_handle, &http_server_reboot));
}
