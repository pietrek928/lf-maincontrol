/* Main data logger */
#include "sd_logger.h"

static const char TAG[] = "SDcard";

TaskHandle_t SDcard_task_handle = NULL;

esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 3,
    .allocation_unit_size = 0,
    .disk_status_check_enable = false};

/**
 * @brief  Main initialization fucntion of the SD card
 *
 */
void sd_card_init()
{
    ESP_LOGI(TAG, "Initializing SD card");

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;  // Bez tego nie działa - przypuszczam w ESP32 jest
                            // to SPI normal, w S3 sprawdzić z 4 jako quad SPI

    sdmmc_host_t host_config = SDMMC_HOST_DEFAULT();

    // Use settings defined above to initialize SD card and mount FAT
    // filesystem.
    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(
        SD_MOUNT, &host_config, &slot_config, &mount_config, &card);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(
                TAG,
                "Failed to mount filesystem. If you want the card to be "
                "formatted, set format_if_mount_failed = true.");
        }
        else
        {
            ESP_LOGE(
                TAG,
                "Failed to initialize the card (%d). Make sure SD card lines "
                "have pull-up resistors in place.",
                ret);
        }
        return;
    }

    /* Print card info */
    sdmmc_card_print_info(stdout, card);

    f_mkdir("logs");
    f_mkdir("webpage");

    /* start wifi manager task */
    ESP_LOGI(TAG, "Task started!");

    xTaskCreate(
        &SDcard_task,
        "SDcard_task",
        4096,
        NULL,
        configMAX_PRIORITIES - 1,
        &SDcard_task_handle);
}

/**
 * @brief Loging data to SD card - only for loging purposes
 *
 */
void SDcard_task(void* pvParameters)
{
    uint32_t val = 666;
    uint32_t val1 = 777;
    uint32_t time = 0;

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        // Write/Read tests
        // const char* myString = "Lubie Kotki!";
        // save_data("/sdcard/logs/data.gz", myString);

        // save_logs("/sdcard/data1.txt", (void *)myString);

        // vTaskDelay(1000/ portTICK_PERIOD_MS);
        // read_data_to_logs("/sdcard/data1.txt");

        // PJ binary logging method
        time++;
        LOG_VALUES("data3", time, val, val1);
    }
    vTaskDelete(NULL);
}

/**
 * @brief Save and create the new file based on the data
 *
 * @param fname
 * @param data
 */
void save_data(const char* fname, const char* data)
{
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(TAG, "%s", "Opening file");
    FILE* file = fopen(fname, "wb");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "%s", "Failed to open file for writing");
        return;
    }
    fprintf(file, "Data %s!\n", data);
    fclose(file);
    ESP_LOGI(TAG, "%s", "File written");
}

/**
 * @brief Write a new line to the file or create the new file
 *
 * @param fname
 * @param data
 */
void save_logs(const char* fname, void* data)
{
    FILE* file = fopen(fname, "a");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "%s", "Failed to open file for writing");
        return;
    }
    fprintf(file, "Data %s!\n", (char*)data);
    fclose(file);
}

/**
 * @brief Read last line from the file
 *
 * @param fname
 */
void read_data_to_logs(const char* fname)
{
    FILE* file = fopen(fname, "r");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "%s", "Failed to open file for writing");
        return;
    }

    char buffer[100];

    // while (fscanf(file, "%s", buffer) != EOF )

    if (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        ESP_LOGI(TAG, "Line: %s", buffer);
    }

    fclose(file);
}
