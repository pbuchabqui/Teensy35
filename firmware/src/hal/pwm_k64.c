/**
 * @file pwm_k64.c
 * @brief PWM driver implementation for Kinetis K64
 * @version 1.0.0
 * @date 2026-02-10
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#include "pwm_k64.h"
#include "clock_k64.h"

//=============================================================================
// SIM Register Access (for clock gating)
//=============================================================================

#define SIM_SCGC6_FTM0              0x01000000
#define SIM_SCGC6_FTM1              0x02000000
#define SIM_SCGC6_FTM2              0x04000000
#define SIM_SCGC3_FTM2              0x01000000
#define SIM_SCGC3_FTM3              0x00000040

//=============================================================================
// Private Constants
//=============================================================================

#define PWM_MAX_PRESCALER           7     // Maximum prescaler value (divide by 128)
#define PWM_MIN_FREQUENCY           1     // Minimum frequency in Hz
#define PWM_MAX_FREQUENCY           1000000  // Maximum frequency in Hz

//=============================================================================
// Private Helper Functions
//=============================================================================

/**
 * @brief Get FTM register pointer for a given module
 *
 * @param ftm FTM module
 * @return Pointer to FTM registers
 */
static FTM_Type* pwm_get_regs(pwm_ftm_t ftm) {
    switch (ftm) {
        case PWM_FTM0: return FTM0;
        case PWM_FTM1: return FTM1;
        case PWM_FTM2: return FTM2;
        case PWM_FTM3: return FTM3;
        default: return NULL;
    }
}

/**
 * @brief Enable FTM clock
 *
 * @param ftm FTM module
 */
static void pwm_enable_clock(pwm_ftm_t ftm) {
    switch (ftm) {
        case PWM_FTM0:
            SIM->SCGC6 |= SIM_SCGC6_FTM0;
            break;
        case PWM_FTM1:
            SIM->SCGC6 |= SIM_SCGC6_FTM1;
            break;
        case PWM_FTM2:
            SIM->SCGC3 |= SIM_SCGC3_FTM2;
            break;
        case PWM_FTM3:
            SIM->SCGC3 |= SIM_SCGC3_FTM3;
            break;
    }
}

/**
 * @brief Calculate prescaler and modulo for desired frequency
 *
 * @param frequency_hz Desired frequency in Hz
 * @param prescaler Output: calculated prescaler value
 * @param modulo Output: calculated modulo value
 * @return true if valid values found
 */
static bool pwm_calculate_parameters(uint32_t frequency_hz,
                                     uint8_t* prescaler,
                                     uint16_t* modulo) {
    if (frequency_hz < PWM_MIN_FREQUENCY || frequency_hz > PWM_MAX_FREQUENCY) {
        return false;
    }

    uint32_t bus_clock = clock_get_bus_freq();

    // Try each prescaler value (0-7 = divide by 1, 2, 4, 8, 16, 32, 64, 128)
    for (uint8_t ps = 0; ps <= PWM_MAX_PRESCALER; ps++) {
        uint32_t prescaler_div = (1 << ps);  // 2^ps
        uint32_t mod_value = (bus_clock / prescaler_div / frequency_hz) - 1;

        // Check if modulo fits in 16-bit register
        if (mod_value > 0 && mod_value <= 0xFFFF) {
            *prescaler = ps;
            *modulo = (uint16_t)mod_value;
            return true;
        }
    }

    return false;
}

//=============================================================================
// Public Functions
//=============================================================================

bool pwm_init(pwm_ftm_t ftm, const pwm_config_t* config) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || config == NULL) {
        return false;
    }

    // Enable FTM clock
    pwm_enable_clock(ftm);

    // Disable counter while configuring
    ftm_regs->SC = 0;
    ftm_regs->CNT = 0;

    // Calculate prescaler and modulo
    uint8_t prescaler;
    uint16_t modulo;

    if (config->enable_prescaler_auto) {
        if (!pwm_calculate_parameters(config->frequency_hz, &prescaler, &modulo)) {
            return false;  // Invalid frequency
        }
    } else {
        // Use default prescaler (divide by 1) and calculate modulo
        prescaler = 0;
        uint32_t bus_clock = clock_get_bus_freq();
        modulo = (uint16_t)((bus_clock / config->frequency_hz) - 1);
    }

    // Set modulo (period)
    ftm_regs->MOD = modulo;

    // Set initial counter value to 0
    ftm_regs->CNTIN = 0;

    // Configure FTM_SC
    uint32_t sc = FTM_SC_CLKS(1) |  // System clock
                  FTM_SC_PS(prescaler);

    // Set center-aligned mode if requested
    if (config->alignment == PWM_CENTER_ALIGNED) {
        sc |= FTM_SC_CPWMS;
    }

    ftm_regs->SC = sc;

    // Enable PWM loading
    ftm_regs->PWMLOAD = 0xFF;  // Enable loading for all channels

    return true;
}

