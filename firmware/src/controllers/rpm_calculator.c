/**
 * @file rpm_calculator.c
 * @brief rusEFI-compatible RPM Calculator Implementation
 *
 * Implements RPM calculation with exponential moving average filtering
 * based on rusEFI's rpm_calculator.cpp algorithm.
 *
 * @version 2.3.0
 * @date 2026-02-12
 */

#include "rpm_calculator.h"
#include <string.h>

// Default parameters (rusEFI-compatible)
#define DEFAULT_FILTER_COEFFICIENT   0.05f      ///< 5% new, 95% old (rusEFI)
#define DEFAULT_TIMEOUT_US           1000000    ///< 1 second timeout
#define MIN_RPM_PERIOD_US            1000       ///< Prevent overflow at very high RPM

// Microseconds per minute for RPM calculation
#define US_PER_MINUTE                60000000ULL

/**
 * @brief Initialize RPM calculator
 */
void rpm_calculator_init(rpm_calculator_t* calc)
{
    if (calc == NULL) {
        return;
    }

    memset(calc, 0, sizeof(rpm_calculator_t));

    // Set default parameters (rusEFI-compatible)
    calc->filter_coefficient = DEFAULT_FILTER_COEFFICIENT;
    calc->timeout_threshold_us = DEFAULT_TIMEOUT_US;
    calc->stopped = true;
    calc->initialized = true;
}

/**
 * @brief Process tooth event and update RPM (rusEFI algorithm)
 *
 * Core rusEFI RPM calculation:
 *   1. Calculate instantaneous RPM from tooth period
 *   2. Apply exponential moving average filter
 *   3. Update timestamp for timeout detection
 */
void rpm_calculator_on_tooth(rpm_calculator_t* calc,
                            uint32_t period_us,
                            uint16_t teeth_per_rev,
                            uint32_t current_time)
{
    if (calc == NULL || period_us < MIN_RPM_PERIOD_US || teeth_per_rev == 0) {
        return;
    }

    // rusEFI instantaneous RPM calculation
    // instant_rpm = 60 * 1,000,000 / (tooth_period * teeth_per_rev)
    uint64_t instant_rpm_calc = US_PER_MINUTE /
                                ((uint64_t)period_us * teeth_per_rev);

    // Clamp to uint16_t range
    if (instant_rpm_calc > 65535) {
        instant_rpm_calc = 65535;
    }

    calc->instant_rpm = (uint16_t)instant_rpm_calc;

    // rusEFI exponential moving average filter
    // new_rpm = instant_rpm * alpha + old_rpm * (1 - alpha)
    // where alpha = filter_coefficient
    if (calc->rpm == 0) {
        // First reading - no filtering needed
        calc->rpm = calc->instant_rpm;
    } else {
        // Apply exponential filter
        float new_rpm = (float)calc->instant_rpm * calc->filter_coefficient +
                       (float)calc->rpm * (1.0f - calc->filter_coefficient);
        calc->rpm = (uint16_t)new_rpm;
    }

    // Update state
    calc->last_update_time = current_time;
    calc->stopped = false;
}

/**
 * @brief Process complete revolution event (rusEFI algorithm)
 *
 * Provides more accurate RPM calculation over full revolution.
 * Used at sync point for reference RPM value.
 */
void rpm_calculator_on_revolution(rpm_calculator_t* calc,
                                 uint32_t revolution_period_us,
                                 uint32_t current_time)
{
    if (calc == NULL || revolution_period_us < MIN_RPM_PERIOD_US) {
        return;
    }

    calc->revolution_period = revolution_period_us;
    calc->last_revolution_time = current_time;
    calc->revolution_counter++;

    // Calculate RPM from full revolution
    // rpm = 60,000,000 / revolution_period_us
    uint64_t revolution_rpm = US_PER_MINUTE / revolution_period_us;

    if (revolution_rpm > 65535) {
        revolution_rpm = 65535;
    }

    // Update instant RPM from revolution measurement
    calc->instant_rpm = (uint16_t)revolution_rpm;

    // Apply filter to main RPM value
    if (calc->rpm == 0) {
        calc->rpm = calc->instant_rpm;
    } else {
        float new_rpm = (float)calc->instant_rpm * calc->filter_coefficient +
                       (float)calc->rpm * (1.0f - calc->filter_coefficient);
        calc->rpm = (uint16_t)new_rpm;
    }

    // Update state
    calc->last_update_time = current_time;
    calc->stopped = false;
}

/**
 * @brief Get current filtered RPM
 */
uint16_t rpm_calculator_get_rpm(const rpm_calculator_t* calc)
{
    if (calc == NULL || calc->stopped) {
        return 0;
    }

    return calc->rpm;
}

/**
 * @brief Get instantaneous (unfiltered) RPM
 */
uint16_t rpm_calculator_get_instant_rpm(const rpm_calculator_t* calc)
{
    if (calc == NULL || calc->stopped) {
        return 0;
    }

    return calc->instant_rpm;
}

/**
 * @brief Check if engine is running
 */
bool rpm_calculator_is_running(const rpm_calculator_t* calc,
                               uint32_t current_time)
{
    if (calc == NULL || !calc->initialized) {
        return false;
    }

    // Check timeout
    if (calc->last_update_time > 0) {
        uint32_t elapsed = current_time - calc->last_update_time;
        if (elapsed > calc->timeout_threshold_us) {
            return false;  // Timed out
        }
    } else {
        return false;  // Never updated
    }

    return !calc->stopped && calc->rpm > 0;
}

/**
 * @brief Set filter coefficient
 */
void rpm_calculator_set_filter_coefficient(rpm_calculator_t* calc,
                                          float coefficient)
{
    if (calc == NULL) {
        return;
    }

    // Clamp to valid range [0.0, 1.0]
    if (coefficient < 0.0f) {
        coefficient = 0.0f;
    } else if (coefficient > 1.0f) {
        coefficient = 1.0f;
    }

    calc->filter_coefficient = coefficient;
}

/**
 * @brief Set timeout threshold
 */
void rpm_calculator_set_timeout(rpm_calculator_t* calc,
                               uint32_t timeout_us)
{
    if (calc == NULL) {
        return;
    }

    calc->timeout_threshold_us = timeout_us;
}

/**
 * @brief Reset RPM calculator
 */
void rpm_calculator_reset(rpm_calculator_t* calc)
{
    if (calc == NULL) {
        return;
    }

    // Preserve configuration settings
    float saved_filter = calc->filter_coefficient;
    uint32_t saved_timeout = calc->timeout_threshold_us;
    bool was_initialized = calc->initialized;

    // Clear all state
    memset(calc, 0, sizeof(rpm_calculator_t));

    // Restore configuration
    calc->filter_coefficient = saved_filter;
    calc->timeout_threshold_us = saved_timeout;
    calc->initialized = was_initialized;
    calc->stopped = true;
}

/**
 * @brief Get revolution counter
 */
uint32_t rpm_calculator_get_revolution_count(const rpm_calculator_t* calc)
{
    if (calc == NULL) {
        return 0;
    }

    return calc->revolution_counter;
}

/**
 * @brief Check and update timeout status
 */
void rpm_calculator_check_timeout(rpm_calculator_t* calc,
                                 uint32_t current_time)
{
    if (calc == NULL) {
        return;
    }

    // Check if we've timed out
    if (calc->last_update_time > 0) {
        uint32_t elapsed = current_time - calc->last_update_time;

        if (elapsed > calc->timeout_threshold_us) {
            // Engine stopped - reset RPM
            calc->rpm = 0;
            calc->instant_rpm = 0;
            calc->stopped = true;
        }
    }
}
