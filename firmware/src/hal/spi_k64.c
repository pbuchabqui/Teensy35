/**
 * @file spi_k64.c
 * @brief SPI Driver for Teensy 3.5 (Kinetis K64)
 * @version 1.0.0
 * @date 2026-02-21
 * 
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#include "spi_k64.h"
#include "sim_k64.h"
#include "gpio_k64.h"
#include <stdint.h>

//=============================================================================
// SPI Register Definitions
//=============================================================================

#define SPI0_BASE       0x4002C000
#define SPI1_BASE       0x4002D000
#define SPI2_BASE       0x400AC000

// SPI Registers
#define SPI_MCR(base)   (*((volatile uint32_t*)(base + 0x00)))
#define SPI_TCR(base)   (*((volatile uint32_t*)(base + 0x08)))
#define SPI_CTAR(base, n) (*((volatile uint32_t*)(base + 0x0C + (n * 4))))
#define SPI_SR(base)    (*((volatile uint32_t*)(base + 0x2C)))
#define SPI_DR(base)    (*((volatile uint32_t*)(base + 0x30)))
#define SPI_PUSHR(base) (*((volatile uint32_t*)(base + 0x34)))
#define SPI_POPR(base)  (*((volatile uint32_t*)(base + 0x38)))

// SPI MCR bits
#define SPI_MCR_HALT        (1 << 0)
#define SPI_MCR_SMPL_PT(x)  ((x) << 8)
#define SPI_MCR_CLR_TXF     (1 << 10)
#define SPI_MCR_CLR_RXF     (1 << 11)
#define SPI_MCR_MDIS        (1 << 12)
#define SPI_MCR_DIS_TXF     (1 << 13)
#define SPI_MCR_DIS_RXF     (1 << 14)
#define SPI_MCR_MSTR       (1 << 31)

// SPI TCR bits
#define SPI_TCR_CPOL        (1 << 31)
#define SPI_TCR_CPHA        (1 << 30)
#define SPI_TCR_LSBFE       (1 << 29)
#define SPI_TCR_PCS(x)      ((x) << 16)

// SPI CTAR bits
#define SPI_CTAR_FMSZ(x)    ((x) << 27)
#define SPI_CTAR_CPOL       (1 << 26)
#define SPI_CTAR_CPHA       (1 << 25)
#define SPI_CTAR_LSBFE      (1 << 24)
#define SPI_CTAR_PCSSCK(x)  ((x) << 22)
#define SPI_CTAR_PASC(x)    ((x) << 20)
#define SPI_CTAR_PDT(x)     ((x) << 18)
#define SPI_CTAR_PBR(x)     ((x) << 16)
#define SPI_CTAR_CSSCK(x)   ((x) << 12)
#define SPI_CTAR_ASC(x)     ((x) << 8)
#define SPI_CTAR_DT(x)      ((x) << 4)
#define SPI_CTAR_BR(x)      ((x) << 0)

// SPI SR bits
#define SPI_SR_TCF          (1 << 31)
#define SPI_SR_TXCTR(x)     (((x) >> 16) & 0xF)
#define SPI_SR_RXCTR(x)     (((x) >> 4) & 0xF)
#define SPI_SR_EOQF         (1 << 28)
#define SPI_SR_TFUF         (1 << 27)
#define SPI_SR_TFFF         (1 << 26)
#define SPI_SR_RFOF         (1 << 25)
#define SPI_SR_RFDF         (1 << 24)
#define SPI_SR_TXFULL       (1 << 23)
#define SPI_SR_RXEMPTY      (1 << 22)

// SPI PUSHR bits
#define SPI_PUSHR_CONT      (1 << 31)
#define SPI_PUSHR_EOQ       (1 << 27)
#define SPI_PUSHR_CTCAS     (1 << 26)
#define SPI_PUSHR_PCS(x)    ((x) << 16)
#define SPI_PUSHR_TXDATA(x) ((x) & 0xFFFF)

// SPI POPR bits
#define SPI_POPR_RXDATA(x)  ((x) & 0xFFFF)

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
// SPI Configuration Structure
//=============================================================================

typedef struct {
    uint32_t base;
    uint8_t  mode;
    uint8_t  clock_div;
    uint8_t  data_size;
    uint8_t  cpol;
    uint8_t  cpha;
    uint8_t  initialized;
} spi_config_t;

//=============================================================================
// Global Variables
//=============================================================================

static spi_config_t spi_configs[3] = {
    {SPI0_BASE, 0, SPI_CLOCK_DIV256, 8, 0, 0, 0},
    {SPI1_BASE, 0, SPI_CLOCK_DIV256, 8, 0, 0, 0},
    {SPI2_BASE, 0, SPI_CLOCK_DIV256, 8, 0, 0, 0}
};

//=============================================================================
// Private Function Prototypes
//=============================================================================

static uint32_t spi_get_base(spi_port_t port);
static void spi_configure_ctar(spi_port_t port);

//=============================================================================
// SPI Private Functions
//=============================================================================

static uint32_t spi_get_base(spi_port_t port) {
    switch (port) {
        case SPI_0: return SPI0_BASE;
        case SPI_1: return SPI1_BASE;
        case SPI_2: return SPI2_BASE;
        default: return SPI0_BASE;
    }
}

static void spi_configure_ctar(spi_port_t port) {
    uint32_t base = spi_get_base(port);
    spi_config_t* config = &spi_configs[port];
    
    // Configure CTAR0 for master mode
    uint32_t ctar = SPI_CTAR_FMSZ(config->data_size - 1) |
                    SPI_CTAR_BR(config->clock_div);
    
    if (config->cpol) {
        ctar |= SPI_CTAR_CPOL;
    }
    
    if (config->cpha) {
        ctar |= SPI_CTAR_CPHA;
    }
    
    SPI_CTAR(base, 0) = ctar;
}

//=============================================================================
// SPI Public Functions
//=============================================================================

void spi_init(spi_port_t port, uint8_t mode, uint8_t clock_div) {
    uint32_t base = spi_get_base(port);
    spi_config_t* config = &spi_configs[port];
    
    config->mode = mode;
    config->clock_div = clock_div;
    config->data_size = 8;
    config->cpol = 0;
    config->cpha = 0;
    
    // Enable SPI clock
    if (port == SPI_0) {
        SIM->SCGC6 |= SIM_SCGC6_SPI0_MASK;
    } else if (port == SPI_1) {
        SIM->SCGC6 |= SIM_SCGC6_SPI1_MASK;
    } else if (port == SPI_2) {
        SIM->SCGC3 |= SIM_SCGC3_SPI2_MASK;
    }
    
    // Configure SPI
    SPI_MCR(base) = SPI_MCR_MDIS | SPI_MCR_HALT | SPI_MCR_CLR_TXF | SPI_MCR_CLR_RXF;
    
    // Configure CTAR
    spi_configure_ctar(port);
    
    // Enable SPI in master mode
    SPI_MCR(base) = SPI_MCR_MSTR | SPI_MCR_CLR_TXF | SPI_MCR_CLR_RXF;
    
    config->initialized = 1;
}

void spi_set_mode(spi_port_t port, uint8_t cpol, uint8_t cpha) {
    spi_config_t* config = &spi_configs[port];
    
    config->cpol = cpol;
    config->cpha = cpha;
    
    spi_configure_ctar(port);
}

void spi_set_clock(spi_port_t port, uint8_t clock_div) {
    spi_config_t* config = &spi_configs[port];
    
    config->clock_div = clock_div;
    spi_configure_ctar(port);
}

void spi_transmit(spi_port_t port, const uint8_t* data, uint16_t length) {
    uint32_t base = spi_get_base(port);
    
    for (uint16_t i = 0; i < length; i++) {
        // Wait for TX FIFO not full
        while (!(SPI_SR(base) & SPI_SR_TFFF));
        
        // Send data
        SPI_PUSHR(base) = SPI_PUSHR_TXDATA(data[i]);
        
        // Wait for transfer complete
        while (!(SPI_SR(base) & SPI_SR_TCF));
        
        // Clear TCF flag
        SPI_SR(base) = SPI_SR_TCF;
    }
}

void spi_receive(spi_port_t port, uint8_t* data, uint16_t length) {
    uint32_t base = spi_get_base(port);
    
    for (uint16_t i = 0; i < length; i++) {
        // Send dummy byte
        while (!(SPI_SR(base) & SPI_SR_TFFF));
        SPI_PUSHR(base) = SPI_PUSHR_TXDATA(0xFF);
        
        // Wait for receive data
        while (!(SPI_SR(base) & SPI_SR_RFDF));
        
        // Read data
        data[i] = SPI_POPR(base) & 0xFF;
        
        // Clear RFDF flag
        SPI_SR(base) = SPI_SR_RFDF;
    }
}

void spi_transmit_receive(spi_port_t port, const uint8_t* tx_data, uint8_t* rx_data, uint16_t length) {
    uint32_t base = spi_get_base(port);
    
    for (uint16_t i = 0; i < length; i++) {
        // Send data
        while (!(SPI_SR(base) & SPI_SR_TFFF));
        SPI_PUSHR(base) = SPI_PUSHR_TXDATA(tx_data[i]);
        
        // Wait for transfer complete and receive data
        while (!(SPI_SR(base) & SPI_SR_RFDF));
        
        // Read received data
        rx_data[i] = SPI_POPR(base) & 0xFF;
        
        // Clear flags
        SPI_SR(base) = SPI_SR_RFDF | SPI_SR_TCF;
    }
}

uint8_t spi_transmit_byte(spi_port_t port, uint8_t data) {
    uint32_t base = spi_get_base(port);
    uint8_t rx_data;
    
    // Send data
    while (!(SPI_SR(base) & SPI_SR_TFFF));
    SPI_PUSHR(base) = SPI_PUSHR_TXDATA(data);
    
    // Wait for transfer complete and receive data
    while (!(SPI_SR(base) & SPI_SR_RFDF));
    
    // Read received data
    rx_data = SPI_POPR(base) & 0xFF;
    
    // Clear flags
    SPI_SR(base) = SPI_SR_RFDF | SPI_SR_TCF;
    
    return rx_data;
}
