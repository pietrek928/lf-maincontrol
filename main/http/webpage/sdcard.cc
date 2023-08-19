/* WEB on SD card in ESP32*/
#include "sdcard.h"

const char* TAG_sdcard = "Webpage SDcard server";

constexpr static const char sdcard_web_path[] = "/sdcard/webpage";
constexpr static const char write_file_path[] = "/write_file";

const char* mime_type_type_default = "application/octet-stream";

static const char* file_ext_mime_type[][2] = {
    {"jpg", "image/jpeg"},
    {"png", "image/png"},
    {"gif", "image/gif"},
    {"html", "text/html"},
    {"css", "text/css"},
    {"js", "application/javascript"},
    {"txt", "text/plain"},
    {"json", "application/json"},
    {"pdf", "application/pdf"},
    {"zip", "application/zip"},
    {"gz", "application/gzip"},
    {"ico", "image/x-icon"},
    {"svg", "image/svg+xml"},
    {"woff", "font/woff"},
    {"woff2", "font/woff2"},
    {"ttf", "font/ttf"},
    {"eot", "application/vnd.ms-fontobject"},
    {"otf", "font/otf"},
    {"xml", "text/xml"},
    {"mp4", "video/mp4"},
    {"mp3", "audio/mpeg"},
    {"wav", "audio/wav"},
    {"ogg", "audio/ogg"},
    {"webm", "video/webm"},
    {"", "application/octet-stream"},
};

static const char* path_mapping[][2] = {
    {"/", "/index.html"},
};

/*
    jebany esp ma jakieś nazwy plików z dosa
    https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/fatfs.html#using-fatfs-with-vfs
    https://en.wikipedia.org/wiki/8.3_filename
*/
void fix_path_for_dos(char* path)
{
    auto n = strlen(path);
    for (int i = 0; i < n; i++)
    {
        switch (path[i])
        {
            case '.':
                if (i + 5 >= n) break;
                path[i] = 'x';
                break;
            case '_':
            case ' ':
            case '-':
            case '+':
            case ',':
            case ';':
            case ':':
            case '=':
            case '[':
            case ']':
            case '(':
            case ')':
            case '!':
            case '@':
            case '#':
            case '$':
            case '%':
            case '^':
            case '&':
            case '{':
            case '}':
            case '~':
            case '`':
            case '\'':
            case '"':
            case '?':
            case '|':
            case '\\':
            case '*':
            case '<':
            case '>':
                path[i] = 'x';
            default:
                break;
        }
    }
}

/* Download static webpage file */
esp_err_t get_file_http_handler(httpd_req_t* req)
{
    HTTP_HANDLER_GUARD(
        constexpr static uint32_t buffer_size = 1024; uint8_t data[buffer_size];
        const char* file_path = req->uri;

        for (auto& path
             : ::path_mapping) {
            if (strcmp(path[0], file_path) == 0)
            {
                file_path = path[1];
                break;
            }
        }

        const char* file_ext = strrchr(file_path, '.');
        if (file_ext) {
            file_ext++;

            for (auto& ext : ::file_ext_mime_type)
            {
                if (strcmp(ext[0], file_ext) == 0)
                {
                    httpd_resp_set_type(req, ext[1]);
                    file_ext = NULL;
                    break;
                }
            }
            if (file_ext)
            {
                httpd_resp_set_type(req, mime_type_type_default);
            }
        }

        FILE* F = NULL;

        /* look for .gz file */
        snprintf(
            (char*)data,
            buffer_size - 2,
            "%s%s.gz",
            sdcard_web_path,
            file_path);
        fix_path_for_dos((char*)data);
        ESP_LOGI(TAG_sdcard, "Opening file %s", (char*)data);
        F = fopen((char*)data, "rb");
        if (F) { /* inform that file is gzip-encoded */
                 httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        }

        if (!F) {
            /* look for uncompressed file */
            snprintf(
                (char*)data,
                buffer_size - 2,
                "%s%s",
                sdcard_web_path,
                file_path);
            fix_path_for_dos((char*)data);
            ESP_LOGI(TAG_sdcard, "Opening file %s", (char*)data);
            F = fopen((char*)data, "rb");
        }

        if (unlikely(!F)) {
            throw HttpException(HTTPD_404_NOT_FOUND, "File not found.");
        }

        while (true) {  // stops when cant read more data
            auto data_read = fread(data, 1, buffer_size, F);
            if (data_read <= 0) break;

            httpd_resp_send_chunk(req, (char*)data, data_read);
        }

        httpd_resp_send_chunk(req, NULL, 0);
        fclose(F);)
}

