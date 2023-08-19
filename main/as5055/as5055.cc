#include <esp_log.h>
#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "hal/spi_types.h"

#include "as5055.h"
#include "register.h"

/* @brief tag used for ESP serial console messages */
static const char TAG[] = "AS5055";

// SPI interface definitions for ESP32
static constexpr auto SPI_BUS = SPI2_HOST;

// global handles
TaskHandle_t as5055_task_handle = NULL;
spi_device_handle_t spi_handle;

constexpr static spi_bus_config_t bus_config = {
    .mosi_io_num = 25, // SPI MOSI GPIO
    .miso_io_num = 26, // SPI MISO GPIO
    .sclk_io_num = 27, // SPI CLK GPIO
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .data4_io_num = -1,
    .data5_io_num = -1,
    .data6_io_num = -1,
    .data7_io_num = -1,
    .max_transfer_sz = 32,
    .flags = 0,
    .intr_flags = 0,
};

constexpr static spi_device_interface_config_t device_config = {
    .command_bits = 0,
    .address_bits = 0,
    .dummy_bits = 0,
    .mode = 1,                                 //SPI mode 1
    .duty_cycle_pos = 0,
    .cs_ena_pretrans = 0,
    .cs_ena_posttrans = 0,
    .clock_speed_hz = 100000,                //Clock out at 100 kHz
    .input_delay_ns = 0,
    .spics_io_num = 33,                     // SPI CS GPIO 33 pin
    .flags = 0,
    .queue_size = 7,                           //We want to be able to queue 7 transactions at a time
    .pre_cb = NULL,
    .post_cb = NULL,                            //Specify pre-transfer callback to handle D/C line
};

/** Main initialization function of the sensor */
void as5055_init() {
    //Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_BUS, &bus_config, SPI_DMA_DISABLED));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_BUS, &device_config, &spi_handle));

    /* start wifi manager task */
    ESP_LOGI(TAG, "Task started!");

    // xTaskCreate(&as5055_task, "as5055_task", 4096, NULL, 10, &as5055_task_handle);
    xTaskCreate(&as5055_task, "as5055_task", 4096, NULL, 10, &as5055_task_handle);
}

/** Main sensor Task - read output data from Enkoder read Angle */
void as5055_task(void* pvParameters) {

    uint16_t data = 0x00;
    float angle = 0;

    // Soft reset
    data = AS_WRITE | SPI_REG_SOFT_RESET;
    as5055_send(spi_handle, data); 
    

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));

        // Czytanie kąta
        data = AS_READ | SPI_REG_DATA;
        data = as5055_send(spi_handle, data);

        if(  spiCalcEvenParity(data) || (data & 2) )
        {
            // Czyszczenie błędu
            data = AS_READ | SPI_REG_CLRERR;
            as5055_send(spi_handle, data);
            ESP_LOGE(TAG, "Clear warning");
        }
        else 
        { 
            data = (data >> 2) & 0x0fff; 
            angle = data * (2 * 3.141592f / (1 << 12));
        }

        ESP_LOGI(TAG, "Angle hex %x", data);
        ESP_LOGI(TAG, "Angle: %f", angle);
    }
    vTaskDelete(NULL);
}

/** Main sensor Task - read output data from Enkoder AGC value */ //TODO: validate
void as5055_task_AGC(void* pvParameters)
 {

    uint16_t data = 0x00;
    uint16_t agc = 0;

    // Soft reset
    data = AS_WRITE | SPI_REG_MASTER_RESET;
    as5055_send(spi_handle, data); 
    

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Czytanie AGC - czytanie natężenia strumienia magnetycznego 
        data = AS_READ | SPI_REG_AGC;
        data = as5055_send(spi_handle, data);


        if(  spiCalcEvenParity(data) || (data & 2) )
        {
            // Czyszczenie błędu
            data = AS_READ | SPI_REG_CLRERR;
            as5055_send(spi_handle, data);
            ESP_LOGE(TAG, "Clear warning");
        }
        else 
        { 
            agc = (data >> 2) & 0x3f;
        }

        ESP_LOGI(TAG, "AGC: %u", agc);

    }
    vTaskDelete(NULL);
}

uint16_t as5055_send(spi_device_handle_t spi_handler, uint16_t buf)
{
    buf = swap_bytes(buf | spiCalcEvenParity(buf));

    spi_transaction_t transaction = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = 16,
        .rxlength = 16,
        .user = (void*)1,
        .tx_buffer = (uint8_t*)&buf,
        .rx_buffer = (uint8_t*)&buf,
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_handler, &transaction));

    // Odwracan czytane bity
    buf = swap_bytes(buf); 

    return buf;
}

uint16_t swap_bytes(uint16_t data) {
    return (data << 8) | (data >> 8);
}

uint16_t spiCalcEvenParity(uint16_t value)
{
    value ^= value >> 8;
    value ^= value >> 4;
    value ^= value >> 2;
    value ^= value >> 1;

    return value & 0x1;
} 