/**
 * @file uart_k64.c
 * @brief UART driver implementation for Kinetis K64
 * @version 1.0.0
 * @date 2026-02-10
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#include "uart_k64.h"
#include "clock_k64.h"
#include "gpio_k64.h"

//=============================================================================
// SIM Register Access (for clock gating)
//=============================================================================

#define SIM_SCGC4_UART0             0x00000400
#define SIM_SCGC4_UART1             0x00000800
#define SIM_SCGC4_UART2             0x00001000
#define SIM_SCGC4_UART3             0x00002000
#define SIM_SCGC1_UART4             0x00000400
#define SIM_SCGC1_UART5             0x00000800

//=============================================================================
// Private Helper Functions
//=============================================================================

/**
 * @brief Get UART register pointer for a given instance
 *
 * @param instance UART instance
 * @return Pointer to UART registers
 */
static UART_Type* uart_get_regs(uart_instance_t instance) {
    switch (instance) {
        case UART_0: return UART0;
        case UART_1: return UART1;
        case UART_2: return UART2;
        case UART_3: return UART3;
        case UART_4: return UART4;
        case UART_5: return UART5;
        default: return NULL;
    }
}

/**
 * @brief Enable UART clock
 *
 * @param instance UART instance
 */
static void uart_enable_clock(uart_instance_t instance) {
    switch (instance) {
        case UART_0:
            SIM->SCGC4 |= SIM_SCGC4_UART0;
            break;
        case UART_1:
            SIM->SCGC4 |= SIM_SCGC4_UART1;
            break;
        case UART_2:
            SIM->SCGC4 |= SIM_SCGC4_UART2;
            break;
        case UART_3:
            SIM->SCGC4 |= SIM_SCGC4_UART3;
            break;
        case UART_4:
            SIM->SCGC1 |= SIM_SCGC1_UART4;
            break;
        case UART_5:
            SIM->SCGC1 |= SIM_SCGC1_UART5;
            break;
    }
}

/**
 * @brief Configure UART pins
 *
 * @param instance UART instance
 */
static void uart_configure_pins(uart_instance_t instance) {
    // Configure TX and RX pins based on UART instance
    // For Teensy 3.5, typical pin mappings are:
    // UART0: PTB16 (RX), PTB17 (TX) - or PTA1 (RX), PTA2 (TX)
    // UART1: PTC3 (RX), PTC4 (TX)
    // UART2: PTD2 (RX), PTD3 (TX)

    switch (instance) {
        case UART_0:
            // Use PTB16/PTB17 for UART0
            PORTB->PCR[16] = PORT_PCR_MUX(3); // RX (Alt 3)
            PORTB->PCR[17] = PORT_PCR_MUX(3); // TX (Alt 3)
            break;

        case UART_1:
            // Use PTC3/PTC4 for UART1
            PORTC->PCR[3] = PORT_PCR_MUX(3);  // RX (Alt 3)
            PORTC->PCR[4] = PORT_PCR_MUX(3);  // TX (Alt 3)
            break;

        case UART_2:
            // Use PTD2/PTD3 for UART2
            PORTD->PCR[2] = PORT_PCR_MUX(3);  // RX (Alt 3)
            PORTD->PCR[3] = PORT_PCR_MUX(3);  // TX (Alt 3)
            break;

        default:
            // Other UARTs not configured yet
            break;
    }
}

//=============================================================================
// Public Functions
//=============================================================================

void uart_init(uart_instance_t instance, const uart_config_t* config) {
    UART_Type* uart = uart_get_regs(instance);
    if (uart == NULL || config == NULL) {
        return;
    }

    // Enable UART clock
    uart_enable_clock(instance);

    // Configure pins
    uart_configure_pins(instance);

    // Disable transmitter and receiver while configuring
    uart->C2 = 0;

    // Calculate baud rate divisor
    // Baud rate = (module clock) / (16 * SBR)
    // SBR = (module clock) / (16 * baud rate)
    uint32_t module_clock = clock_get_bus_freq(); // UART0-3 use bus clock
    uint16_t sbr = module_clock / (16 * config->baud_rate);

    // Set baud rate
    uart->BDH = (sbr >> 8) & 0x1F;
    uart->BDL = sbr & 0xFF;

    // Configure 8-bit mode, no parity
    uart->C1 = 0;

    // Enable transmitter and/or receiver
    uint8_t c2 = 0;
    if (config->enable_tx) {
        c2 |= UART_C2_TE;
    }
    if (config->enable_rx) {
        c2 |= UART_C2_RE;
    }
    uart->C2 = c2;
}

void uart_putc(uart_instance_t instance, uint8_t data) {
    UART_Type* uart = uart_get_regs(instance);
    if (uart == NULL) {
        return;
    }

    // Wait for transmit buffer to be empty
    while (!(uart->S1 & UART_S1_TDRE)) {
        // Wait
    }

    // Write data to transmit buffer
    uart->D = data;
}

void uart_puts(uart_instance_t instance, const char* str) {
    while (*str) {
        uart_putc(instance, *str++);
    }
}

uint8_t uart_getc(uart_instance_t instance) {
    UART_Type* uart = uart_get_regs(instance);
    if (uart == NULL) {
        return 0;
    }

    // Wait for data to be received
    while (!(uart->S1 & UART_S1_RDRF)) {
        // Wait
    }

    // Read and return received data
    return uart->D;
}

bool uart_tx_ready(uart_instance_t instance) {
    UART_Type* uart = uart_get_regs(instance);
    if (uart == NULL) {
        return false;
    }

    return (uart->S1 & UART_S1_TDRE) != 0;
}

bool uart_rx_ready(uart_instance_t instance) {
    UART_Type* uart = uart_get_regs(instance);
    if (uart == NULL) {
        return false;
    }

    return (uart->S1 & UART_S1_RDRF) != 0;
}
