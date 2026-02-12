/**
 * @file rpm_calculator_phase3.c
 * @brief Phase 3 Extensions for RPM Calculator
 *
 * Implements acceleration compensation and cranking mode.
 *
 * @version 2.4.0
 * @date 2026-02-12
 */

#include "rpm_calculator.h"

// Phase 3 defaults
#define DEFAULT_CRANKING_THRESHOLD_RPM  400     ///< RPM below = cranking
#define DEFAULT_CRANKING_FILTER_COEFF   0.2f    ///< Faster filter for cranking
#define ACCEL_THRESHOLD_RPM_PER_SEC     50      ///< Min acceleration to detect

/**
 * @brief Update acceleration tracking (internal)
 *
 * Call this from rpm_calculator_on_tooth() to track acceleration.
 */
void rpm_calculator_update_acceleration(rpm_calculator_t* calc,
                                       uint32_t current_time)
{
    if (calc == NULL || !calc->initialized) {
        return;
    }

    // Need at least two RPM readings
    if (calc->prev_rpm_time == 0) {
        calc->prev_rpm = calc->rpm;
        calc->prev_rpm_time = current_time;
        return;
    }

    // Calculate time delta
    uint32_t time_delta_us = current_time - calc->prev_rpm_time;
    if (time_delta_us < 100000) {  // Min 100ms between acceleration updates
        return;
    }

    // Calculate RPM change
    int32_t rpm_delta = (int32_t)calc->rpm - (int32_t)calc->prev_rpm;

    // Convert to RPM/second
    // rpm_per_sec = (rpm_delta * 1,000,000) / time_delta_us
    calc->rpm_acceleration = (rpm_delta * 1000000L) / (int32_t)time_delta_us;

    // Detect acceleration/deceleration
    calc->accelerating = (calc->rpm_acceleration > ACCEL_THRESHOLD_RPM_PER_SEC);
    calc->decelerating = (calc->rpm_acceleration < -ACCEL_THRESHOLD_RPM_PER_SEC);

    // Update previous values
    calc->prev_rpm = calc->rpm;
    calc->prev_rpm_time = current_time;
}

/**
 * @brief Update cranking mode (internal)
 *
 * Call this from rpm_calculator_on_tooth() to adjust filtering.
 */
void rpm_calculator_update_cranking_mode(rpm_calculator_t* calc)
{
    if (calc == NULL) {
        return;
    }

    // Check if in cranking RPM range
    if (calc->rpm < calc->cranking_rpm_threshold) {
        calc->cranking = true;
    } else {
        calc->cranking = false;
    }
}

/**
 * @brief Get appropriate filter coefficient
 *
 * Returns faster coefficient during cranking for quicker response.
 */
float rpm_calculator_get_active_filter_coeff(const rpm_calculator_t* calc)
{
    if (calc == NULL) {
        return 0.05f;
    }

    if (calc->cranking) {
        return calc->cranking_filter_coeff;
    }

    return calc->filter_coefficient;
}

//=============================================================================
// Phase 3 Public API
//=============================================================================

/**
 * @brief Get RPM acceleration
 */
int32_t rpm_calculator_get_acceleration(const rpm_calculator_t* calc)
{
    if (calc == NULL) {
        return 0;
    }
    return calc->rpm_acceleration;
}

/**
 * @brief Check if engine is accelerating
 */
bool rpm_calculator_is_accelerating(const rpm_calculator_t* calc)
{
    if (calc == NULL) {
        return false;
    }
    return calc->accelerating;
}

/**
 * @brief Check if engine is decelerating
 */
bool rpm_calculator_is_decelerating(const rpm_calculator_t* calc)
{
    if (calc == NULL) {
        return false;
    }
    return calc->decelerating;
}

/**
 * @brief Check if engine is cranking
 */
bool rpm_calculator_is_cranking(const rpm_calculator_t* calc)
{
    if (calc == NULL) {
        return false;
    }
    return calc->cranking;
}

/**
 * @brief Set cranking RPM threshold
 */
void rpm_calculator_set_cranking_threshold(rpm_calculator_t* calc,
                                          uint16_t threshold_rpm)
{
    if (calc == NULL) {
        return;
    }
    calc->cranking_rpm_threshold = threshold_rpm;
}

/**
 * @brief Set cranking filter coefficient
 */
void rpm_calculator_set_cranking_filter(rpm_calculator_t* calc,
                                       float coefficient)
{
    if (calc == NULL) {
        return;
    }

    // Clamp to valid range
    if (coefficient < 0.0f) {
        coefficient = 0.0f;
    } else if (coefficient > 1.0f) {
        coefficient = 1.0f;
    }

    calc->cranking_filter_coeff = coefficient;
}