class CloseFileAtEnd
{
   public:
    FILE* F = NULL;
    void close()
    {
        if (F) fclose(F);
        F = NULL;
    }
    operator FILE*() { return F; }
    CloseFileAtEnd(FILE* F) : F(F) {}
    ~CloseFileAtEnd() { close(); }
};

void prepare_dir(char* path_buf)
{
    char* last_slash = strrchr(path_buf, '/');
    if (last_slash)
    {
        *last_slash = 0;
        auto mkdir_result = mkdir(path_buf, 0777);
        if (mkdir_result)
        {  // mkdir failed - try to create parent directory first
            prepare_dir(path_buf);
            mkdir(path_buf, 0777);
        }
        *last_slash = '/';
    }
}

/* open and check requested file from uri */
FILE* open_file_to_write(const char* uri, const char* mode)
{
    char path_buf[128];
    if (strlen(uri) < sizeof(write_file_path))
    {
        throw HttpException(HTTPD_400_BAD_REQUEST, "Invalid path.");
    }
    uri += sizeof(write_file_path) - 1;
    snprintf(path_buf, sizeof(path_buf) - 1, "%s%s", sdcard_web_path, uri);
    fix_path_for_dos(path_buf);

    FILE* F = fopen(path_buf, mode);
    if (unlikely(!F))
    {
        /* create directory if not exists */
        prepare_dir(path_buf);
        F = fopen(path_buf, mode);
    }

    if (unlikely(!F))
    {
        throw HttpException(
            HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file.");
    }
    return F;
}

/* transfer post data from request to file */
void transfer_post_data(httpd_req_t* req, FILE* F, size_t data_size)
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

        size_t write_result = fwrite(data, 1, post_received, F);
        if (write_result != post_received)
        {
            throw HttpException(
                HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file.");
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

/* Upload static webpage file */
esp_err_t write_file_http_handler(httpd_req_t* req)
{
    HTTP_HANDLER_GUARD(
        constexpr static int recv_mode_size = 2;
        char file_mode[recv_mode_size + 1];
        if (  // first 3 bytes of post are file open mode
            req->content_len < recv_mode_size ||
            httpd_req_recv(req, file_mode, recv_mode_size) < recv_mode_size) {
            throw HttpException(
                HTTPD_400_BAD_REQUEST, "File mode not received.");
        } file_mode[recv_mode_size] = 0;

        auto F = CloseFileAtEnd(open_file_to_write(req->uri, file_mode));

        transfer_post_data(req, F, req->content_len - recv_mode_size);

        httpd_resp_set_status(req, http_200_hdr);
        httpd_resp_send(req, NULL, 0);)
}

/* Main endpoint for HTTP page view */
/* There are 3 endpoints for access web page from diffrent
 * device /, /static, /manifest.json */
static constexpr httpd_uri_t http_server_static_files = {
    .uri = "/static/*",
    .method = HTTP_GET,
    .handler = get_file_http_handler,
    .user_ctx = NULL};
/* Main endpoint for HTTP page view */
static constexpr httpd_uri_t http_server_root_handler = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_file_http_handler,
    .user_ctx = NULL};
/* Main endpoint for HTTP page view */
static constexpr httpd_uri_t http_server_manifest_handler = {
    .uri = "/manifest.json",
    .method = HTTP_GET,
    .handler = get_file_http_handler,
    .user_ctx = NULL};
/* Write to file handler */
static constexpr httpd_uri_t http_server_write_file_handler = {
    .uri = "/write_file/*",
    .method = HTTP_POST,
    .handler = write_file_http_handler,
    .user_ctx = NULL};

void register_webpage_sdcard_http_handlers(httpd_handle_t httpd_handle)
{
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(httpd_handle, &http_server_static_files));
    ESP_ERROR_CHECK(
        httpd_register_uri_handler(httpd_handle, &http_server_root_handler));
    ESP_ERROR_CHECK(httpd_register_uri_handler(
        httpd_handle, &http_server_manifest_handler));
    ESP_ERROR_CHECK(httpd_register_uri_handler(
        httpd_handle, &http_server_write_file_handler));
}
