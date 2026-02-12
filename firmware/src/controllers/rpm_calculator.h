/**
 * @file rpm_calculator.h
 * @brief rusEFI-compatible RPM Calculator for Teensy 3.5 (MK64FX512)
 *
 * Implements RPM calculation with exponential moving average filtering,
 * acceleration compensation, and cranking mode support.
 * Based on rusEFI's rpm_calculator.cpp algorithm.
 *
 * @version 2.4.0 (Phase 3: Advanced Features)
 * @date 2026-02-12
 *
 * Based on rusEFI:
 * - firmware/controllers/rpm_calculator.cpp
 * - firmware/controllers/rpm_calculator.h
 * - Class: RpmCalculator
 *
 * References:
 * - https://github.com/rusefi/rusefi/blob/master/firmware/controllers/rpm_calculator.cpp
 * - https://rusefi.com/docs/html/class_rpm_calculator.html
 */

#ifndef RPM_CALCULATOR_H
#define RPM_CALCULATOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RPM calculator structure (rusEFI-compatible)
 *
 * Calculates engine RPM with exponential moving average filtering,
 * acceleration compensation, and cranking mode support.
 * Compatible with rusEFI RpmCalculator (Phase 3).
 */
typedef struct {
    // RPM values
    uint16_t rpm;                     ///< Current filtered RPM
    uint16_t instant_rpm;             ///< Instantaneous RPM (unfiltered)

    // Revolution tracking
    uint32_t last_revolution_time;    ///< Timestamp of last complete revolution (µs)
    uint32_t revolution_period;       ///< Period of last complete revolution (µs)
    uint32_t revolution_counter;      ///< Total number of revolutions

    // Filtering parameters
    float filter_coefficient;         ///< Smoothing factor (default: 0.05)
                                      ///< 0.05 = 5% new, 95% old (rusEFI default)

    // Timeout detection
    uint32_t timeout_threshold_us;    ///< Timeout before considering stopped (µs)
    uint32_t last_update_time;        ///< Timestamp of last RPM update (µs)

    // State flags
    bool stopped;                     ///< Engine stopped flag
    bool initialized;                 ///< Calculator initialized flag
    bool cranking;                    ///< Cranking mode active (Phase 3)

    // Acceleration tracking (Phase 3)
    int32_t rpm_acceleration;         ///< RPM/s (positive = accelerating)
    uint16_t prev_rpm;                ///< Previous RPM for acceleration calc
    uint32_t prev_rpm_time;           ///< Time of previous RPM measurement
    bool accelerating;                ///< Engine is accelerating
    bool decelerating;                ///< Engine is decelerating

    // Cranking mode parameters (Phase 3)
    uint16_t cranking_rpm_threshold;  ///< RPM below which cranking mode is active
    float cranking_filter_coeff;      ///< Faster filter during cranking (0.2 typical)

} rpm_calculator_t;

/**
 * @brief Initialize RPM calculator
 *
 * Sets up the calculator with default parameters.
 * Default filter coefficient: 0.05 (rusEFI standard)
 * Default timeout: 1 second
 *
 * @param calc Pointer to RPM calculator structure
 */
void rpm_calculator_init(rpm_calculator_t* calc);

/**
 * @brief Process tooth event and update RPM (rusEFI algorithm)
 *
 * Called on each crank tooth event with the tooth period.
 * Implements rusEFI's instantaneous RPM calculation with exponential filtering:
 *
 *   instant_rpm = 60 * 1,000,000 / (tooth_period * teeth_per_rev)
 *   filtered_rpm = instant_rpm * alpha + old_rpm * (1 - alpha)
 *
 * @param calc Pointer to RPM calculator structure
 * @param period_us Tooth period in microseconds
 * @param teeth_per_rev Total teeth per revolution (e.g., 36 for 36-1)
 * @param current_time Current timestamp in microseconds
 */
void rpm_calculator_on_tooth(rpm_calculator_t* calc,
                            uint32_t period_us,
                            uint16_t teeth_per_rev,
                            uint32_t current_time);

/**
 * @brief Process complete revolution event (rusEFI algorithm)
 *
 * Called once per complete engine revolution (e.g., at sync point).
 * Calculates RPM based on full revolution period for improved accuracy.
 *
 * @param calc Pointer to RPM calculator structure
 * @param revolution_period_us Period of complete revolution in microseconds
 * @param current_time Current timestamp in microseconds
 */
void rpm_calculator_on_revolution(rpm_calculator_t* calc,
                                 uint32_t revolution_period_us,
                                 uint32_t current_time);

/**
 * @brief Get current filtered RPM
 *
 * Returns the filtered RPM value, or 0 if engine is stopped/timed out.
 *
 * @param calc Pointer to RPM calculator structure
 * @return Current RPM (0 if stopped)
 */
uint16_t rpm_calculator_get_rpm(const rpm_calculator_t* calc);

