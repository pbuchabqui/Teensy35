/**
 * @file event_scheduler.h
 * @brief rusEFI-compatible Event Scheduler for Teensy 3.5 (MK64FX512)
 *
 * Implements angle-based event scheduling for injection and ignition timing
 * based on rusEFI's event_queue.cpp algorithm.
 *
 * @version 2.3.0
 * @date 2026-02-12
 *
 * Based on rusEFI:
 * - firmware/controllers/scheduling/event_queue.cpp
 * - firmware/controllers/scheduling/event_queue.h
 * - Angle-based scheduling system
 *
 * References:
 * - https://github.com/rusefi/rusefi/blob/master/firmware/controllers/scheduling/
 * - https://rusefi.com/docs/
 */

#ifndef EVENT_SCHEDULER_H
#define EVENT_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of scheduled events
 *
 * Typical usage:
 * - 4 cylinders: 4 injection + 4 ignition = 8 events
 * - 6 cylinders: 6 injection + 6 ignition = 12 events
 * - 8 cylinders: 8 injection + 8 ignition = 16 events
 */
#define MAX_SCHEDULED_EVENTS  16

/**
 * @brief Full crank rotation angle (720° for 4-stroke)
 */
#define FULL_CYCLE_ANGLE      720

/**
 * @brief Scheduled event structure (rusEFI-compatible)
 *
 * Represents a single event to be executed at a specific crank angle.
 */
typedef struct {
    uint16_t trigger_angle;           ///< Crank angle to trigger (0-720°)
    uint8_t cylinder;                 ///< Cylinder number (0-7)
    void (*action)(uint8_t cyl);      ///< Action callback to execute
    bool active;                      ///< Event is currently scheduled
    uint32_t scheduled_time_us;       ///< Calculated execution time (µs)
    uint32_t angle_delta;             ///< Angle delta from current position
} scheduled_event_t;

/**
 * @brief Event scheduler structure (rusEFI-compatible)
 *
 * Manages angle-based event scheduling for injection and ignition timing.
 */
typedef struct {
    // Event queue
    scheduled_event_t events[MAX_SCHEDULED_EVENTS];  ///< Event queue
    uint8_t num_active_events;                       ///< Number of active events

    // Current engine state
    uint16_t current_angle;           ///< Current crank angle (0-720°)
    uint16_t rpm;                     ///< Current RPM for timing calculations

    // Angle-to-time conversion (rusEFI algorithm)
    uint32_t us_per_degree;           ///< Microseconds per crank degree

    // Statistics
    uint32_t events_scheduled;        ///< Total events scheduled
    uint32_t events_fired;            ///< Total events fired
    uint32_t events_missed;           ///< Events that fired late

} event_scheduler_t;

/**
 * @brief Initialize event scheduler
 *
 * Sets up the scheduler with default parameters.
 *
 * @param sched Pointer to scheduler structure
 */
void scheduler_init(event_scheduler_t* sched);

/**
 * @brief Update current crank angle and RPM (rusEFI algorithm)
 *
 * Called on each tooth event to update the scheduler's state.
 * Calculates microseconds per degree for angle-to-time conversion.
 *
 * rusEFI formula:
 *   us_per_degree = 60,000,000 / (rpm * 360)
 *
 * Examples:
 *   - At 600 RPM: 1° = 277.7 µs, 360° = 100 ms
 *   - At 3000 RPM: 1° = 55.5 µs, 360° = 20 ms
 *   - At 6000 RPM: 1° = 27.7 µs, 360° = 10 ms
 *
 * @param sched Pointer to scheduler structure
 * @param angle Current crank angle (0-720°)
 * @param rpm Current engine RPM
 * @param current_time_us Current timestamp in microseconds
 */
void scheduler_update_angle(event_scheduler_t* sched,
                           uint16_t angle,
                           uint16_t rpm,
                           uint32_t current_time_us);

/**
 * @brief Schedule an event at a specific crank angle (rusEFI algorithm)
 *
 * Calculates when to fire the event based on:
 *   1. Angle delta from current position
 *   2. Time until angle (angle_delta * us_per_degree)
 *   3. Absolute execution time (current_time + time_until)
 *
 * Example:
 *   scheduler_add_event(&sched, 180, 0, fire_injector);
 *   // Schedules injector for cylinder 0 to fire at 180° BTDC
 *
 * @param sched Pointer to scheduler structure
 * @param angle Target crank angle (0-720°)
 * @param cylinder Cylinder number (0-7)
 * @param action Callback function to execute
 * @param current_time_us Current timestamp in microseconds
 * @return true if scheduled successfully, false if queue full
 */
bool scheduler_add_event(event_scheduler_t* sched,
                        uint16_t angle,
                        uint8_t cylinder,
                        void (*action)(uint8_t),
                        uint32_t current_time_us);

/**
 * @brief Process scheduled events (rusEFI algorithm)
 *
 * Checks all active events and fires those that have reached their
 * scheduled execution time. Should be called frequently from main loop.
 *
 * @param sched Pointer to scheduler structure
 * @param current_time_us Current timestamp in microseconds
 */
void scheduler_process_events(event_scheduler_t* sched,
                              uint32_t current_time_us);

/**
 * @brief Remove all scheduled events
 *
 * Clears the event queue. Use this when engine sync is lost.
 *
 * @param sched Pointer to scheduler structure
 */
void scheduler_clear_events(event_scheduler_t* sched);

/**
 * @brief Remove event for specific cylinder
 *
 * Cancels scheduled events for a given cylinder.
 * Useful when disabling a cylinder or changing timing.
 *
 * @param sched Pointer to scheduler structure
 * @param cylinder Cylinder number (0-7)
 */
void scheduler_remove_cylinder_events(event_scheduler_t* sched,
                                     uint8_t cylinder);

/**
 * @brief Get number of active events
 *
 * Returns how many events are currently scheduled.
 *
 * @param sched Pointer to scheduler structure
 * @return Number of active events
 */
uint8_t scheduler_get_active_count(const event_scheduler_t* sched);

/**
 * @brief Get scheduler statistics
 *
 * Returns diagnostic information about scheduler performance.
 *
 * @param sched Pointer to scheduler structure
 * @param scheduled Output: total events scheduled
 * @param fired Output: total events fired
 * @param missed Output: events that fired late
 */
void scheduler_get_stats(const event_scheduler_t* sched,
                        uint32_t* scheduled,
                        uint32_t* fired,
                        uint32_t* missed);

/**
 * @brief Calculate microseconds until angle (rusEFI algorithm)
 *
 * Calculates time remaining until a target crank angle is reached.
 *
 * @param sched Pointer to scheduler structure
 * @param target_angle Target crank angle (0-720°)
 * @return Microseconds until angle is reached
 */
uint32_t scheduler_angle_to_time(const event_scheduler_t* sched,
                                uint16_t target_angle);

/**
 * @brief Get current crank angle
 *
 * @param sched Pointer to scheduler structure
 * @return Current crank angle (0-720°)
 */
uint16_t scheduler_get_current_angle(const event_scheduler_t* sched);

/**
 * @brief Get microseconds per degree
 *
 * @param sched Pointer to scheduler structure
 * @return Microseconds per crank degree
 */
uint32_t scheduler_get_us_per_degree(const event_scheduler_t* sched);

#ifdef __cplusplus
}
#endif

#endif // EVENT_SCHEDULER_H
