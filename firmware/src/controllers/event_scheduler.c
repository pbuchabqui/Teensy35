/**
 * @file event_scheduler.c
 * @brief rusEFI-compatible Event Scheduler Implementation
 *
 * Implements angle-based event scheduling for injection and ignition timing
 * based on rusEFI's event_queue.cpp algorithm.
 *
 * Now uses hardware timers for precise event execution (v2.3.1+).
 *
 * @version 2.3.1
 * @date 2026-02-12
 */

#include "event_scheduler.h"
#include "hardware_scheduler_k64.h"
#include <string.h>

// Global hardware scheduler instance
static hw_scheduler_t hw_sched;

// Minimum RPM for scheduling (prevents overflow)
#define MIN_RPM_FOR_SCHEDULING   100

/**
 * @brief Initialize event scheduler
 */
void scheduler_init(event_scheduler_t* sched)
{
    if (sched == NULL) {
        return;
    }

    memset(sched, 0, sizeof(event_scheduler_t));

    // Clear all events
    for (uint8_t i = 0; i < MAX_SCHEDULED_EVENTS; i++) {
        sched->events[i].active = false;
    }

    sched->num_active_events = 0;

    // Initialize hardware timer scheduler
    hw_scheduler_init(&hw_sched);
}

/**
 * @brief Update current crank angle and RPM (rusEFI algorithm)
 *
 * Calculates microseconds per degree for angle-to-time conversion:
 *   us_per_degree = 60,000,000 / (rpm * 360)
 */
void scheduler_update_angle(event_scheduler_t* sched,
                           uint16_t angle,
                           uint16_t rpm,
                           uint32_t current_time_us)
{
    if (sched == NULL) {
        return;
    }

    // Update current state
    sched->current_angle = angle % FULL_CYCLE_ANGLE;
    sched->rpm = rpm;

    // Calculate microseconds per degree (rusEFI formula)
    // At 600 RPM: 360° = 100ms, so 1° = 277.7µs
    // At 3000 RPM: 360° = 20ms, so 1° = 55.5µs
    // At 6000 RPM: 360° = 10ms, so 1° = 27.7µs
    if (rpm >= MIN_RPM_FOR_SCHEDULING) {
        // us_per_degree = 60,000,000 / (rpm * 360)
        sched->us_per_degree = 60000000UL / ((uint32_t)rpm * 360);
    } else {
        // RPM too low - use large value to prevent scheduling
        sched->us_per_degree = 0xFFFFFFFF;
    }

    // Update scheduled event times based on new angle/RPM
    // This recalculates timing for all pending events
    for (uint8_t i = 0; i < MAX_SCHEDULED_EVENTS; i++) {
        if (sched->events[i].active) {
            // Recalculate time until event
            uint32_t time_until = scheduler_angle_to_time(sched,
                                                         sched->events[i].trigger_angle);
            sched->events[i].scheduled_time_us = current_time_us + time_until;
        }
    }
}

/**
 * @brief Calculate microseconds until angle (rusEFI algorithm)
 */
uint32_t scheduler_angle_to_time(const event_scheduler_t* sched,
                                uint16_t target_angle)
{
    if (sched == NULL) {
        return 0;
    }

    // Calculate angle delta (handle wrap-around)
    int16_t angle_delta = target_angle - sched->current_angle;
    if (angle_delta < 0) {
        angle_delta += FULL_CYCLE_ANGLE;  // Wrap to next cycle
    }

    // Convert angle to time: time = angle * us_per_degree
    uint32_t time_until_event = (uint32_t)angle_delta * sched->us_per_degree;

    return time_until_event;
}

/**
 * @brief Schedule an event at a specific crank angle (rusEFI algorithm)
 */
