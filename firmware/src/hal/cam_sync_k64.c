/**
 * @file cam_sync_k64.c
 * @brief Camshaft Synchronization Implementation
 *
 * Implements camshaft sensor processing for engine cycle phase detection.
 *
 * @version 2.3.1
 * @date 2026-02-12
 */

#include "cam_sync_k64.h"
#include <string.h>

// Callback for sync events
static void (*g_sync_callback)(engine_cycle_phase_t phase) = NULL;

/**
 * @brief Initialize cam sync system
 */
void cam_sync_init(cam_sync_state_t* cam_sync)
{
    if (cam_sync == NULL) {
        return;
    }

    memset(cam_sync, 0, sizeof(cam_sync_state_t));

    cam_sync->cycle_phase = CYCLE_PHASE_UNKNOWN;
    cam_sync->cycle_synced = false;
    cam_sync->waiting_for_cam = true;
}

/**
 * @brief Process cam sensor event
 *
 * Simple cam sync algorithm:
 * 1. Detect cam signal edge (rising or falling)
 * 2. Record crank tooth position at cam event
 * 3. On next cam event, check if pattern repeats
 * 4. If pattern matches → Cycle synced!
 */
void cam_sync_process_event(cam_sync_state_t* cam_sync,
                            bool cam_signal,
                            uint8_t crank_tooth,
                            uint32_t timestamp)
{
    if (cam_sync == NULL) {
        return;
    }

    // Check for edge (signal changed)
    bool edge_detected = (cam_signal != cam_sync->prev_cam_signal);

    if (edge_detected) {
        cam_sync->cam_events_total++;
        cam_sync->last_cam_event_time = timestamp;

        // Simple algorithm: rising edge on cam sensor
        if (cam_signal) {
            // Rising edge detected

            if (!cam_sync->cycle_synced) {
                // First time seeing cam signal
                if (cam_sync->waiting_for_cam) {
                    // Record crank position at first cam event
                    cam_sync->crank_tooth_at_cam = crank_tooth;
                    cam_sync->waiting_for_cam = false;

                    // Assume first 360° for now
                    cam_sync->cycle_phase = CYCLE_PHASE_FIRST_360;
                } else {
                    // Second cam event - check if pattern repeats
                    // In real implementation, would check tooth position
                    // For now, just alternate phases

                    if (cam_sync->cycle_phase == CYCLE_PHASE_FIRST_360) {
                        cam_sync->cycle_phase = CYCLE_PHASE_SECOND_360;
                    } else {
                        cam_sync->cycle_phase = CYCLE_PHASE_FIRST_360;
                    }

                    // Now we know the pattern - synced!
                    cam_sync->cycle_synced = true;
                    cam_sync->sync_count++;
                    cam_sync->last_sync_time = timestamp;

                    // Call sync callback
                    if (g_sync_callback != NULL) {
                        g_sync_callback(cam_sync->cycle_phase);
                    }
                }
            } else {
                // Already synced - update phase on each cam event
                // Toggle phase on each cam rising edge
                if (cam_sync->cycle_phase == CYCLE_PHASE_FIRST_360) {
                    cam_sync->cycle_phase = CYCLE_PHASE_SECOND_360;
                } else {
                    cam_sync->cycle_phase = CYCLE_PHASE_FIRST_360;
                }
            }
        }
    }

    // Update previous state
    cam_sync->prev_cam_signal = cam_signal;
    cam_sync->cam_signal = cam_signal;
}

/**
 * @brief Check if cycle is synchronized
 */
bool cam_sync_is_synced(const cam_sync_state_t* cam_sync)
{
    if (cam_sync == NULL) {
        return false;
    }
    return cam_sync->cycle_synced;
}

/**
 * @brief Get current cycle phase
 */
engine_cycle_phase_t cam_sync_get_phase(const cam_sync_state_t* cam_sync)
{
    if (cam_sync == NULL || !cam_sync->cycle_synced) {
        return CYCLE_PHASE_UNKNOWN;
    }
    return cam_sync->cycle_phase;
}

/**
 * @brief Convert crank angle to full cycle angle (0-720°)
 */
uint16_t cam_sync_get_full_cycle_angle(const cam_sync_state_t* cam_sync,
                                       uint16_t crank_angle)
{
    if (cam_sync == NULL || !cam_sync->cycle_synced) {
        // Not synced - just return crank angle
        return crank_angle;
    }

    // Normalize crank angle to 0-360°
    crank_angle = crank_angle % 360;

    // Add 360° offset if in second half of cycle
    if (cam_sync->cycle_phase == CYCLE_PHASE_SECOND_360) {
        return crank_angle + 360;
    }

    return crank_angle;
}

/**
 * @brief Reset cam sync state
 */
void cam_sync_reset(cam_sync_state_t* cam_sync)
{
    if (cam_sync == NULL) {
        return;
    }

    cam_sync->cycle_phase = CYCLE_PHASE_UNKNOWN;
    cam_sync->cycle_synced = false;
    cam_sync->waiting_for_cam = true;
    cam_sync->cam_signal = false;
    cam_sync->prev_cam_signal = false;
}

/**
 * @brief Get cam sync statistics
 */
void cam_sync_get_stats(const cam_sync_state_t* cam_sync,
                       uint32_t* sync_count,
                       uint32_t* sync_loss_count,
                       uint32_t* cam_events)
{
    if (cam_sync == NULL) {
        return;
    }

    if (sync_count != NULL) {
        *sync_count = cam_sync->sync_count;
    }

    if (sync_loss_count != NULL) {
        *sync_loss_count = cam_sync->sync_loss_count;
    }

    if (cam_events != NULL) {
        *cam_events = cam_sync->cam_events_total;
    }
}

/**
 * @brief Set callback for cycle sync events
 */
void cam_sync_set_callback(cam_sync_state_t* cam_sync,
                          void (*callback)(engine_cycle_phase_t phase))
{
    (void)cam_sync;  // Not stored in struct for this simplified implementation
    g_sync_callback = callback;
}
