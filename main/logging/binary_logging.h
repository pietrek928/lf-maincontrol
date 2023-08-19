#pragma once

#include <cstdint>
#include <cstdio>

// #define __STRINGIFY(x) #x
#define STRINGIFY(x) __STRINGIFY(x)

#define MAX_LOG_RECORD_SIZE 1024

#define ENABLE_TYPENAME(A) \
    inline const char* get_typename(A v) { return #A; }

ENABLE_TYPENAME(uint8_t)
ENABLE_TYPENAME(int8_t)
ENABLE_TYPENAME(uint16_t)
ENABLE_TYPENAME(int16_t)
ENABLE_TYPENAME(uint32_t)
ENABLE_TYPENAME(int32_t)
ENABLE_TYPENAME(float)
ENABLE_TYPENAME(double)

template <class Ta>
struct ArrayDescr
{
    Ta* ptr;
    uint32_t size;

    ArrayDescr(Ta* ptr, unsigned int size) : ptr(ptr), size(size) {}

    auto get_size() { return sizeof(Ta) * size; }

    void write_value(void* data) { memcpy(data, (void*)ptr, get_size()); }

    void write_json_descr(FILE* F)
    {
        fprintf(
            F,
            "{\"type\": \"%s\", \"count\": \"%u\"}",
            get_typename(*(Ta*)-1),
            size);
    }
};

template <class T>
struct ValueDescr
{
    T v;
    ValueDescr(T v) : v(v) {}

    auto get_size() { return sizeof(T); }

    void write_value(void* data) { *((T*)data) = v; }

    void write_json_descr(FILE* F)
    {
        fprintf(F, "{\"type\": \"%s\"}", get_typename(v));
    }
};

template <class T>
inline auto get_writer(ArrayDescr<T> v)
{
    return v;
}

template <class T>
inline auto get_writer(T v)
{
    return ValueDescr<T>(v);
}

// Create description of given parameters
template <class Tv1, class... Targs>
void write_json_descr_items(FILE* F, Tv1 v1, Targs... args)
{
    get_writer(v1).write_json_descr(F);
    // Check amount of arguments
    if constexpr (sizeof...(args))
    {
        fprintf(F, ",");
        write_json_descr_items(F, args...);
    }
}

template <class... Targs>
void write_json_descr(
    const char* descr_fname, const char* vnames, Targs... args)
{
    auto F = fopen(descr_fname, "w");
    if (F == NULL)
    {
        ESP_LOGE("bSDcard: ", "Failed to open file for writing");
        return;
    }
    fprintf(F, "{\"names\":\"%s\", \"types\":[", vnames);
    write_json_descr_items(F, args...);
    fprintf(F, "]}");
    fclose(F);
}

// Write all data into buffor
template <class Tv1, class... Targs>
auto assemble_record(uint8_t* buf, Tv1 v1, Targs... args)
{
    auto w = get_writer(v1);
    w.write_value((void*)buf);
    auto s = w.get_size();
    if constexpr (sizeof...(args))
    {
        s += assemble_record(buf + w.get_size(), args...);
    }
    return s;
}

template <class... Targs>
void write_record(FILE* F, Targs... args)
{
    uint8_t buf[MAX_LOG_RECORD_SIZE];
    auto s = assemble_record(buf, args...);
    // TODO: asssert s <= MAX_RECORD_SIZE
    fwrite(buf, 1, s, F);
    fflush(F);
}

// TODO: some unique/random number in file name ?

/**
 * @brief LOG_VALUES(zmienna1, zmienna2...) podajemy w argumencie zmienne jakie
 * chcemy zapisać do pliku Musi być stała o znanym rozmiarze podana jako
 * argument #define SD_MOUNT "/sdcard" - należy dodać int nie działa, uint32_t
 * działa NIE PRZYJMUJE INT
 */
#define LOG_VALUES(LOG_NAME, ...)                                              \
    {                                                                          \
        FILE* F = fopen(SD_MOUNT "/logs/" LOG_NAME ".bin", "r+");              \
        if (likely(F))                                                         \
        { /* File open */                                                      \
            fseek(F, 0, SEEK_END);                                             \
            write_record(F, __VA_ARGS__);                                      \
            fclose(F);                                                         \
        }                                                                      \
        else /* File not open */                                               \
        {                                                                      \
            ESP_LOGI(                                                          \
                "bSDCard",                                                     \
                "Opening JSON file " SD_MOUNT "/logs/" LOG_NAME ".bin");       \
            write_json_descr(                                                  \
                SD_MOUNT "/logs/" LOG_NAME ".txt", #__VA_ARGS__, __VA_ARGS__); \
            ESP_LOGI(                                                          \
                "bSDCard", "Opening DATA " SD_MOUNT "/logs/" LOG_NAME ".txt"); \
            F = fopen(SD_MOUNT "/logs/" LOG_NAME ".bin", "w");                 \
            if (likely(F))                                                     \
            {                                                                  \
                write_record(F, __VA_ARGS__);                                  \
                fclose(F);                                                     \
            }                                                                  \
            else                                                               \
            {                                                                  \
                ESP_LOGE("bSDcard: ", "Failed to open file for writing");      \
            }                                                                  \
        }                                                                      \
    }
