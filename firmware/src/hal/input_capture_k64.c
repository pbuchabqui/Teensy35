/**
 * @file input_capture_k64.c
 * @brief Input Capture driver implementation for Kinetis K64
 * @version 1.0.0
 * @date 2026-02-10
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#include "input_capture_k64.h"
#include "clock_k64.h"

//=============================================================================
// Private Variables
//=============================================================================

// Callback storage for each FTM channel
static ic_callback_t ic_callbacks[4][8] = {{NULL}};  // 4 FTMs, 8 channels each

// Last capture values for period calculation
static uint32_t last_capture[4][8] = {{0}};

// Engine position tracking
static engine_position_t engine_pos = {0};
static uint16_t crank_teeth_per_rev = 36;
static uint16_t crank_missing_teeth = 1;

//=============================================================================
// Private Helper Functions
//=============================================================================

/**
 * @brief Get FTM register pointer (from pwm_k64.c)
 */
extern FTM_Type* pwm_get_regs(pwm_ftm_t ftm);  // Reuse from PWM driver

/**
 * @brief Convert FTM ticks to microseconds
 */
static uint32_t ticks_to_us(uint32_t ticks, pwm_ftm_t ftm) {
    // Get FTM prescaler from SC register
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL) return 0;

    uint8_t ps = (ftm_regs->SC & 0x07);
    uint32_t prescaler_div = (1 << ps);

    // Calculate period: ticks * (1 / (bus_clock / prescaler))
    uint32_t bus_clock = clock_get_bus_freq();
    uint64_t period_us = ((uint64_t)ticks * prescaler_div * 1000000) / bus_clock;

    return (uint32_t)period_us;
}

//=============================================================================
// Public Functions
//=============================================================================

bool ic_init(pwm_ftm_t ftm, pwm_channel_t channel, const ic_config_t* config) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7 || config == NULL) {
        return false;
    }

    // Configure FTM channel for input capture
    uint32_t cnsc = 0;

    // Set edge detection mode
    switch (config->edge) {
        case IC_EDGE_RISING:
            cnsc = 0x04;  // ELSA = 1, ELSB = 0 (rising edge)
            break;
        case IC_EDGE_FALLING:
            cnsc = 0x08;  // ELSA = 0, ELSB = 1 (falling edge)
            break;
        case IC_EDGE_BOTH:
            cnsc = 0x0C;  // ELSA = 1, ELSB = 1 (both edges)
            break;
    }

    // Enable interrupt if requested
    if (config->enable_interrupt) {
        cnsc |= 0x40;  // CHIE = 1
    }

    ftm_regs->CONTROLS[channel].CnSC = cnsc;

    // Enable input filter if requested
    if (config->enable_filter) {
        ftm_regs->FILTER |= (0x0F << (channel * 4));  // Max filter value
    }

    return true;
}

void ic_register_callback(pwm_ftm_t ftm, pwm_channel_t channel,
                          ic_callback_t callback) {
    if (ftm <= PWM_FTM3 && channel <= PWM_CHANNEL_7) {
        ic_callbacks[ftm][channel] = callback;
    }
}

uint32_t ic_get_capture_value(pwm_ftm_t ftm, pwm_channel_t channel) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return 0;
    }

    return ftm_regs->CONTROLS[channel].CnV;
}

uint32_t ic_get_period_us(pwm_ftm_t ftm, pwm_channel_t channel) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return 0;
    }

    uint32_t current = ftm_regs->CONTROLS[channel].CnV;
    uint32_t previous = last_capture[ftm][channel];

    // Calculate period (handle counter overflow)
    uint32_t period_ticks;
    if (current >= previous) {
        period_ticks = current - previous;
    } else {
        // Counter wrapped around
        uint32_t mod = ftm_regs->MOD;
        period_ticks = (mod - previous) + current;
    }

    // Update last capture value
    last_capture[ftm][channel] = current;

    // Convert to microseconds
    return ticks_to_us(period_ticks, ftm);
}

uint16_t ic_calculate_rpm(uint32_t period_us, uint16_t teeth_per_rev) {
    if (period_us == 0 || teeth_per_rev == 0) {
        return 0;
    }

    // RPM = (60,000,000 µs/min) / (period_us * teeth_per_rev)
    uint64_t rpm = (60000000ULL) / ((uint64_t)period_us * teeth_per_rev);

    return (uint16_t)(rpm > 65535 ? 65535 : rpm);
}

