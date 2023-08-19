#include <cstdint>

#define WriteFlag 0x80

// register addresses
enum tmc5160_regaddr_t
{
    TMC5160Reg_GCONF = 0x00,
    TMC5160Reg_GSTAT = 0x01,
    TMC5160Reg_IFCNT = 0x02,
    TMC5160Reg_SLAVECONF = 0x03,
    TMC5160Reg_IOIN = 0x04,

    TMC5160Reg_OUTPUT = 0x05,
    TMC5160Reg_X_COMPARE = 0x06,
    TMC5160Reg_OTP_READ = 0x07,
    TMC5160Reg_FACTORY_CONF = 0x08,
    TMC5160Reg_SHORT_CONF = 0x09,
    TMC5160Reg_DRV_CONF = 0x0A,
    TMC5160Reg_GLOBAL_SCALER = 0x0B,
    TMC5160Reg_OFFSET_READ = 0x0C,

    TMC5160Reg_IHOLD_IRUN = 0x10,
    TMC5160Reg_TPOWERDOWN = 0x11,
    TMC5160Reg_TSTEP = 0x12,
    TMC5160Reg_TPWMTHRS = 0x13,
    TMC5160Reg_TCOOLTHRS = 0x14,
    TMC5160Reg_THIGH = 0x15,

    TMC5160Reg_RAMPMODE = 0x0020,
    TMC5160Reg_XACTUAL = 0x0021,
    TMC5160Reg_VACTUAL = 0x0022,
    TMC5160Reg_VSTART = 0x0023,
    TMC5160Reg_A1 = 0x0024,
    TMC5160Reg_V1 = 0x0025,
    TMC5160Reg_AMAX = 0x0026,
    TMC5160Reg_VMAX = 0x0027,
    TMC5160Reg_DMAX = 0x0028,
    TMC5160Reg_D1 = 0x002A,
    TMC5160Reg_VSTOP = 0x002B,
    TMC5160Reg_TZEROWAIT = 0x002C,
    TMC5160Reg_XTARGET = 0x2D,

    TMC5160Reg_VDCMIN = 0x0033,
    TMC5160Reg_SW_MODE = 0x0034,
    TMC5160Reg_RAMP_STAT = 0x0035,
    TMC5160Reg_XLATCH = 0x0036,

    TMC5160Reg_MSLUT_BASE = 0x60,
    TMC5160Reg_MSLUTSEL = 0x68,
    TMC5160Reg_MSLUTSTART = 0x69,
    TMC5160Reg_MSCNT = 0x6A,
    TMC5160Reg_MSCURACT = 0x6B,
    TMC5160Reg_CHOPCONF = 0x6C,
    TMC5160Reg_COOLCONF = 0x6D,
    TMC5160Reg_DCCTRL = 0x6E,
    TMC5160Reg_DRV_STATUS = 0x6F,
    TMC5160Reg_PWMCONF = 0x70,
    TMC5160Reg_PWM_SCALE = 0x71,
    TMC5160Reg_PWM_AUTO = 0x72,
    TMC5160Reg_LOST_STEPS = 0x73,
};

struct __attribute__((packed)) TMC5160_gconf_regw_t
{
    uint8_t addr = (uint8_t)TMC5160Reg_GCONF | WriteFlag;
    union
    {
        uint32_t value = 0;
        struct
        {
            uint32_t recalibrate : 1, faststandstill : 1, en_pwm_mode : 1,
                multistep_filt : 1, shaft : 1, diag0_error : 1, diag0_otpw : 1,
                diag0_stall : 1, diag1_stall : 1, diag1_index : 1,
                diag1_onstate : 1, diag1_steps_skipped : 1,
                diag0_int_pushpull : 1, diag1_poscomp_pushpull : 1,
                small_hysteresis : 1, stop_enable : 1, direct_mode : 1,
                test_mode : 1, reserved : 14;
        };
    };
};

struct __attribute__((packed)) TMC5160_ihold_irun_regw_t
{
    uint8_t addr = (uint8_t)TMC5160Reg_IHOLD_IRUN | WriteFlag;
    union
    {
        uint32_t value = 0;
        struct
        {
            uint32_t ihold : 5, reserved1 : 3, irun : 5, reserved2 : 3,
                iholddelay : 4, reserved3 : 12;
        };
    };
};

struct __attribute__((packed)) TMC5160_tpowerdown_regw_t
{
    uint8_t addr = (uint8_t)TMC5160Reg_TPOWERDOWN | WriteFlag;
    union
    {
        uint32_t value = 0;
        struct
        {
            uint32_t tpowerdown : 8, reserved : 24;
        };
    };
};

struct __attribute__((packed)) TMC5160_CHOPCONF_regw_t
{
    uint8_t addr = (uint8_t)TMC5160Reg_CHOPCONF | WriteFlag;
    union
    {
        uint32_t value = 0;
        struct
        {
            uint32_t diss2vs : 1, diss2g : 1, dedge : 1, intpol : 1, mres : 4,
                tpfd : 4, vhighchm : 1, vhighfs : 1, rsvd : 1, tbl : 2, chm : 1,
                rsvd1 : 1, disfdcc : 1, fd3 : 1, hend : 4, hstrt : 3, toff : 4;
        };
    };
};

struct __attribute__((packed)) TMC5160_TPWMTHRS_regw_t
{
    uint8_t addr = (uint8_t)TMC5160Reg_TPWMTHRS | WriteFlag;
    union
    {
        uint32_t value = 0;
        struct
        {
            uint32_t TPWMTHRS : 32;
        };
    };
};

struct __attribute__((packed)) TMC5160_data_reg_t
{
    uint8_t addr = 0;
    uint32_t data = 0;
};

typedef union
{
    uint32_t value = 0;
    struct
    {
        uint32_t reset : 1, drverr : 1, uvcp : 1, reserved : 29;
    };
} GSTAT_reg_t;
