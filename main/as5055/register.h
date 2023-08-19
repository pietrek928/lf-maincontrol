#pragma once

// Command values for reading and writing
#define AS_WRITE (0x0000)
#define AS_READ (0x8000)

#define SPI_CMD_READ 0x8000   /*!< flag indicating read attempt when using SPI interface */
#define SPI_REG_DATA 0x7ffe   /*!< data register when using SPI */
#define SPI_REG_AGC 0x7ff0    /*!< agc register when using SPI */
#define SPI_REG_CLRERR 0x6700 /*!< clear error register when using SPI */
#define SPI_REG_SOFT_RESET 0x7800
#define SPI_REG_MASTER_RESET 0x674a
#define SPI_REG_SYSTEM 0x7E40
