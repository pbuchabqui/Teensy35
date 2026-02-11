/**
 * @file uart_k64.h
 * @brief UART driver for Kinetis K64 (Teensy 3.5)
 * @version 1.0.0
 * @date 2026-02-10
 *
 * This file provides basic UART (Universal Asynchronous Receiver/Transmitter)
 * functionality for serial communication and debugging.
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#ifndef UART_K64_H
#define UART_K64_H

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// UART Instances
//=============================================================================

typedef enum {
    UART_0 = 0,
    UART_1 = 1,
    UART_2 = 2,
    UART_3 = 3,
    UART_4 = 4,
    UART_5 = 5,
} uart_instance_t;

//=============================================================================
// UART Configuration
//=============================================================================

typedef struct {
    uint32_t baud_rate;       // Baud rate (e.g., 9600, 115200)
    bool enable_tx;           // Enable transmitter
    bool enable_rx;           // Enable receiver
} uart_config_t;

//=============================================================================
// UART Register Definitions
//=============================================================================

#define UART0_BASE                  0x4006A000
#define UART1_BASE                  0x4006B000
#define UART2_BASE                  0x4006C000
#define UART3_BASE                  0x4006D000
#define UART4_BASE                  0x4006E000
#define UART5_BASE                  0x4006F000

typedef struct {
    volatile uint8_t  BDH;        // Baud Rate Register High
    volatile uint8_t  BDL;        // Baud Rate Register Low
    volatile uint8_t  C1;         // Control Register 1
    volatile uint8_t  C2;         // Control Register 2
    volatile uint8_t  S1;         // Status Register 1
    volatile uint8_t  S2;         // Status Register 2
    volatile uint8_t  C3;         // Control Register 3
    volatile uint8_t  D;          // Data Register
    volatile uint8_t  MA1;        // Match Address Register 1
    volatile uint8_t  MA2;        // Match Address Register 2
    volatile uint8_t  C4;         // Control Register 4
    volatile uint8_t  C5;         // Control Register 5
    volatile uint8_t  ED;         // Extended Data Register
    volatile uint8_t  MODEM;      // Modem Register
    volatile uint8_t  IR;         // Infrared Register
    volatile uint8_t  RESERVED0;
    volatile uint8_t  PFIFO;      // FIFO Parameters
    volatile uint8_t  CFIFO;      // FIFO Control Register
    volatile uint8_t  SFIFO;      // FIFO Status Register
    volatile uint8_t  TWFIFO;     // FIFO Transmit Watermark
    volatile uint8_t  TCFIFO;     // FIFO Transmit Count
    volatile uint8_t  RWFIFO;     // FIFO Receive Watermark
    volatile uint8_t  RCFIFO;     // FIFO Receive Count
    volatile uint8_t  RESERVED1;
    volatile uint8_t  C7816;      // 7816 Control Register
    volatile uint8_t  IE7816;     // 7816 Interrupt Enable Register
    volatile uint8_t  IS7816;     // 7816 Interrupt Status Register
    volatile uint8_t  WP7816;     // 7816 Wait Parameter Register
    volatile uint8_t  WN7816;     // 7816 Wait N Register
    volatile uint8_t  WF7816;     // 7816 Wait FD Register
    volatile uint8_t  ET7816;     // 7816 Error Threshold Register
    volatile uint8_t  TL7816;     // 7816 Transmit Length Register
    volatile uint8_t  RESERVED2;
    volatile uint8_t  AP7816A_T0; // 7816 ATR Duration Timer Register A
    volatile uint8_t  AP7816B_T0; // 7816 ATR Duration Timer Register B
    union {
        volatile uint8_t WP7816A_T0;  // 7816 Wait Parameter Register A
        volatile uint8_t WP7816A_T1;  // 7816 Wait Parameter Register A
    };
    union {
        volatile uint8_t WP7816B_T0;  // 7816 Wait Parameter Register B
        volatile uint8_t WP7816B_T1;  // 7816 Wait Parameter Register B
    };
    volatile uint8_t WGP7816_T1;  // 7816 Wait and Guard Parameter Register
    volatile uint8_t WP7816C_T1;  // 7816 Wait Parameter Register C
} UART_Type;

#define UART0                       ((UART_Type*)UART0_BASE)
#define UART1                       ((UART_Type*)UART1_BASE)
#define UART2                       ((UART_Type*)UART2_BASE)
#define UART3                       ((UART_Type*)UART3_BASE)
#define UART4                       ((UART_Type*)UART4_BASE)
#define UART5                       ((UART_Type*)UART5_BASE)

// UART_C2 bits
#define UART_C2_TIE                 0x80  // Transmit Interrupt Enable
#define UART_C2_TCIE                0x40  // Transmission Complete Interrupt Enable
#define UART_C2_RIE                 0x20  // Receiver Interrupt Enable
#define UART_C2_ILIE                0x10  // Idle Line Interrupt Enable
#define UART_C2_TE                  0x08  // Transmitter Enable
#define UART_C2_RE                  0x04  // Receiver Enable
#define UART_C2_RWU                 0x02  // Receiver Wakeup Control
#define UART_C2_SBK                 0x01  // Send Break

// UART_S1 bits
#define UART_S1_TDRE                0x80  // Transmit Data Register Empty Flag
#define UART_S1_TC                  0x40  // Transmission Complete Flag
#define UART_S1_RDRF                0x20  // Receive Data Register Full Flag
#define UART_S1_IDLE                0x10  // Idle Line Flag
#define UART_S1_OR                  0x08  // Receiver Overrun Flag
#define UART_S1_NF                  0x04  // Noise Flag
#define UART_S1_FE                  0x02  // Framing Error Flag
#define UART_S1_PF                  0x01  // Parity Error Flag

//=============================================================================
// Function Prototypes
//=============================================================================

/**
 * @brief Initialize UART with specified configuration
 *
 * @param instance UART instance (0-5)
 * @param config Pointer to UART configuration structure
 */
void uart_init(uart_instance_t instance, const uart_config_t* config);

/**
 * @brief Transmit a single byte
 *
 * @param instance UART instance
 * @param data Byte to transmit
 */
void uart_putc(uart_instance_t instance, uint8_t data);

/**
 * @brief Transmit a string
 *
 * @param instance UART instance
 * @param str Null-terminated string to transmit
 */
void uart_puts(uart_instance_t instance, const char* str);

/**
 * @brief Receive a single byte (blocking)
 *
 * @param instance UART instance
 * @return Received byte
 */
uint8_t uart_getc(uart_instance_t instance);

/**
 * @brief Check if transmit buffer is empty
 *
 * @param instance UART instance
 * @return true if transmit buffer is empty
 */
bool uart_tx_ready(uart_instance_t instance);

/**
 * @brief Check if receive buffer has data
 *
 * @param instance UART instance
 * @return true if data is available
 */
bool uart_rx_ready(uart_instance_t instance);

#endif // UART_K64_H
