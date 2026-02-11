/**
 * @file pit_k64.h
 * @brief PIT (Periodic Interrupt Timer) driver for Kinetis K64
 * @version 1.0.0
 * @date 2026-02-10
 *
 * This file provides PIT functionality for precise timing and scheduling
 * of engine control events (fuel injection, ignition timing, etc.)
 *
 * Features:
 * - 4 independent timer channels
 * - 32-bit resolution
 * - Microsecond precision
 * - Interrupt-driven callbacks
 * - Chain timers for extended periods
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#ifndef PIT_K64_H
#define PIT_K64_H

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// PIT Channel Selection
//=============================================================================

typedef enum {
    PIT_CHANNEL_0 = 0,
    PIT_CHANNEL_1 = 1,
    PIT_CHANNEL_2 = 2,
    PIT_CHANNEL_3 = 3,
} pit_channel_t;

//=============================================================================
// PIT Configuration
//=============================================================================

typedef struct {
    uint32_t period_us;          // Timer period in microseconds
    bool enable_interrupt;       // Enable timer interrupt
    bool enable_chain;           // Chain with previous timer
} pit_config_t;

//=============================================================================
// PIT Callback Function Type
//=============================================================================

typedef void (*pit_callback_t)(void);

//=============================================================================
// PIT Register Definitions
//=============================================================================

#define PIT_BASE                    0x40037000

typedef struct {
    volatile uint32_t LDVAL;      // Timer Load Value Register
    volatile uint32_t CVAL;       // Current Timer Value Register
    volatile uint32_t TCTRL;      // Timer Control Register
    volatile uint32_t TFLG;       // Timer Flag Register
} PIT_TIMER_Type;

typedef struct {
    volatile uint32_t MCR;        // Module Control Register
    volatile uint32_t RESERVED0[55];
    PIT_TIMER_Type TIMER[4];      // Timer channels 0-3
} PIT_Type;

#define PIT                         ((PIT_Type*)PIT_BASE)

// PIT_MCR bits
#define PIT_MCR_MDIS                0x00000002  // Module Disable
#define PIT_MCR_FRZ                 0x00000001  // Freeze

// PIT_TCTRL bits
#define PIT_TCTRL_CHN               0x00000004  // Chain Mode
#define PIT_TCTRL_TIE               0x00000002  // Timer Interrupt Enable
#define PIT_TCTRL_TEN               0x00000001  // Timer Enable

// PIT_TFLG bits
#define PIT_TFLG_TIF                0x00000001  // Timer Interrupt Flag

//=============================================================================
// Function Prototypes
//=============================================================================

/**
 * @brief Initialize PIT module
 *
 * Enables the PIT clock and module
 */
void pit_init(void);

/**
 * @brief Configure PIT channel
 *
 * @param channel PIT channel (0-3)
 * @param config Pointer to PIT configuration structure
 * @return true if configuration successful
 */
bool pit_channel_init(pit_channel_t channel, const pit_config_t* config);

/**
 * @brief Register callback function for PIT channel
 *
 * @param channel PIT channel
 * @param callback Function to call on timer interrupt
 */
void pit_register_callback(pit_channel_t channel, pit_callback_t callback);

/**
 * @brief Start PIT channel timer
 *
 * @param channel PIT channel
 */
void pit_start(pit_channel_t channel);

/**
 * @brief Stop PIT channel timer
 *
 * @param channel PIT channel
 */
void pit_stop(pit_channel_t channel);

/**
 * @brief Set PIT channel period
 *
 * @param channel PIT channel
 * @param period_us Period in microseconds
 */
void pit_set_period_us(pit_channel_t channel, uint32_t period_us);

/**
 * @brief Get current timer value
 *
 * @param channel PIT channel
 * @return Current timer count value
 */
uint32_t pit_get_current_value(pit_channel_t channel);

/**
 * @brief Check if timer interrupt flag is set
 *
 * @param channel PIT channel
 * @return true if interrupt flag set
 */
bool pit_get_interrupt_flag(pit_channel_t channel);

/**
 * @brief Clear timer interrupt flag
 *
 * @param channel PIT channel
 */
void pit_clear_interrupt_flag(pit_channel_t channel);

#endif // PIT_K64_H
