/**
 * @file multi_stage_scheduler.h
 * @brief Multi-Stage Event Scheduler for Teensy 3.5 (MK64FX512)
 *
 * Implements multi-stage events (start + end) for injection and ignition control.
 * Essential for controlling injection duration and ignition dwell time.
 *
 * @version 2.3.1
 * @date 2026-02-12
 *
 * Based on rusEFI:
 * - firmware/controllers/actuators/injector_model.cpp
 * - firmware/controllers/actuators/ignition_controller.cpp
 * - Multi-stage event scheduling for duration control
 *
 * References:
 * - rusEFI injection control (open + close events)
 * - rusEFI ignition dwell (charge + fire events)
 */

#ifndef MULTI_STAGE_SCHEDULER_H
#define MULTI_STAGE_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include "event_scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Multi-stage event type
 */
typedef enum {
    MULTISTAGE_INJECTION,   ///< Injection event (open + close)
    MULTISTAGE_IGNITION,    ///< Ignition event (charge + fire)
    MULTISTAGE_CUSTOM       ///< Custom two-stage event
} multistage_event_type_t;

/**
 * @brief Multi-stage event callbacks
 */
typedef struct {
    void (*start_action)(uint8_t cylinder);  ///< Start event (e.g., open injector)
    void (*end_action)(uint8_t cylinder);    ///< End event (e.g., close injector)
} multistage_callbacks_t;

/**
 * @brief Multi-stage event structure
 *
 * Represents a two-stage event (start + end) with duration control.
 */
typedef struct {
    // Event identification
    uint8_t cylinder;                        ///< Cylinder number
    multistage_event_type_t type;            ///< Event type

    // Timing (angle-based)
    uint16_t start_angle;                    ///< Start angle (0-720°)
    uint32_t duration_us;                    ///< Duration in microseconds

    // Or timing (time-based)
    uint32_t start_time_us;                  ///< Absolute start time (µs)
    uint32_t end_time_us;                    ///< Absolute end time (µs)

    // Callbacks
    multistage_callbacks_t callbacks;        ///< Start and end actions

    // State
    bool active;                             ///< Event is scheduled
    bool start_fired;                        ///< Start event has fired
    bool end_fired;                          ///< End event has fired

} multistage_event_t;

/**
 * @brief Multi-stage scheduler structure
 */
typedef struct {
    multistage_event_t events[8];            ///< Max 8 multi-stage events
    event_scheduler_t* angle_scheduler;      ///< Reference to angle scheduler
    uint8_t num_active;                      ///< Number of active events

    // Statistics
    uint32_t events_started;                 ///< Total start events fired
    uint32_t events_completed;               ///< Total end events fired
    uint32_t events_cancelled;               ///< Events cancelled mid-flight

} multistage_scheduler_t;

/**
 * @brief Initialize multi-stage scheduler
 *
 * @param ms_sched Pointer to multi-stage scheduler
 * @param angle_sched Pointer to angle-based scheduler (for integration)
 */
void multistage_scheduler_init(multistage_scheduler_t* ms_sched,
                               event_scheduler_t* angle_sched);

/**
 * @brief Schedule injection event (angle + duration)
 *
 * Schedules injector to open at specified angle and close after duration.
 *
 * Example:
 *   // Open injector at 180° BTDC, keep open for 2ms
 *   multistage_schedule_injection(&ms_sched, 0, 180, 2000,
 *                                open_injector, close_injector,
 *                                rpm, current_time);
 *
 * @param ms_sched Pointer to multi-stage scheduler
 * @param cylinder Cylinder number (0-7)
 * @param start_angle Start angle in degrees (0-720°)
 * @param duration_us Duration in microseconds
 * @param start_action Callback to open injector
 * @param end_action Callback to close injector
 * @param rpm Current engine RPM (for angle-to-time conversion)
 * @param current_time_us Current time in microseconds
 * @return Event ID (0-7) if scheduled, -1 if queue full
 */
int8_t multistage_schedule_injection(multistage_scheduler_t* ms_sched,
                                    uint8_t cylinder,
                                    uint16_t start_angle,
                                    uint32_t duration_us,
                                    void (*start_action)(uint8_t),
                                    void (*end_action)(uint8_t),
                                    uint16_t rpm,
                                    uint32_t current_time_us);

/**
 * @brief Schedule ignition event (dwell + fire)
 *
 * Schedules coil to start charging (dwell) and then fire spark.
 *
 * Example:
 *   // Start dwell at 30° BTDC, fire spark at 15° BTDC (dwell = 1ms @ 6000 RPM)
 *   multistage_schedule_ignition(&ms_sched, 0, 30, 15,
 *                               start_dwell, fire_spark,
 *                               6000, current_time);
 *
 * @param ms_sched Pointer to multi-stage scheduler
 * @param cylinder Cylinder number (0-7)
 * @param dwell_angle Angle to start charging coil (0-720°)
 * @param fire_angle Angle to fire spark (0-720°)
 * @param start_action Callback to start charging coil
 * @param end_action Callback to fire spark
 * @param rpm Current engine RPM
 * @param current_time_us Current time in microseconds
 * @return Event ID (0-7) if scheduled, -1 if queue full
 */
int8_t multistage_schedule_ignition(multistage_scheduler_t* ms_sched,
                                   uint8_t cylinder,
                                   uint16_t dwell_angle,
                                   uint16_t fire_angle,
                                   void (*start_action)(uint8_t),
                                   void (*end_action)(uint8_t),
                                   uint16_t rpm,
                                   uint32_t current_time_us);

/**
 * @brief Schedule custom two-stage event
 *
 * Generic interface for any two-stage event with duration.
 *
 * @param ms_sched Pointer to multi-stage scheduler
 * @param cylinder Cylinder/channel number
 * @param start_angle Start angle (0-720°)
 * @param duration_us Duration in microseconds
 * @param start_action Start callback
 * @param end_action End callback
 * @param rpm Current RPM
 * @param current_time_us Current time
 * @return Event ID if scheduled, -1 if queue full
 */
int8_t multistage_schedule_custom(multistage_scheduler_t* ms_sched,
                                 uint8_t cylinder,
                                 uint16_t start_angle,
                                 uint32_t duration_us,
                                 void (*start_action)(uint8_t),
                                 void (*end_action)(uint8_t),
                                 uint16_t rpm,
                                 uint32_t current_time_us);

/**
 * @brief Cancel a multi-stage event
 *
 * Cancels both start and end events if not yet fired.
 * If start has fired but end hasn't, only end is cancelled.
 *
 * @param ms_sched Pointer to multi-stage scheduler
 * @param event_id Event ID from schedule function
 * @return true if cancelled, false if not found
 */
bool multistage_cancel_event(multistage_scheduler_t* ms_sched,
                             int8_t event_id);

/**
 * @brief Cancel all events for a cylinder
 *
 * @param ms_sched Pointer to multi-stage scheduler
 * @param cylinder Cylinder number
 */
void multistage_cancel_cylinder(multistage_scheduler_t* ms_sched,
                               uint8_t cylinder);

/**
 * @brief Get multi-stage scheduler statistics
 *
 * @param ms_sched Pointer to multi-stage scheduler
 * @param started Output: events started
 * @param completed Output: events completed
 * @param cancelled Output: events cancelled
 */
void multistage_get_stats(const multistage_scheduler_t* ms_sched,
                         uint32_t* started,
                         uint32_t* completed,
                         uint32_t* cancelled);

#ifdef __cplusplus
}
#endif

#endif // MULTI_STAGE_SCHEDULER_H