void ic_enable(pwm_ftm_t ftm, pwm_channel_t channel) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return;
    }

    // Clear output mask to enable channel
    ftm_regs->OUTMASK &= ~(1 << channel);
}

void ic_disable(pwm_ftm_t ftm, pwm_channel_t channel) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return;
    }

    // Set output mask to disable channel
    ftm_regs->OUTMASK |= (1 << channel);
}

bool ic_event_occurred(pwm_ftm_t ftm, pwm_channel_t channel) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return false;
    }

    return (ftm_regs->CONTROLS[channel].CnSC & 0x80) != 0;  // CHF flag
}

void ic_clear_event(pwm_ftm_t ftm, pwm_channel_t channel) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return;
    }

    // Clear CHF flag by reading CnSC then writing 0 to CHF
    (void)ftm_regs->CONTROLS[channel].CnSC;
    ftm_regs->CONTROLS[channel].CnSC &= ~0x80;
}

//=============================================================================
// High-Level Crank/Cam Functions
//=============================================================================

/**
 * @brief Crank sensor interrupt callback
 */
static void crank_sensor_callback(uint32_t timestamp) {
    // Update engine position
    engine_pos.last_tooth_time = timestamp;
    engine_pos.tooth_count++;

    // Calculate period between teeth
    uint32_t period_us = ic_get_period_us(PWM_FTM0, PWM_CHANNEL_4);
    engine_pos.tooth_period = period_us;

    // Calculate RPM
    engine_pos.rpm = ic_calculate_rpm(period_us, crank_teeth_per_rev);

    // Reset tooth count after full revolution
    if (engine_pos.tooth_count >= crank_teeth_per_rev) {
        engine_pos.tooth_count = 0;
    }

    // Check for sync (detect missing teeth)
    // TODO: Implement missing tooth detection for sync

    if (engine_pos.rpm > 100 && engine_pos.rpm < 10000) {
        engine_pos.sync_locked = true;
    }
}

void crank_sensor_init(uint16_t teeth_per_rev, uint16_t missing_teeth,
                       sensor_type_t sensor_type) {
    crank_teeth_per_rev = teeth_per_rev;
    crank_missing_teeth = missing_teeth;

    // Configure input capture for crank sensor
    // Typically on FTM0_CH4 (Pin 33 on Teensy 3.5)
    ic_config_t ic_cfg = {
        .edge = (sensor_type == SENSOR_TYPE_VR) ? IC_EDGE_BOTH : IC_EDGE_RISING,
        .enable_interrupt = true,
        .enable_filter = (sensor_type == SENSOR_TYPE_VR),  // VR needs filtering
    };

    ic_init(PWM_FTM0, PWM_CHANNEL_4, &ic_cfg);
    ic_register_callback(PWM_FTM0, PWM_CHANNEL_4, crank_sensor_callback);
    ic_enable(PWM_FTM0, PWM_CHANNEL_4);

    // Reset engine position
    engine_pos.tooth_count = 0;
    engine_pos.rpm = 0;
    engine_pos.sync_locked = false;
}

void cam_sensor_init(uint16_t teeth_per_rev, sensor_type_t sensor_type) {
    // Configure input capture for cam sensor
    // Typically on FTM0_CH5 (Pin 34 on Teensy 3.5)
    ic_config_t ic_cfg = {
        .edge = IC_EDGE_RISING,
        .enable_interrupt = true,
        .enable_filter = false,
    };

    ic_init(PWM_FTM0, PWM_CHANNEL_5, &ic_cfg);
    // Callback for cam sensor would be registered here
    ic_enable(PWM_FTM0, PWM_CHANNEL_5);
}

engine_position_t* get_engine_position(void) {
    return &engine_pos;
}

uint16_t get_engine_rpm(void) {
    return engine_pos.sync_locked ? engine_pos.rpm : 0;
}

bool is_engine_synced(void) {
    return engine_pos.sync_locked;
}

//=============================================================================
// Interrupt Handlers (shared with PWM, need to check mode)
//=============================================================================

// Note: FTM interrupt handlers would be added here to dispatch to
// registered callbacks based on channel interrupt flags