bool pwm_channel_init(pwm_ftm_t ftm, pwm_channel_t channel,
                     const pwm_channel_config_t* config) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || config == NULL || channel > PWM_CHANNEL_7) {
        return false;
    }

    // Set initial duty cycle
    uint16_t modulo = ftm_regs->MOD;
    uint16_t duty_value = (modulo * config->duty_cycle_percent) / 100;
    ftm_regs->CONTROLS[channel].CnV = duty_value;

    // Configure channel mode (edge-aligned PWM)
    uint32_t cnsc = FTM_CnSC_MSB;  // PWM mode

    // Set polarity
    if (config->polarity == PWM_POLARITY_HIGH) {
        cnsc |= FTM_CnSC_ELSB;  // High-true pulses
    } else {
        cnsc |= FTM_CnSC_ELSA;  // Low-true pulses
    }

    ftm_regs->CONTROLS[channel].CnSC = cnsc;

    // Enable output if requested
    if (config->enable_output) {
        // Clear output mask bit to enable output
        ftm_regs->OUTMASK &= ~(1 << channel);
    } else {
        // Set output mask bit to disable output
        ftm_regs->OUTMASK |= (1 << channel);
    }

    return true;
}

void pwm_set_duty_cycle(pwm_ftm_t ftm, pwm_channel_t channel,
                       uint16_t duty_percent) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return;
    }

    // Clamp duty cycle to 0-100%
    if (duty_percent > 100) {
        duty_percent = 100;
    }

    // Calculate duty value based on modulo
    uint16_t modulo = ftm_regs->MOD;
    uint16_t duty_value = (modulo * duty_percent) / 100;

    ftm_regs->CONTROLS[channel].CnV = duty_value;
}

void pwm_set_duty_value(pwm_ftm_t ftm, pwm_channel_t channel,
                       uint16_t duty_value) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return;
    }

    ftm_regs->CONTROLS[channel].CnV = duty_value;
}

bool pwm_set_frequency(pwm_ftm_t ftm, uint32_t frequency_hz) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL) {
        return false;
    }

    // Calculate new prescaler and modulo
    uint8_t prescaler;
    uint16_t modulo;

    if (!pwm_calculate_parameters(frequency_hz, &prescaler, &modulo)) {
        return false;  // Invalid frequency
    }

    // Disable counter
    uint32_t sc = ftm_regs->SC;
    ftm_regs->SC = 0;

    // Update modulo and prescaler
    ftm_regs->MOD = modulo;
    sc = (sc & ~FTM_SC_PS_MASK) | FTM_SC_PS(prescaler);

    // Re-enable counter
    ftm_regs->SC = sc;

    return true;
}

void pwm_enable(pwm_ftm_t ftm, pwm_channel_t channel) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return;
    }

    // Clear output mask bit to enable output
    ftm_regs->OUTMASK &= ~(1 << channel);
}

void pwm_disable(pwm_ftm_t ftm, pwm_channel_t channel) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return;
    }

    // Set output mask bit to disable output
    ftm_regs->OUTMASK |= (1 << channel);
}

uint16_t pwm_get_modulo(pwm_ftm_t ftm) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL) {
        return 0;
    }

    return (uint16_t)ftm_regs->MOD;
}

void pwm_set_pulse_width_us(pwm_ftm_t ftm, pwm_channel_t channel,
                           uint32_t pulse_us) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return;
    }

    // Get current prescaler
    uint8_t ps = (ftm_regs->SC & FTM_SC_PS_MASK);
    uint32_t prescaler_div = (1 << ps);

    // Calculate FTM clock frequency
    uint32_t ftm_clock = clock_get_bus_freq() / prescaler_div;

    // Convert microseconds to timer ticks
    // ticks = (pulse_us * ftm_clock) / 1,000,000
    uint32_t ticks = (pulse_us * ftm_clock) / 1000000;

    // Clamp to modulo value
    uint16_t modulo = ftm_regs->MOD;
    if (ticks > modulo) {
        ticks = modulo;
    }

    ftm_regs->CONTROLS[channel].CnV = (uint16_t)ticks;
}
