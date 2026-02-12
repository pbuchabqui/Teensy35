/**
 * @file vvt_tracker_k64.c
 * @brief Variable Valve Timing (VVT) Position Tracker Implementation
 *
 * @version 2.4.0
 * @date 2026-02-12
 */

#include "vvt_tracker_k64.h"
#include <string.h>

/**
 * @brief Initialize VVT tracker
 */
void vvt_tracker_init(vvt_tracker_t* vvt,
                     uint8_t teeth_per_rev,
                     int16_t offset_degrees)
{
    if (vvt == NULL) {
        return;
    }

    memset(vvt, 0, sizeof(vvt_tracker_t));

    vvt->vvt_teeth_per_rev = teeth_per_rev;
    vvt->vvt_offset_degrees = offset_degrees;
    vvt->vvt_synced = false;
    vvt->position_degrees = 0;
    vvt->target_position = 0;
}

/**
 * @brief Process VVT sensor event
 *
 * Simplified VVT position calculation:
 * 1. Record crank position when VVT event occurs
 * 2. Compare to expected position (0° VVT)
 * 3. Difference = VVT advance/retard
 */
void vvt_tracker_process_event(vvt_tracker_t* vvt,
                               uint8_t crank_tooth,
                               uint16_t crank_angle,
                               uint32_t timestamp)
{
    if (vvt == NULL) {
        return;
    }

    vvt->vvt_events_total++;
    vvt->last_vvt_event_time = timestamp;

    // Record crank position at VVT event
    vvt->crank_tooth_at_vvt = crank_tooth;

    if (!vvt->vvt_synced) {
        // First VVT event - establish baseline
        vvt->phase_offset = 0;  // Assume 0° position initially
        vvt->vvt_synced = true;
        vvt->sync_count++;
    } else {
        // Calculate VVT position relative to crank
        // This is simplified - real implementation would use more complex correlation

        // For now, use crank angle modulo to estimate VVT position
        // Typical VVT range: -50° to +50°
        // Base position at crank tooth 0

        // Calculate expected crank tooth for 0° VVT
        uint8_t expected_tooth = 0;  // Simplified

        // Calculate phase difference
        int16_t tooth_diff = (int16_t)crank_tooth - (int16_t)expected_tooth;

        // Convert tooth difference to degrees
        // Assuming 36-1 wheel: 360° / 36 teeth = 10° per tooth
        int16_t degrees_per_tooth = 10;
        vvt->phase_offset = tooth_diff * degrees_per_tooth;

        // Calculate final position with offset calibration
        vvt->position_degrees = vvt->phase_offset + vvt->vvt_offset_degrees;

        // Clamp to typical VVT range (-50° to +50°)
        if (vvt->position_degrees > 50) {
            vvt->position_degrees = 50;
        } else if (vvt->position_degrees < -50) {
            vvt->position_degrees = -50;
        }

        // Track min/max
        if (vvt->position_degrees < vvt->min_position) {
            vvt->min_position = vvt->position_degrees;
        }
        if (vvt->position_degrees > vvt->max_position) {
            vvt->max_position = vvt->position_degrees;
        }
    }

    // Increment tooth counter
    vvt->vvt_tooth_count++;
    if (vvt->vvt_tooth_count >= vvt->vvt_teeth_per_rev) {
        vvt->vvt_tooth_count = 0;
    }
}

/**
 * @brief Get current VVT position
 */
int16_t vvt_tracker_get_position(const vvt_tracker_t* vvt)
{
    if (vvt == NULL || !vvt->vvt_synced) {
        return 0;
    }
    return vvt->position_degrees;
}

/**
 * @brief Check if VVT is synchronized
 */
bool vvt_tracker_is_synced(const vvt_tracker_t* vvt)
{
    if (vvt == NULL) {
        return false;
    }
    return vvt->vvt_synced;
}

/**
 * @brief Set VVT target position
 */
void vvt_tracker_set_target(vvt_tracker_t* vvt, int16_t target_degrees)
{
    if (vvt == NULL) {
        return;
    }

    // Clamp target to valid range
    if (target_degrees > 50) {
        target_degrees = 50;
    } else if (target_degrees < -50) {
        target_degrees = -50;
    }

    vvt->target_position = target_degrees;
}

/**
 * @brief Get VVT error (target - actual)
 */
int16_t vvt_tracker_get_error(const vvt_tracker_t* vvt)
{
    if (vvt == NULL || !vvt->vvt_synced) {
        return 0;
    }

    return vvt->target_position - vvt->position_degrees;
}

/**
 * @brief Reset VVT tracker
 */
void vvt_tracker_reset(vvt_tracker_t* vvt)
{
    if (vvt == NULL) {
        return;
    }

    vvt->vvt_synced = false;
    vvt->position_degrees = 0;
    vvt->phase_offset = 0;
    vvt->vvt_tooth_count = 0;
}

/**
 * @brief Get VVT statistics
 */
void vvt_tracker_get_stats(const vvt_tracker_t* vvt,
                          uint32_t* sync_count,
                          uint32_t* sync_loss_count,
                          uint32_t* events_total)
{
    if (vvt == NULL) {
        return;
    }

    if (sync_count != NULL) {
        *sync_count = vvt->sync_count;
    }

    if (sync_loss_count != NULL) {
        *sync_loss_count = vvt->sync_loss_count;
    }

    if (events_total != NULL) {
        *events_total = vvt->vvt_events_total;
    }
}
