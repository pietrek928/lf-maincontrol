#pragma once

#include <cstdint>

typedef struct
{
    uint8_t ip[4];
} ipv4_t;

void __json_finish(void* http_req_handle);
void __json_print(void* http_req_handle, const char* obj);
void __json_print(void* http_req_handle, char obj);
void __json_put_object(void* http_req_handle, bool obj);
void __json_put_object(void* http_req_handle, const char* obj);
void __json_put_object(void* http_req_handle, int obj);
void __json_put_object(void* http_req_handle, uint32_t obj);
void __json_put_object(void* http_req_handle, float obj);
void __json_put_object(void* http_req_handle, double obj);
void __json_put_object(void* http_req_handle, const ipv4_t& obj);

#define __JSON_COMA                                   \
    {                                                 \
        if (coma) __json_print(http_req_handle, ','); \
        coma = true;                                  \
    }

#define __JSON_COMA_START auto coma __attribute__((unused)) = false;

#define JSON_KEY(name, value)                      \
    {                                              \
        __JSON_COMA                                \
        __json_put_object(http_req_handle, #name); \
        __json_print(http_req_handle, ':');        \
        __json_put_object(http_req_handle, value); \
    }

#define JSON_SUBKEY(name, ...)                     \
    {                                              \
        __JSON_COMA                                \
        __json_put_object(http_req_handle, #name); \
        __json_print(http_req_handle, ':');        \
        {                                          \
            __JSON_COMA_START __VA_ARGS__          \
        }                                          \
    }

#define JSON_ELEM(obj)                           \
    {                                            \
        __JSON_COMA                              \
        __json_put_object(http_req_handle, obj); \
    }

#define JSON_SUBELEM(...)                             \
    {                                                 \
        __JSON_COMA { __JSON_COMA_START __VA_ARGS__ } \
    }

#define JSON_LIST(...)                                                      \
    {                                                                       \
        __json_print(http_req_handle, '[');                                 \
        {__JSON_COMA_START __VA_ARGS__} __json_print(http_req_handle, ']'); \
    }

#define JSON_DICT(...)                                                      \
    {                                                                       \
        __json_print(http_req_handle, '{');                                 \
        {__JSON_COMA_START __VA_ARGS__} __json_print(http_req_handle, '}'); \
    }

#define JSON_TO_HTTP(handler, ...)                                      \
    {                                                                   \
        void* http_req_handle = (void*)(handler);                       \
        {__JSON_COMA_START __VA_ARGS__} __json_finish(http_req_handle); \
    }
