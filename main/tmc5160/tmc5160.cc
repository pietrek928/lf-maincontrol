#include "tmc5160.h"

#include "register.h"

/* @brief tag used for ESP serial console messages */
static const char TAG[] = "tmc5160";

// SPI interface definitions for ESP32
static constexpr auto TMC5160_SPI_BUS = SPI2_HOST;

constexpr static spi_bus_config_t bus_config = {
    .mosi_io_num = 25,  // SPI MOSI GPIO
    .miso_io_num = 26,  // SPI MISO GPIO
    .sclk_io_num = 27,  // SPI CLK GPIO
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
    .mode = 3,  // SPI mode 3
    .duty_cycle_pos = 0,
    .cs_ena_pretrans = 0,
    .cs_ena_posttrans = 0,
    .clock_speed_hz = 1000000,  // Clock out at 1MHz
    .input_delay_ns = 0,
    .spics_io_num = 33,  // SPI CS GPIO 33 pin
    .flags = 0,
    .queue_size = 7,  // We want to be able to queue 7 transactions at a time
    .pre_cb = NULL,
    .post_cb = NULL,  // Specify pre-transfer callback to handle D/C line
};

constexpr float velocity_scale = ((1 << 23) - 512);
constexpr float amax_scale = ((1 << 16) - 1);

// global handles
TaskHandle_t tmc5160_task_handle = NULL;
spi_device_handle_t spi_handle_tmc5160;
RAMP_MODE RAMPMODE_last = POSITION_MODE;

/** Main initialization function of the sensor */
void tmc5160_init()
{
    // Initialize the SPI bus
    ESP_ERROR_CHECK(
        spi_bus_initialize(TMC5160_SPI_BUS, &bus_config, SPI_DMA_DISABLED));
    ESP_ERROR_CHECK(spi_bus_add_device(
        TMC5160_SPI_BUS, &device_config, &spi_handle_tmc5160));

    /* Clear all reset flags */
    TMC5160_clearGStat();

    vTaskDelay(pdMS_TO_TICKS(10));

    tmc5160_init_registers();

    TMC5160_setAMAX(.05f);

    /* start wifi manager task */
    ESP_LOGI(TAG, "Task started!");

    // xTaskCreate(&tmc5160_task, "tmc5160_tgitask", 4096, NULL, 10,
    // &tmc5160_task_handle);
    xTaskCreate(
        &tmc5160_task, "tmc5160_task", 4096, NULL, 10, &tmc5160_task_handle);
}