bool scheduler_add_event(event_scheduler_t* sched,
                        uint16_t angle,
                        uint8_t cylinder,
                        void (*action)(uint8_t),
                        uint32_t current_time_us)
{
    if (sched == NULL || action == NULL) {
        return false;
    }

    // Normalize angle to 0-720° range
    angle = angle % FULL_CYCLE_ANGLE;

    // Find free slot in event queue
    for (uint8_t i = 0; i < MAX_SCHEDULED_EVENTS; i++) {
        if (!sched->events[i].active) {
            // Found free slot - schedule event
            sched->events[i].trigger_angle = angle;
            sched->events[i].cylinder = cylinder;
            sched->events[i].action = action;
            sched->events[i].active = true;

            // Calculate angle delta from current position
            int16_t angle_delta = angle - sched->current_angle;
            if (angle_delta < 0) {
                angle_delta += FULL_CYCLE_ANGLE;  // Wrap to next cycle
            }
            sched->events[i].angle_delta = (uint32_t)angle_delta;

            // Calculate absolute execution time (rusEFI angle-based scheduling)
            uint32_t time_until_event = angle_delta * sched->us_per_degree;
            sched->events[i].scheduled_time_us = current_time_us + time_until_event;

            // Update statistics
            sched->num_active_events++;
            sched->events_scheduled++;

            return true;
        }
    }

    // Queue full - could not schedule
    return false;
}

/**
 * @brief Process scheduled events (rusEFI algorithm)
 *
 * Checks all active events and fires those that have reached their
 * scheduled execution time.
 */
void scheduler_process_events(event_scheduler_t* sched,
                              uint32_t current_time_us)
{
    if (sched == NULL) {
        return;
    }

    // Check each active event
    for (uint8_t i = 0; i < MAX_SCHEDULED_EVENTS; i++) {
        if (sched->events[i].active) {
            // Check if event should fire (time has arrived)
            if (current_time_us >= sched->events[i].scheduled_time_us) {
                // Fire event! (rusEFI execution)
                sched->events[i].action(sched->events[i].cylinder);

                // Mark as complete
                sched->events[i].active = false;
                sched->num_active_events--;
                sched->events_fired++;

                // Check if event fired late (missed timing)
                uint32_t lateness_us = current_time_us - sched->events[i].scheduled_time_us;
                if (lateness_us > 1000) {  // More than 1ms late
                    sched->events_missed++;
                }
            }
        }
    }
}

/**
 * @brief Remove all scheduled events
 */
void scheduler_clear_events(event_scheduler_t* sched)
{
    if (sched == NULL) {
        return;
    }

    // Clear all events
    for (uint8_t i = 0; i < MAX_SCHEDULED_EVENTS; i++) {
        sched->events[i].active = false;
    }

    sched->num_active_events = 0;
}

/**
 * @brief Remove event for specific cylinder
 */
void scheduler_remove_cylinder_events(event_scheduler_t* sched,
                                     uint8_t cylinder)
{
    if (sched == NULL) {
        return;
    }

    // Find and remove events for this cylinder
    for (uint8_t i = 0; i < MAX_SCHEDULED_EVENTS; i++) {
        if (sched->events[i].active && sched->events[i].cylinder == cylinder) {
            sched->events[i].active = false;
            sched->num_active_events--;
        }
    }
}

/**
 * @brief Get number of active events
 */
uint8_t scheduler_get_active_count(const event_scheduler_t* sched)
{
    if (sched == NULL) {
        return 0;
    }

    return sched->num_active_events;
}

/**
 * @brief Get scheduler statistics
 */
void scheduler_get_stats(const event_scheduler_t* sched,
                        uint32_t* scheduled,
                        uint32_t* fired,
                        uint32_t* missed)
{
    if (sched == NULL) {
        return;
    }

    if (scheduled != NULL) {
        *scheduled = sched->events_scheduled;
    }

    if (fired != NULL) {
        *fired = sched->events_fired;
    }

    if (missed != NULL) {
        *missed = sched->events_missed;
    }
}

/**
 * @brief Get current crank angle
 */
uint16_t scheduler_get_current_angle(const event_scheduler_t* sched)
{
    if (sched == NULL) {
        return 0;
    }

    return sched->current_angle;
}

/**
 * @brief Get microseconds per degree
 */
uint32_t scheduler_get_us_per_degree(const event_scheduler_t* sched)
{
    if (sched == NULL) {
        return 0;
    }

    return sched->us_per_degree;
}