/**
 * @brief Get instantaneous (unfiltered) RPM
 *
 * Returns the instantaneous RPM without filtering.
 * Useful for diagnostics and quick response applications.
 *
 * @param calc Pointer to RPM calculator structure
 * @return Instantaneous RPM
 */
uint16_t rpm_calculator_get_instant_rpm(const rpm_calculator_t* calc);

/**
 * @brief Check if engine is running
 *
 * Returns true if RPM updates are recent (within timeout threshold).
 *
 * @param calc Pointer to RPM calculator structure
 * @param current_time Current timestamp in microseconds
 * @return true if engine is running, false if stopped/timed out
 */
bool rpm_calculator_is_running(const rpm_calculator_t* calc,
                               uint32_t current_time);

/**
 * @brief Set filter coefficient (rusEFI smoothing)
 *
 * Configures the exponential moving average filter strength.
 * Lower values = more smoothing, slower response
 * Higher values = less smoothing, faster response
 *
 * rusEFI defaults:
 *   - Normal: 0.05 (5% new, 95% old)
 *   - Fast response: 0.1
 *   - Heavy smoothing: 0.02
 *
 * @param calc Pointer to RPM calculator structure
 * @param coefficient Filter coefficient (0.0 to 1.0)
 */
void rpm_calculator_set_filter_coefficient(rpm_calculator_t* calc,
                                          float coefficient);

/**
 * @brief Set timeout threshold
 *
 * Configures how long to wait before considering the engine stopped.
 * Default: 1,000,000 µs (1 second)
 *
 * @param calc Pointer to RPM calculator structure
 * @param timeout_us Timeout in microseconds
 */
void rpm_calculator_set_timeout(rpm_calculator_t* calc,
                               uint32_t timeout_us);

/**
 * @brief Reset RPM calculator
 *
 * Clears all state and resets to initial values.
 * Use this when engine stops or loses sync.
 *
 * @param calc Pointer to RPM calculator structure
 */
void rpm_calculator_reset(rpm_calculator_t* calc);

/**
 * @brief Get revolution counter
 *
 * Returns the total number of complete engine revolutions detected.
 *
 * @param calc Pointer to RPM calculator structure
 * @return Total revolution count
 */
uint32_t rpm_calculator_get_revolution_count(const rpm_calculator_t* calc);

/**
 * @brief Check and update timeout status
 *
 * Internal function to check if RPM calculation has timed out.
 * Automatically called by rpm_calculator_is_running().
 *
 * @param calc Pointer to RPM calculator structure
 * @param current_time Current timestamp in microseconds
 */
void rpm_calculator_check_timeout(rpm_calculator_t* calc,
                                 uint32_t current_time);

//=============================================================================
// Phase 3: Advanced Features
//=============================================================================

/**
 * @brief Get RPM acceleration (Phase 3)
 *
 * Returns rate of RPM change in RPM/second.
 * Positive = accelerating, Negative = decelerating.
 *
 * @param calc Pointer to RPM calculator structure
 * @return RPM acceleration (RPM/s)
 */
int32_t rpm_calculator_get_acceleration(const rpm_calculator_t* calc);

/**
 * @brief Check if engine is accelerating (Phase 3)
 *
 * @param calc Pointer to RPM calculator structure
 * @return true if accelerating, false otherwise
 */
bool rpm_calculator_is_accelerating(const rpm_calculator_t* calc);

/**
 * @brief Check if engine is decelerating (Phase 3)
 *
 * @param calc Pointer to RPM calculator structure
 * @return true if decelerating, false otherwise
 */
bool rpm_calculator_is_decelerating(const rpm_calculator_t* calc);

/**
 * @brief Check if engine is cranking (Phase 3)
 *
 * Cranking mode uses faster filtering for quicker sync during startup.
 *
 * @param calc Pointer to RPM calculator structure
 * @return true if cranking (RPM below threshold), false otherwise
 */
bool rpm_calculator_is_cranking(const rpm_calculator_t* calc);

/**
 * @brief Set cranking RPM threshold (Phase 3)
 *
 * Default: 400 RPM
 *
 * @param calc Pointer to RPM calculator structure
 * @param threshold_rpm RPM below which cranking mode is active
 */
void rpm_calculator_set_cranking_threshold(rpm_calculator_t* calc,
                                          uint16_t threshold_rpm);

/**
 * @brief Set cranking filter coefficient (Phase 3)
 *
 * Faster filtering during cranking for quicker response.
 * Default: 0.2 (20% new, 80% old) vs normal 0.05
 *
 * @param calc Pointer to RPM calculator structure
 * @param coefficient Filter coefficient for cranking mode (0.0-1.0)
 */
void rpm_calculator_set_cranking_filter(rpm_calculator_t* calc,
                                       float coefficient);

#ifdef __cplusplus
}
#endif

#endif // RPM_CALCULATOR_H
