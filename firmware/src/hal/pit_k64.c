/**
 * @file pit_k64.c
 * @brief PIT driver implementation for Kinetis K64
 * @version 1.0.0
 * @date 2026-02-10
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#include "pit_k64.h"
#include "clock_k64.h"

//=============================================================================
// SIM Register Access (for clock gating)
//=============================================================================

#define SIM_SCGC6_PIT               0x00800000

//=============================================================================
// Private Variables
//=============================================================================

static pit_callback_t pit_callbacks[4] = {NULL, NULL, NULL, NULL};

//=============================================================================
// Private Helper Functions
//=============================================================================

/**
 * @brief Convert microseconds to timer ticks
 *
 * @param period_us Period in microseconds
 * @return Timer load value
 */
static uint32_t pit_us_to_ticks(uint32_t period_us) {
    // PIT runs at bus clock frequency (60 MHz)
    // ticks = (period_us * bus_clock) / 1,000,000
    uint32_t bus_clock = clock_get_bus_freq();
    uint64_t ticks = ((uint64_t)period_us * bus_clock) / 1000000;

    // PIT counts down from LDVAL to 0, so we need to subtract 1
    if (ticks > 0) {
        ticks--;
    }

    return (uint32_t)ticks;
}

//=============================================================================
// Interrupt Handlers
//=============================================================================

extern "C" void PIT0_IRQHandler(void) {
    if (PIT->TIMER[0].TFLG & PIT_TFLG_TIF) {
        // Clear interrupt flag
        PIT->TIMER[0].TFLG = PIT_TFLG_TIF;

        // Call registered callback
        if (pit_callbacks[0] != NULL) {
            pit_callbacks[0]();
        }
    }
}

extern "C" void PIT1_IRQHandler(void) {
    if (PIT->TIMER[1].TFLG & PIT_TFLG_TIF) {
        PIT->TIMER[1].TFLG = PIT_TFLG_TIF;
        if (pit_callbacks[1] != NULL) {
            pit_callbacks[1]();
        }
    }
}

extern "C" void PIT2_IRQHandler(void) {
    if (PIT->TIMER[2].TFLG & PIT_TFLG_TIF) {
        PIT->TIMER[2].TFLG = PIT_TFLG_TIF;
        if (pit_callbacks[2] != NULL) {
            pit_callbacks[2]();
        }
    }
}

extern "C" void PIT3_IRQHandler(void) {
    if (PIT->TIMER[3].TFLG & PIT_TFLG_TIF) {
        PIT->TIMER[3].TFLG = PIT_TFLG_TIF;
        if (pit_callbacks[3] != NULL) {
            pit_callbacks[3]();
        }
    }
}

//=============================================================================
// Public Functions
//=============================================================================

void pit_init(void) {
    // Enable PIT clock
    SIM->SCGC6 |= SIM_SCGC6_PIT;

    // Enable PIT module and allow timers to run in debug mode
    PIT->MCR = 0;  // Clear MDIS and FRZ bits

    // Enable PIT interrupts in NVIC
    // PIT0-PIT3 are IRQ 68-71
    *((volatile uint32_t*)0xE000E104) = (1 << ((68 - 32) % 32));  // NVIC_ISER2 for IRQ 68
    *((volatile uint32_t*)0xE000E104) = (1 << ((69 - 32) % 32));  // IRQ 69
    *((volatile uint32_t*)0xE000E104) = (1 << ((70 - 32) % 32));  // IRQ 70
    *((volatile uint32_t*)0xE000E104) = (1 << ((71 - 32) % 32));  // IRQ 71
}

bool pit_channel_init(pit_channel_t channel, const pit_config_t* config) {
    if (channel > PIT_CHANNEL_3 || config == NULL) {
        return false;
    }

    // Disable timer while configuring
    PIT->TIMER[channel].TCTRL = 0;

    // Set timer period
    uint32_t ticks = pit_us_to_ticks(config->period_us);
    PIT->TIMER[channel].LDVAL = ticks;

    // Configure timer control register
    uint32_t tctrl = 0;

    if (config->enable_interrupt) {
        tctrl |= PIT_TCTRL_TIE;
    }

    if (config->enable_chain) {
        tctrl |= PIT_TCTRL_CHN;
    }

    PIT->TIMER[channel].TCTRL = tctrl;

    return true;
}

void pit_register_callback(pit_channel_t channel, pit_callback_t callback) {
    if (channel <= PIT_CHANNEL_3) {
        pit_callbacks[channel] = callback;
    }
}

void pit_start(pit_channel_t channel) {
    if (channel <= PIT_CHANNEL_3) {
        PIT->TIMER[channel].TCTRL |= PIT_TCTRL_TEN;
    }
}

void pit_stop(pit_channel_t channel) {
    if (channel <= PIT_CHANNEL_3) {
        PIT->TIMER[channel].TCTRL &= ~PIT_TCTRL_TEN;
    }
}

void pit_set_period_us(pit_channel_t channel, uint32_t period_us) {
    if (channel <= PIT_CHANNEL_3) {
        uint32_t ticks = pit_us_to_ticks(period_us);
        PIT->TIMER[channel].LDVAL = ticks;
    }
}

uint32_t pit_get_current_value(pit_channel_t channel) {
    if (channel <= PIT_CHANNEL_3) {
        return PIT->TIMER[channel].CVAL;
    }
    return 0;
}

bool pit_get_interrupt_flag(pit_channel_t channel) {
    if (channel <= PIT_CHANNEL_3) {
        return (PIT->TIMER[channel].TFLG & PIT_TFLG_TIF) != 0;
    }
    return false;
}

void pit_clear_interrupt_flag(pit_channel_t channel) {
    if (channel <= PIT_CHANNEL_3) {
        PIT->TIMER[channel].TFLG = PIT_TFLG_TIF;
    }
}