/** Main sensor Task - read output data from Enkoder read Angle */
void tmc5160_task(void* pvParameters)
{
    while (1)
    {
        tmc5160_readGStat();
        auto reset_status = TMC5160_setVelocity(.1f);

        GSTAT_reg_t status_value;
        status_value.value = reset_status.value;
        if (unlikely(status_value.value))
        {
            TMC5160_clearGStat();

            if (status_value.uvcp)
            {
                ESP_LOGE(TAG, "TMC5160 undervoltage");
            }
            if (status_value.drverr)
            {
                ESP_LOGE(TAG, "TMC5160 driver error");
            }
            if (status_value.reset)
            {
                ESP_LOGE(TAG, "TMC5160 was reset");
                vTaskDelay(pdMS_TO_TICKS(10));
                tmc5160_init_registers();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

void tmc5160_init_registers()
{
    /* Enable motor */
    tmc5160_enable();

    /* Set stelth chopper */
    tmc5160_setStelthChopper();

    /* Set current */
    TMC5160_setCurrent(16, 31, 6);

    /* Set power down of 2 for stelth chopper*/
    TMC5160_setPowerDown(2);
}

/* Send address and 4 bytes of data*/
TMC5160_response_t tmc5160_send_comand(const uint8_t* data, size_t len)
{
    TMC5160_response_t response;

    spi_transaction_t transaction = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = 40,
        .rxlength = 40,
        .user = (void*)1,
        .tx_buffer = data,
        .rx_buffer = &response,
    };
    ESP_ERROR_CHECK(
        spi_device_polling_transmit(spi_handle_tmc5160, &transaction));

    return response;
}

/** Enable the motor */
void tmc5160_enable()
{
    // MULTISTEP_FILT=1, EN_PWM_MODE=1 enables stealthChop
    TMC5160_gconf_regw_t w_register = {
        .en_pwm_mode = 1,
        .multistep_filt = 1,  // TODO: zmieniłem na 1->0
    };
    tmc5160_send_comand((uint8_t*)&w_register, sizeof(w_register));
}

/** Set register StelthMode */
void tmc5160_setStelthChopper()
{
    // TOFF=3, HSTRT=4, HEND=1, TBL=2, CHM=0 (spreadCycle)
    TMC5160_CHOPCONF_regw_t w_register1 = {
        .tbl = 2,
        .chm = 0,
        .hend = 1,
        .hstrt = 4,
        .toff = 3,
    };
    tmc5160_send_comand((uint8_t*)&w_register1, sizeof(w_register1));

    // TPWMTHRS=500 This is the upper velocity for StealthChop voltage PWM mode
    TMC5160_TPWMTHRS_regw_t w_register2 = {
        .TPWMTHRS = 0,  // Górna prędkość sthealth choppera
    };
    tmc5160_send_comand((uint8_t*)&w_register2, sizeof(w_register2));
}

void TMC5160_clearGStat()
{
    TMC5160_data_reg_t w_register = {
        .addr = TMC5160Reg_GSTAT | WriteFlag,
        .data = swap_bytes((uint32_t)0xFFFFFFFF),
    };
    tmc5160_send_comand((uint8_t*)&w_register, sizeof(w_register));
}

/* Set register IHOLD and IRUN */
void TMC5160_setCurrent(uint8_t IHOLD, uint8_t IRUN, uint8_t HOLDDELAY)
{
    // IHOLD=10, IRUN=15 (max. current), IHOLDDELAY=6
    TMC5160_ihold_irun_regw_t w_register = {
        .ihold = IHOLD,
        .irun = IRUN,
        .iholddelay = HOLDDELAY,
    };
    tmc5160_send_comand((uint8_t*)&w_register, sizeof(w_register));
}

/** Set Power_Down reset */
void TMC5160_setPowerDown(uint8_t TPOWERDOWN)
{
    TMC5160_tpowerdown_regw_t w_register = {
        .tpowerdown = TPOWERDOWN,
    };

    tmc5160_send_comand((uint8_t*)&w_register, sizeof(w_register));
}

/** Manual mode setup */
void tmc5610_setRamp(
    uint8_t RampMode,
    uint32_t VSTART,
    uint32_t A1,
    uint32_t V1,
    uint32_t AMAX,
    uint32_t VMAX,
    uint32_t DMAX,
    uint32_t D1,
    uint32_t VSTOP)
{
    TMC5160_data_reg_t w_reg = {TMC5160Reg_RAMPMODE | WriteFlag, RampMode};
    tmc5160_send_comand((uint8_t*)&w_reg, sizeof(w_reg));

    w_reg = {TMC5160Reg_VSTART | WriteFlag, swap_bytes(VSTART)};
    tmc5160_send_comand((uint8_t*)&w_reg, sizeof(w_reg));

    w_reg = {TMC5160Reg_A1 | WriteFlag, swap_bytes(A1)};
    tmc5160_send_comand((uint8_t*)&w_reg, sizeof(w_reg));

    w_reg = {TMC5160Reg_V1 | WriteFlag, swap_bytes(V1)};
    tmc5160_send_comand((uint8_t*)&w_reg, sizeof(w_reg));

    w_reg = {TMC5160Reg_AMAX | WriteFlag, swap_bytes(AMAX)};
    tmc5160_send_comand((uint8_t*)&w_reg, sizeof(w_reg));

    w_reg = {TMC5160Reg_VMAX | WriteFlag, swap_bytes(VMAX)};
    tmc5160_send_comand((uint8_t*)&w_reg, sizeof(w_reg));

    w_reg = {TMC5160Reg_DMAX | WriteFlag, swap_bytes(DMAX)};
    tmc5160_send_comand((uint8_t*)&w_reg, sizeof(w_reg));

    w_reg = {TMC5160Reg_D1 | WriteFlag, swap_bytes(D1)};
    tmc5160_send_comand((uint8_t*)&w_reg, sizeof(w_reg));

    w_reg = {TMC5160Reg_VSTOP | WriteFlag, swap_bytes(VSTOP)};
    tmc5160_send_comand((uint8_t*)&w_reg, sizeof(w_reg));
}

/** Set position to the motor */
void tmc5160_setPosition(uint32_t position)
{
    TMC5160_data_reg_t w_register = {
        .addr = TMC5160Reg_XTARGET | WriteFlag,  // read
        .data = swap_bytes((uint32_t)position),
    };
    tmc5160_send_comand((uint8_t*)&w_register, sizeof(w_register));
}

/** Ordered reset flag read, value is returned in next command*/
void tmc5160_readGStat()
{
    TMC5160_data_reg_t w_register1 = {
        .addr = TMC5160Reg_GSTAT,  // read
        .data = swap_bytes((uint32_t)0),
    };
    tmc5160_send_comand((uint8_t*)&w_register1, sizeof(w_register1));
}

void TMC5160_setRampMode(RAMP_MODE mode)
{
    if (mode != RAMPMODE_last)
    {
        RAMPMODE_last = mode;
        TMC5160_data_reg_t w_register = {
            .addr = TMC5160Reg_RAMPMODE | WriteFlag,  // read
            .data = swap_bytes((uint32_t)mode),
        };
        tmc5160_send_comand((uint8_t*)&w_register, sizeof(w_register));
    }
}

/* Set register AMAX */
void TMC5160_setAMAX(float AMAX)
{
    AMAX = amax_scale * AMAX;

    TMC5160_data_reg_t w_register = {
        .addr = TMC5160Reg_AMAX | WriteFlag,  // read
        .data = swap_bytes((uint32_t)AMAX),
    };
    tmc5160_send_comand((uint8_t*)&w_register, sizeof(w_register));
}

// returns status and value previously ordered to read
TMC5160_response_t TMC5160_setVelocity(float v)
{
    RAMP_MODE mode;
    uint32_t vmax;
    if (v >= 0)
    {
        mode = VELOCITY_RIGHT_MODE;
        vmax = velocity_scale * v;
    }
    else
    {
        mode = VELOCITY_LEFT_MODE;
        vmax = velocity_scale * -v;
    }

    TMC5160_data_reg_t w_register = {
        .addr = TMC5160Reg_VMAX | WriteFlag,
        .data = swap_bytes((uint32_t)vmax),
    };
    TMC5160_response_t resp =
        tmc5160_send_comand((uint8_t*)&w_register, sizeof(w_register));

    TMC5160_setRampMode(mode);

    ESP_LOGI(TAG, "%lu", vmax);

    return resp;
}

/** Reverse byte position LSB->MSB */
uint16_t swap_bytes(uint16_t data) { return (data << 8) | (data >> 8); }

/** Reverse byte position LSB->MSB */
uint32_t swap_bytes(uint32_t data)
{
    return (data << 24) | ((data << 8) & 0x00FF0000) |
           ((data >> 8) & 0x0000FF00) | (data >> 24);
}
