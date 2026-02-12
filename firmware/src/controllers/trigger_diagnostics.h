/**
 * @file trigger_diagnostics.h
 * @brief Trigger Diagnostics and Logging for Teensy 3.5
 *
 * Implements advanced error detection and trigger event logging.
 * Based on rusEFI diagnostics systems.
 *
 * @version 2.4.0
 * @date 2026-02-12
 *
 * Based on rusEFI:
 * - firmware/controllers/trigger/trigger_decoder.cpp
 * - Advanced error detection and logging
 */

#ifndef TRIGGER_DIAGNOSTICS_H
#define TRIGGER_DIAGNOSTICS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Trigger error types
 */
typedef enum {
    TRIGGER_ERROR_NONE = 0,
    TRIGGER_ERROR_JITTER,           ///< Excessive timing jitter
    TRIGGER_ERROR_NOISE,            ///< Noise/false triggers
    TRIGGER_ERROR_MISSING_TOOTH,    ///< Unexpected missing tooth
    TRIGGER_ERROR_EXTRA_TOOTH,      ///< Extra tooth detected
    TRIGGER_ERROR_SYNC_LOSS,        ///< Lost synchronization
    TRIGGER_ERROR_RPM_JUMP,         ///< Impossible RPM change
} trigger_error_type_t;

/**
 * @brief Trigger event log entry
 */
typedef struct {
    uint32_t timestamp_us;          ///< Event timestamp
    uint16_t tooth_period_us;       ///< Tooth period
    uint8_t tooth_index;            ///< Tooth number
    trigger_error_type_t error;     ///< Error type (if any)
    uint16_t rpm;                   ///< RPM at event
} trigger_log_entry_t;

/**
 * @brief Trigger diagnostics structure
 */
typedef struct {
    // Error counters
    uint32_t jitter_events;         ///< Jitter errors
    uint32_t noise_events;          ///< Noise errors
    uint32_t sync_loss_events;      ///< Sync losses
    uint32_t rpm_jump_events;       ///< RPM jumps

    // Error thresholds
    uint16_t jitter_threshold_us;   ///< Max acceptable jitter (µs)
    uint16_t noise_min_period_us;   ///< Min valid tooth period (µs)
    uint16_t rpm_jump_threshold;    ///< Max RPM change per tooth

    // Event logging
    trigger_log_entry_t log[64];    ///< Circular log buffer
    uint8_t log_index;              ///< Current log position
    bool logging_enabled;           ///< Event logging active

    // Real-time monitoring
    uint16_t last_tooth_period_us;  ///< Last valid tooth period
    uint16_t min_period_seen_us;    ///< Minimum period (max RPM)
    uint16_t max_period_seen_us;    ///< Maximum period (min RPM)
    uint32_t total_errors;          ///< Total error count

} trigger_diagnostics_t;

/**
 * @brief Initialize trigger diagnostics
 *
 * @param diag Pointer to diagnostics structure
 */
void trigger_diag_init(trigger_diagnostics_t* diag);

/**
 * @brief Process tooth event with error detection
 *
 * Checks for errors and logs event if enabled.
 *
 * @param diag Pointer to diagnostics structure
 * @param tooth_period_us Tooth period in microseconds
 * @param tooth_index Tooth number
 * @param rpm Current RPM
 * @param timestamp Event timestamp
 * @return Error type (TRIGGER_ERROR_NONE if OK)
 */
trigger_error_type_t trigger_diag_process_event(trigger_diagnostics_t* diag,
                                                uint16_t tooth_period_us,
                                                uint8_t tooth_index,
                                                uint16_t rpm,
                                                uint32_t timestamp);

/**
 * @brief Enable/disable event logging
 *
 * @param diag Pointer to diagnostics structure
 * @param enable true to enable logging, false to disable
 */
void trigger_diag_set_logging(trigger_diagnostics_t* diag, bool enable);

/**
 * @brief Get logged events
 *
 * Returns pointer to circular log buffer.
 * Use log_index to find most recent entry.
 *
 * @param diag Pointer to diagnostics structure
 * @param count Output: number of entries (max 64)
 * @return Pointer to log array
 */
const trigger_log_entry_t* trigger_diag_get_log(const trigger_diagnostics_t* diag,
                                                uint8_t* count);

/**
 * @brief Clear all error counters
 *
 * @param diag Pointer to diagnostics structure
 */
void trigger_diag_clear_errors(trigger_diagnostics_t* diag);

/**
 * @brief Get error statistics
 *
 * @param diag Pointer to diagnostics structure
 * @param jitter Output: jitter error count
 * @param noise Output: noise error count
 * @param sync_loss Output: sync loss count
 * @param rpm_jump Output: RPM jump count
 */
void trigger_diag_get_stats(const trigger_diagnostics_t* diag,
                           uint32_t* jitter,
                           uint32_t* noise,
                           uint32_t* sync_loss,
                           uint32_t* rpm_jump);

/**
 * @brief Set jitter threshold
 *
 * @param diag Pointer to diagnostics structure
 * @param threshold_us Maximum acceptable jitter in microseconds
 */
void trigger_diag_set_jitter_threshold(trigger_diagnostics_t* diag,
                                      uint16_t threshold_us);

/**
 * @brief Set noise threshold
 *
 * Periods shorter than this are considered noise.
 *
 * @param diag Pointer to diagnostics structure
 * @param min_period_us Minimum valid tooth period
 */
void trigger_diag_set_noise_threshold(trigger_diagnostics_t* diag,
                                     uint16_t min_period_us);

/**
 * @brief Print diagnostics report
 *
 * Outputs diagnostic information to Serial.
 *
 * @param diag Pointer to diagnostics structure
 */
void trigger_diag_print_report(const trigger_diagnostics_t* diag);

#ifdef __cplusplus
}
#endif

#endif // TRIGGER_DIAGNOSTICS_H
