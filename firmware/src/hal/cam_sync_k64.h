/**
 * @file cam_sync_k64.h
 * @brief Camshaft Synchronization for Teensy 3.5 (MK64FX512)
 *
 * Implements camshaft sensor processing for engine cycle phase detection.
 * Essential for sequential fuel injection and wasted spark elimination.
 *
 * @version 2.3.1
 * @date 2026-02-12
 *
 * Based on rusEFI:
 * - firmware/controllers/trigger/trigger_central.cpp
 * - Cam signal processing and cycle phase detection
 *
 * References:
 * - rusEFI cam sync implementation
 * - 4-stroke engine cycle (720° = 2 crank rotations)
 */

#ifndef CAM_SYNC_K64_H
#define CAM_SYNC_K64_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Engine cycle phase
 *
 * In a 4-stroke engine, one complete cycle is 720° (2 crank revolutions).
 * The cam sensor helps distinguish which 360° we're in.
 */
typedef enum {
    CYCLE_PHASE_UNKNOWN = 0,    ///< Not yet synchronized
    CYCLE_PHASE_FIRST_360,      ///< First 360° (e.g., compression/power)
    CYCLE_PHASE_SECOND_360,     ///< Second 360° (e.g., exhaust/intake)
} engine_cycle_phase_t;

/**
 * @brief Cam sync state
 */
typedef struct {
    // Sensor state
    bool cam_signal;                ///< Current cam sensor state (high/low)
    bool prev_cam_signal;           ///< Previous cam sensor state
    uint32_t last_cam_event_time;   ///< Time of last cam edge (µs)

    // Cycle tracking
    engine_cycle_phase_t cycle_phase;  ///< Current cycle phase
    bool cycle_synced;              ///< Cycle phase is known
    uint32_t sync_count;            ///< Number of successful syncs
    uint32_t sync_loss_count;       ///< Times sync was lost

    // Crank-cam correlation
    uint8_t crank_tooth_at_cam;     ///< Crank tooth index when cam signal came
    bool waiting_for_cam;           ///< Waiting for cam signal to sync

    // Statistics
    uint32_t cam_events_total;      ///< Total cam sensor events
    uint32_t last_sync_time;        ///< Time of last successful sync

} cam_sync_state_t;

/**
 * @brief Initialize cam sync system
 *
 * @param cam_sync Pointer to cam sync state structure
 */
void cam_sync_init(cam_sync_state_t* cam_sync);

/**
 * @brief Process cam sensor event
 *
 * Called when cam sensor signal changes (rising or falling edge).
 * Uses crank position to determine cycle phase.
 *
 * Example (rusEFI algorithm):
 * - Crank synced at tooth 0 (after missing tooth gap)
 * - Cam signal rises at tooth 10 → First 360°
 * - Cam signal rises at tooth 10 again (next revolution) → Second 360°
 * - Pattern identified → Cycle synced!
 *
 * @param cam_sync Pointer to cam sync state
 * @param cam_signal Current cam sensor signal (true = high, false = low)
 * @param crank_tooth Current crank tooth index (from trigger decoder)
 * @param timestamp Current timestamp in microseconds
 */
void cam_sync_process_event(cam_sync_state_t* cam_sync,
                            bool cam_signal,
                            uint8_t crank_tooth,
                            uint32_t timestamp);

/**
 * @brief Check if cycle is synchronized
 *
 * @param cam_sync Pointer to cam sync state
 * @return true if cycle phase is known, false otherwise
 */
bool cam_sync_is_synced(const cam_sync_state_t* cam_sync);

/**
 * @brief Get current cycle phase
 *
 * @param cam_sync Pointer to cam sync state
 * @return Current cycle phase (UNKNOWN if not synced)
 */
engine_cycle_phase_t cam_sync_get_phase(const cam_sync_state_t* cam_sync);

/**
 * @brief Convert crank angle to full cycle angle (0-720°)
 *
 * Takes crank angle (0-360°) and cycle phase to get full cycle angle.
 *
 * Example:
 *   - Crank angle: 90°, Phase: FIRST_360 → 90°
 *   - Crank angle: 90°, Phase: SECOND_360 → 450°
 *
 * @param cam_sync Pointer to cam sync state
 * @param crank_angle Crank angle (0-360°)
 * @return Full cycle angle (0-720°), or crank_angle if not synced
 */
uint16_t cam_sync_get_full_cycle_angle(const cam_sync_state_t* cam_sync,
                                       uint16_t crank_angle);

/**
 * @brief Reset cam sync state
 *
 * Clears synchronization. Use when crank sync is lost.
 *
 * @param cam_sync Pointer to cam sync state
 */
void cam_sync_reset(cam_sync_state_t* cam_sync);

/**
 * @brief Get cam sync statistics
 *
 * @param cam_sync Pointer to cam sync state
 * @param sync_count Output: successful syncs
 * @param sync_loss_count Output: times sync was lost
 * @param cam_events Output: total cam events
 */
void cam_sync_get_stats(const cam_sync_state_t* cam_sync,
                       uint32_t* sync_count,
                       uint32_t* sync_loss_count,
                       uint32_t* cam_events);

/**
 * @brief Set callback for cycle sync events
 *
 * Called when cycle phase is first determined.
 *
 * @param cam_sync Pointer to cam sync state
 * @param callback Function to call on sync (can be NULL)
 */
void cam_sync_set_callback(cam_sync_state_t* cam_sync,
                          void (*callback)(engine_cycle_phase_t phase));

#ifdef __cplusplus
}
#endif

#endif // CAM_SYNC_K64_H
