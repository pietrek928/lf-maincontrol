#pragma once

#include <stdint.h>
#include <cstdint>

#define SINGLE_REG_READ 0x10
#define SINGLE_REG_WRITE 0x08
#define SET_BIT 0x18
#define CLEAR_BIT 0x20
#define READ_CONTINOUS 0x30
#define WRITE_CONTINOUS 0x28

enum ads7138_regaddr_t
{
    SYSTEM_STATUS = 0x00,
    GENERAL_CFG = 0x01,
    DATA_CFG = 0x02,
    OSR_CFG = 0x03,
    OPMODE_CFG = 0x04,
    PIN_CFG = 0x05,
    GPIO_CFG = 0x07,
    GPO_DRIVE_CFG = 0x09,
    GPO_VALUE = 0x0B,
    GPI_VALUE = 0x0D,
    SEQUENCE_CFG = 0x10,
    CHANNEL_SEL = 0x11,
    AUTO_SEQ_CH_SEL = 0x12,
    ALERT_CH_SEL = 0x14,
    ALERT_MAP = 0x16,
    ALERT_PIN_CFG = 0x17,
    EVENT_FLAG = 0x18,
    EVENT_HIGH_FLAG = 0x1A,
    EVENT_LOW_FLAG = 0x1C,
    EVENT_RGN = 0x1E,
    HYSTERESIS_CH0 = 0x20,
    HIGH_TH_CH0 = 0x21,
    EVENT_COUNT_CH0 = 0x22,
    LOW_TH_CH0 = 0x23,
    HYSTERESIS_CH1 = 0x24,
    HIGH_TH_CH1 = 0x25,
    EVENT_COUNT_CH1 = 0x26,
    LOW_TH_CH1 = 0x27,
    HYSTERESIS_CH2 = 0x28,
    HIGH_TH_CH2 = 0x29,
    EVENT_COUNT_CH2 = 0x2A,
    LOW_TH_CH2 = 0x2B,
    HYSTERESIS_CH3 = 0x2C,
    HIGH_TH_CH3 = 0x2D,
    EVENT_COUNT_CH3 = 0x2E,
    LOW_TH_CH3 = 0x2F,
    HYSTERESIS_CH4 = 0x30,
    HIGH_TH_CH4 = 0x31,
    EVENT_COUNT_CH4 = 0x32,
    LOW_TH_CH4 = 0x33,
    HYSTERESIS_CH5 = 0x34,
    HIGH_TH_CH5 = 0x35,
    EVENT_COUNT_CH5 = 0x36,
    LOW_TH_CH5 = 0x37,
    HYSTERESIS_CH6 = 0x38,
    HIGH_TH_CH6 = 0x39,
    EVENT_COUNT_CH6 = 0x3A,
    LOW_TH_CH6 = 0x3B,
    HYSTERESIS_CH7 = 0x3C,
    HIGH_TH_CH7 = 0x3D,
    EVENT_COUNT_CH7 = 0x3E,
    LOW_TH_CH7 = 0x3F,
    MAX_CH0_LSB = 0x60,
    MAX_CH0_MSB = 0x61,
    MAX_CH1_LSB = 0x62,
    MAX_CH1_MSB = 0x63,
    MAX_CH2_LSB = 0x64,
    MAX_CH2_MSB = 0x65,
    MAX_CH3_LSB = 0x66,
    MAX_CH3_MSB = 0x67,
    MAX_CH4_LSB = 0x68,
    MAX_CH4_MSB = 0x69,
    MAX_CH5_LSB = 0x6A,
    MAX_CH5_MSB = 0x6B,
    MAX_CH6_LSB = 0x6C,
    MAX_CH6_MSB = 0x6D,
    MAX_CH7_LSB = 0x6E,
    MAX_CH7_MSB = 0x6F,
    MIN_CH0_LSB = 0x80,
    MIN_CH0_MSB = 0x81,
    MIN_CH1_LSB = 0x82,
    MIN_CH1_MSB = 0x83,
    MIN_CH2_LSB = 0x84,
    MIN_CH2_MSB = 0x85,
    MIN_CH3_LSB = 0x86,
    MIN_CH3_MSB = 0x87,
    MIN_CH4_LSB = 0x88,
    MIN_CH4_MSB = 0x89,
    MIN_CH5_LSB = 0x8A,
    MIN_CH5_MSB = 0x8B,
    MIN_CH6_LSB = 0x8C,
    MIN_CH6_MSB = 0x8D,
    MIN_CH7_LSB = 0x8E,
    MIN_CH7_MSB = 0x8F,
    RECENT_CH0_LSB = 0xA0,
    RECENT_CH0_MSB = 0xA1,
    RECENT_CH1_LSB = 0xA2,
    RECENT_CH1_MSB = 0xA3,
    RECENT_CH2_LSB = 0xA4,
    RECENT_CH2_MSB = 0xA5,
    RECENT_CH3_LSB = 0xA6,
    RECENT_CH3_MSB = 0xA7,
    RECENT_CH4_LSB = 0xA8,
    RECENT_CH4_MSB = 0xA9,
    RECENT_CH5_LSB = 0xAA,
    RECENT_CH5_MSB = 0xAB,
    RECENT_CH6_LSB = 0xAC,
    RECENT_CH6_MSB = 0xAD,
    RECENT_CH7_LSB = 0xAE,
    RECENT_CH7_MSB = 0xAF,
    GPO0_TRIG_EVENT_SEL = 0xC3,
    GPO1_TRIG_EVENT_SEL = 0xC5,
    GPO2_TRIG_EVENT_SEL = 0xC7,
    GPO3_TRIG_EVENT_SEL = 0xC9,
    GPO4_TRIG_EVENT_SEL = 0xCB,
    GPO5_TRIG_EVENT_SEL = 0xCD,
    GPO6_TRIG_EVENT_SEL = 0xCF,
    GPO7_TRIG_EVENT_SEL = 0xD1,
    GPO_TRIGGER_CFG = 0xE9,
    GPO_VALUE_TRIG = 0xEB
};

struct __attribute__((packed)) ADS7138_DATA_CFG_regw_t
{
    uint8_t addr = (uint8_t)DATA_CFG;
    union
    {
        uint8_t value = 0;
        struct
        {
            uint8_t 
            RSVD        :1,
            CRC_EN      :1,
            STATS_EN    :1,
            DWC_EN      :1,
            CNVST       :1,
            CH_RST      :1,
            CAL         :1,
            RST         :1;
        };
    };
};

struct __attribute__((packed)) ADS7138_OSR_CFG_regw_t
{
    uint8_t addr = (uint8_t)OSR_CFG;
    union
    {
        uint8_t value = 0;
        struct
        {
            uint8_t 
            RSVD    :5,
            OSR     :3;
        };
    };
};

struct __attribute__((packed)) ADS7138_OPMODE_CFG_regw_t
{
    uint8_t addr = (uint8_t)OPMODE_CFG;
    union
    {
        uint8_t value = 0;
        struct
        {
            uint8_t
            CONV_ON_ERR     :1,
            CONV_MODE       :2,
            OSC_SEL         :1,
            CLK_DIV         :4;
        };
    };
};

struct __attribute__((packed)) ADS7138_SEQUENCE_CFG_regw_t
{
    uint8_t addr = (uint8_t)SEQUENCE_CFG;
    union
    {
        uint8_t value = 0;
        struct
        {
            uint8_t
            RSVD        :3,
            SEQ_START   :1,
            RSVD1       :2,
            SEQ_MODE    :2;
        };
    };
};