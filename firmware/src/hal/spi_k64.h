/**
 * @file spi_k64.h
 * @brief SPI Driver for Teensy 3.5 (Kinetis K64)
 * @version 1.0.0
 * @date 2026-02-21
 * 
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#ifndef SPI_K64_H
#define SPI_K64_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// SPI Ports
//=============================================================================

typedef enum {
    SPI_0 = 0,
    SPI_1 = 1,
    SPI_2 = 2
} spi_port_t;

//=============================================================================
// SPI Modes
//=============================================================================

#define SPI_MODE_MASTER    0
#define SPI_MODE_SLAVE     1

//=============================================================================
// SPI Clock Dividers
//=============================================================================

#define SPI_CLOCK_DIV2      0   // 30MHz
#define SPI_CLOCK_DIV4      1   // 15MHz
#define SPI_CLOCK_DIV8      2   // 7.5MHz
#define SPI_CLOCK_DIV16     3   // 3.75MHz
#define SPI_CLOCK_DIV32     4   // 1.875MHz
#define SPI_CLOCK_DIV64     5   // 937.5kHz
#define SPI_CLOCK_DIV128    6   // 468.75kHz
#define SPI_CLOCK_DIV256    7   // 234.375kHz

//=============================================================================
// SPI Function Prototypes
//=============================================================================

void spi_init(spi_port_t port, uint8_t mode, uint8_t clock_div);
void spi_set_mode(spi_port_t port, uint8_t cpol, uint8_t cpha);
void spi_set_clock(spi_port_t port, uint8_t clock_div);
void spi_transmit(spi_port_t port, const uint8_t* data, uint16_t length);
void spi_receive(spi_port_t port, uint8_t* data, uint16_t length);
void spi_transmit_receive(spi_port_t port, const uint8_t* tx_data, uint8_t* rx_data, uint16_t length);
uint8_t spi_transmit_byte(spi_port_t port, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif // SPI_K64_H
