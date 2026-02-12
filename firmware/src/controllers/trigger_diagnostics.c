/**
 * @file trigger_diagnostics.c
 * @brief Trigger Diagnostics and Logging Implementation
 *
 * @version 2.4.0
 * @date 2026-02-12
 */

#include "trigger_diagnostics.h"
#include <string.h>
#include <stdlib.h>

// Default thresholds
#define DEFAULT_JITTER_THRESHOLD_US      500    ///< 500µs max jitter
#define DEFAULT_NOISE_MIN_PERIOD_US      100    ///< 100µs min period
#define DEFAULT_RPM_JUMP_THRESHOLD       1000   ///< 1000 RPM max jump

/**
 * @brief Initialize trigger diagnostics
 */
void trigger_diag_init(trigger_diagnostics_t* diag)
{
    if (diag == NULL) {
        return;
    }

    memset(diag, 0, sizeof(trigger_diagnostics_t));

    // Set default thresholds
    diag->jitter_threshold_us = DEFAULT_JITTER_THRESHOLD_US;
    diag->noise_min_period_us = DEFAULT_NOISE_MIN_PERIOD_US;
    diag->rpm_jump_threshold = DEFAULT_RPM_JUMP_THRESHOLD;

    diag->logging_enabled = false;
    diag->min_period_seen_us = 0xFFFF;  // Max value
    diag->max_period_seen_us = 0;
}

/**
 * @brief Process tooth event with error detection
 */
trigger_error_type_t trigger_diag_process_event(trigger_diagnostics_t* diag,
                                                uint16_t tooth_period_us,
                                                uint8_t tooth_index,
                                                uint16_t rpm,
                                                uint32_t timestamp)
{
    if (diag == NULL) {
        return TRIGGER_ERROR_NONE;
    }

    trigger_error_type_t error = TRIGGER_ERROR_NONE;

    // Update period statistics
    if (tooth_period_us < diag->min_period_seen_us) {
        diag->min_period_seen_us = tooth_period_us;
    }
    if (tooth_period_us > diag->max_period_seen_us) {
        diag->max_period_seen_us = tooth_period_us;
    }

    // Error detection
    if (tooth_period_us < diag->noise_min_period_us) {
        // Noise: period too short
        error = TRIGGER_ERROR_NOISE;
        diag->noise_events++;
        diag->total_errors++;

    } else if (diag->last_tooth_period_us > 0) {
        // Check jitter
        uint16_t period_diff = (tooth_period_us > diag->last_tooth_period_us) ?
                              (tooth_period_us - diag->last_tooth_period_us) :
                              (diag->last_tooth_period_us - tooth_period_us);

        if (period_diff > diag->jitter_threshold_us) {
            error = TRIGGER_ERROR_JITTER;
            diag->jitter_events++;
            diag->total_errors++;
        }
    }

    // Log event if enabled
    if (diag->logging_enabled) {
        diag->log[diag->log_index].timestamp_us = timestamp;
        diag->log[diag->log_index].tooth_period_us = tooth_period_us;
        diag->log[diag->log_index].tooth_index = tooth_index;
        diag->log[diag->log_index].error = error;
        diag->log[diag->log_index].rpm = rpm;

        diag->log_index++;
        if (diag->log_index >= 64) {
            diag->log_index = 0;  // Wrap around
        }
    }

    // Update last period for next comparison
    if (error == TRIGGER_ERROR_NONE) {
        diag->last_tooth_period_us = tooth_period_us;
    }

    return error;
}

/**
 * @brief Enable/disable event logging
 */
void trigger_diag_set_logging(trigger_diagnostics_t* diag, bool enable)
{
    if (diag == NULL) {
        return;
    }
    diag->logging_enabled = enable;
}

/**
 * @brief Get logged events
 */
const trigger_log_entry_t* trigger_diag_get_log(const trigger_diagnostics_t* diag,
                                                uint8_t* count)
{
    if (diag == NULL || count == NULL) {
        return NULL;
    }

    *count = 64;  // Full log size
    return diag->log;
}

/**
 * @brief Clear all error counters
 */
void trigger_diag_clear_errors(trigger_diagnostics_t* diag)
{
    if (diag == NULL) {
        return;
    }

    diag->jitter_events = 0;
    diag->noise_events = 0;
    diag->sync_loss_events = 0;
    diag->rpm_jump_events = 0;
    diag->total_errors = 0;
}

/**
 * @brief Get error statistics
 */
void trigger_diag_get_stats(const trigger_diagnostics_t* diag,
                           uint32_t* jitter,
                           uint32_t* noise,
                           uint32_t* sync_loss,
                           uint32_t* rpm_jump)
{
    if (diag == NULL) {
        return;
    }

    if (jitter != NULL) {
        *jitter = diag->jitter_events;
    }
    if (noise != NULL) {
        *noise = diag->noise_events;
    }
    if (sync_loss != NULL) {
        *sync_loss = diag->sync_loss_events;
    }
    if (rpm_jump != NULL) {
        *rpm_jump = diag->rpm_jump_events;
    }
}

/**
 * @brief Set jitter threshold
 */
void trigger_diag_set_jitter_threshold(trigger_diagnostics_t* diag,
                                      uint16_t threshold_us)
{
    if (diag == NULL) {
        return;
    }
    diag->jitter_threshold_us = threshold_us;
}

/**
 * @brief Set noise threshold
 */
void trigger_diag_set_noise_threshold(trigger_diagnostics_t* diag,
                                     uint16_t min_period_us)
{
    if (diag == NULL) {
        return;
    }
    diag->noise_min_period_us = min_period_us;
}

/**
 * @brief Print diagnostics report
 */
void trigger_diag_print_report(const trigger_diagnostics_t* diag)
{
    if (diag == NULL) {
        return;
    }

    // Note: This function would use Serial.print() in Arduino environment
    // Commented out for portability

    /*
    Serial.println("=== Trigger Diagnostics ===");
    Serial.printf("Total errors: %lu\n", diag->total_errors);
    Serial.printf("  Jitter:     %lu\n", diag->jitter_events);
    Serial.printf("  Noise:      %lu\n", diag->noise_events);
    Serial.printf("  Sync loss:  %lu\n", diag->sync_loss_events);
    Serial.printf("  RPM jump:   %lu\n", diag->rpm_jump_events);
    Serial.printf("\nPeriod range: %u - %u µs\n",
                 diag->min_period_seen_us,
                 diag->max_period_seen_us);
    Serial.printf("Logging: %s\n", diag->logging_enabled ? "ON" : "OFF");
    */
}
