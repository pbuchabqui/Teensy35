/**
 * @file vvt_tracker_k64.h
 * @brief Variable Valve Timing (VVT) Position Tracker for Teensy 3.5
 *
 * Implements VVT position tracking and phase calculation.
 * Compatible with rusEFI VVT control algorithms.
 *
 * @version 2.4.0
 * @date 2026-02-12
 *
 * Based on rusEFI:
 * - firmware/controllers/actuators/vvt.cpp
 * - firmware/controllers/actuators/vvt.h
 * - VVT position sensing and control
 *
 * References:
 * - https://rusefi.com/docs/html/vvt_8cpp.html
 * - VVT sensor processing for cam phasing
 */

#ifndef VVT_TRACKER_K64_H
#define VVT_TRACKER_K64_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VVT position structure
 *
 * Tracks Variable Valve Timing position for intake/exhaust cams.
 */
typedef struct {
    // Position tracking
    int16_t position_degrees;       ///< Current VVT position in degrees (-50 to +50)
    int16_t target_position;        ///< Target VVT position (from ECU control)

    // Sensor measurements
    uint32_t vvt_tooth_period_us;   ///< VVT sensor tooth period
    uint32_t last_vvt_event_time;   ///< Last VVT sensor event timestamp
    uint8_t vvt_tooth_count;        ///< VVT teeth seen this revolution

    // Crank correlation
    uint8_t crank_tooth_at_vvt;     ///< Crank tooth when VVT event occurred
    int16_t phase_offset;           ///< Phase offset from base position

    // Synchronization
    bool vvt_synced;                ///< VVT position is known
    uint32_t sync_count;            ///< Successful VVT syncs
    uint32_t sync_loss_count;       ///< Times VVT sync was lost

    // Statistics
    uint32_t vvt_events_total;      ///< Total VVT sensor events
    int16_t min_position;           ///< Minimum position seen
    int16_t max_position;           ///< Maximum position seen

    // Configuration
    uint8_t vvt_teeth_per_rev;      ///< VVT teeth per revolution (e.g., 4)
    int16_t vvt_offset_degrees;     ///< Calibration offset

} vvt_tracker_t;

/**
 * @brief Initialize VVT tracker
 *
 * @param vvt Pointer to VVT tracker structure
 * @param teeth_per_rev VVT teeth per revolution (typically 1-4)
 * @param offset_degrees Calibration offset in degrees
 */
void vvt_tracker_init(vvt_tracker_t* vvt,
                     uint8_t teeth_per_rev,
                     int16_t offset_degrees);

/**
 * @brief Process VVT sensor event
 *
 * Called on VVT sensor edge (interrupt).
 * Calculates VVT position relative to crank position.
 *
 * Algorithm:
 * 1. Measure time between VVT events
 * 2. Correlate with crank position
 * 3. Calculate phase offset
 * 4. Convert to degrees of cam advance/retard
 *
 * @param vvt Pointer to VVT tracker
 * @param crank_tooth Current crank tooth index
 * @param crank_angle Current crank angle (degrees)
 * @param timestamp Current timestamp (microseconds)
 */
void vvt_tracker_process_event(vvt_tracker_t* vvt,
                               uint8_t crank_tooth,
                               uint16_t crank_angle,
                               uint32_t timestamp);

/**
 * @brief Get current VVT position
 *
 * @param vvt Pointer to VVT tracker
 * @return VVT position in degrees (negative = retarded, positive = advanced)
 */
int16_t vvt_tracker_get_position(const vvt_tracker_t* vvt);

/**
 * @brief Check if VVT is synchronized
 *
 * @param vvt Pointer to VVT tracker
 * @return true if VVT position is known, false otherwise
 */
bool vvt_tracker_is_synced(const vvt_tracker_t* vvt);

/**
 * @brief Set VVT target position
 *
 * Sets target position for VVT control.
 * Actual position control is done by external controller.
 *
 * @param vvt Pointer to VVT tracker
 * @param target_degrees Target position in degrees
 */
void vvt_tracker_set_target(vvt_tracker_t* vvt, int16_t target_degrees);

/**
 * @brief Get VVT error (target - actual)
 *
 * @param vvt Pointer to VVT tracker
 * @return Position error in degrees
 */
int16_t vvt_tracker_get_error(const vvt_tracker_t* vvt);

/**
 * @brief Reset VVT tracker
 *
 * Clears synchronization. Use when crank sync is lost.
 *
 * @param vvt Pointer to VVT tracker
 */
void vvt_tracker_reset(vvt_tracker_t* vvt);

/**
 * @brief Get VVT statistics
 *
 * @param vvt Pointer to VVT tracker
 * @param sync_count Output: successful syncs
 * @param sync_loss_count Output: sync losses
 * @param events_total Output: total events
 */
void vvt_tracker_get_stats(const vvt_tracker_t* vvt,
                          uint32_t* sync_count,
                          uint32_t* sync_loss_count,
                          uint32_t* events_total);

#ifdef __cplusplus
}
#endif

#endif // VVT_TRACKER_K64_H
