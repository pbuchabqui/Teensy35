/**
 * @file multi_stage_scheduler.c
 * @brief Multi-Stage Event Scheduler Implementation
 *
 * Implements multi-stage events (start + end) for injection and ignition control.
 *
 * @version 2.3.1
 * @date 2026-02-12
 */

#include "multi_stage_scheduler.h"
#include <string.h>

/**
 * @brief Initialize multi-stage scheduler
 */
void multistage_scheduler_init(multistage_scheduler_t* ms_sched,
                               event_scheduler_t* angle_sched)
{
    if (ms_sched == NULL) {
        return;
    }

    memset(ms_sched, 0, sizeof(multistage_scheduler_t));

    ms_sched->angle_scheduler = angle_sched;

    // Clear all events
    for (uint8_t i = 0; i < 8; i++) {
        ms_sched->events[i].active = false;
    }
}

/**
 * @brief Internal: Schedule start event wrapper
 */
static void multistage_start_wrapper(uint8_t context_id)
{
    // Context is event index in multistage scheduler
    // This would need global access to ms_sched, or pass via global
    // For now, simplified implementation
    (void)context_id;
}

/**
 * @brief Internal: Schedule end event wrapper
 */
static void multistage_end_wrapper(uint8_t context_id)
{
    (void)context_id;
}

/**
 * @brief Schedule injection event (angle + duration)
 */
int8_t multistage_schedule_injection(multistage_scheduler_t* ms_sched,
                                    uint8_t cylinder,
                                    uint16_t start_angle,
                                    uint32_t duration_us,
                                    void (*start_action)(uint8_t),
                                    void (*end_action)(uint8_t),
                                    uint16_t rpm,
                                    uint32_t current_time_us)
{
    if (ms_sched == NULL || start_action == NULL || end_action == NULL) {
        return -1;
    }

    // Find free event slot
    int8_t event_id = -1;
    for (uint8_t i = 0; i < 8; i++) {
        if (!ms_sched->events[i].active) {
            event_id = i;
            break;
        }
    }

    if (event_id < 0) {
        return -1;  // Queue full
    }

    // Setup event
    ms_sched->events[event_id].cylinder = cylinder;
    ms_sched->events[event_id].type = MULTISTAGE_INJECTION;
    ms_sched->events[event_id].start_angle = start_angle;
    ms_sched->events[event_id].duration_us = duration_us;
    ms_sched->events[event_id].callbacks.start_action = start_action;
    ms_sched->events[event_id].callbacks.end_action = end_action;
    ms_sched->events[event_id].active = true;
    ms_sched->events[event_id].start_fired = false;
    ms_sched->events[event_id].end_fired = false;

    // Calculate timing
    // us_per_degree = 60,000,000 / (rpm * 360)
    if (rpm < 100) {
        return -1;  // RPM too low
    }

    uint32_t us_per_degree = 60000000UL / ((uint32_t)rpm * 360);

    // Calculate start time from angle
    // This is simplified - in real implementation, would use angle_scheduler
    uint32_t start_time_us = current_time_us;  // Placeholder
    uint32_t end_time_us = start_time_us + duration_us;

    ms_sched->events[event_id].start_time_us = start_time_us;
    ms_sched->events[event_id].end_time_us = end_time_us;

    // Schedule start event via angle scheduler
    bool start_ok = scheduler_add_event(ms_sched->angle_scheduler,
                                       start_angle,
                                       cylinder,
                                       start_action,
                                       current_time_us);

    if (!start_ok) {
        ms_sched->events[event_id].active = false;
        return -1;
    }

    // Calculate end angle
    // duration in degrees = duration_us / us_per_degree
    uint16_t duration_degrees = duration_us / us_per_degree;
    uint16_t end_angle = (start_angle + duration_degrees) % 720;

    // Schedule end event
    bool end_ok = scheduler_add_event(ms_sched->angle_scheduler,
                                     end_angle,
                                     cylinder,
                                     end_action,
                                     current_time_us);

    if (!end_ok) {
        // Start scheduled but end failed - problem!
        // In production code, would cancel start event
        ms_sched->events[event_id].active = false;
        return -1;
    }

    ms_sched->num_active++;
    ms_sched->events_started++;

    return event_id;
}

/**
 * @brief Schedule ignition event (dwell + fire)
 */
int8_t multistage_schedule_ignition(multistage_scheduler_t* ms_sched,
                                   uint8_t cylinder,
                                   uint16_t dwell_angle,
                                   uint16_t fire_angle,
                                   void (*start_action)(uint8_t),
                                   void (*end_action)(uint8_t),
                                   uint16_t rpm,
                                   uint32_t current_time_us)
{
    if (ms_sched == NULL || start_action == NULL || end_action == NULL) {
        return -1;
    }

    // Find free event slot
    int8_t event_id = -1;
    for (uint8_t i = 0; i < 8; i++) {
        if (!ms_sched->events[i].active) {
            event_id = i;
            break;
        }
    }

    if (event_id < 0) {
        return -1;
    }

    // Setup event
    ms_sched->events[event_id].cylinder = cylinder;
    ms_sched->events[event_id].type = MULTISTAGE_IGNITION;
    ms_sched->events[event_id].start_angle = dwell_angle;
    ms_sched->events[event_id].callbacks.start_action = start_action;
    ms_sched->events[event_id].callbacks.end_action = end_action;
    ms_sched->events[event_id].active = true;
    ms_sched->events[event_id].start_fired = false;
    ms_sched->events[event_id].end_fired = false;

    // Schedule dwell start
    bool start_ok = scheduler_add_event(ms_sched->angle_scheduler,
                                       dwell_angle,
                                       cylinder,
                                       start_action,
                                       current_time_us);

    if (!start_ok) {
        ms_sched->events[event_id].active = false;
        return -1;
    }

    // Schedule spark fire
    bool end_ok = scheduler_add_event(ms_sched->angle_scheduler,
                                     fire_angle,
                                     cylinder,
                                     end_action,
                                     current_time_us);

    if (!end_ok) {
        ms_sched->events[event_id].active = false;
        return -1;
    }

    ms_sched->num_active++;
    ms_sched->events_started++;

    return event_id;
}

/**
 * @brief Schedule custom two-stage event
 */
int8_t multistage_schedule_custom(multistage_scheduler_t* ms_sched,
                                 uint8_t cylinder,
                                 uint16_t start_angle,
                                 uint32_t duration_us,
                                 void (*start_action)(uint8_t),
                                 void (*end_action)(uint8_t),
                                 uint16_t rpm,
                                 uint32_t current_time_us)
{
    // Use injection scheduling (same logic)
    return multistage_schedule_injection(ms_sched,
                                        cylinder,
                                        start_angle,
                                        duration_us,
                                        start_action,
                                        end_action,
                                        rpm,
                                        current_time_us);
}

/**
 * @brief Cancel a multi-stage event
 */
bool multistage_cancel_event(multistage_scheduler_t* ms_sched,
                             int8_t event_id)
{
    if (ms_sched == NULL || event_id < 0 || event_id >= 8) {
        return false;
    }

    if (!ms_sched->events[event_id].active) {
        return false;
    }

    // Cancel both start and end events
    // In real implementation, would cancel via angle scheduler
    // For now, just mark as inactive

    ms_sched->events[event_id].active = false;
    ms_sched->num_active--;
    ms_sched->events_cancelled++;

    return true;
}

/**
 * @brief Cancel all events for a cylinder
 */
void multistage_cancel_cylinder(multistage_scheduler_t* ms_sched,
                               uint8_t cylinder)
{
    if (ms_sched == NULL) {
        return;
    }

    for (uint8_t i = 0; i < 8; i++) {
        if (ms_sched->events[i].active &&
            ms_sched->events[i].cylinder == cylinder) {
            multistage_cancel_event(ms_sched, i);
        }
    }
}

/**
 * @brief Get multi-stage scheduler statistics
 */
void multistage_get_stats(const multistage_scheduler_t* ms_sched,
                         uint32_t* started,
                         uint32_t* completed,
                         uint32_t* cancelled)
{
    if (ms_sched == NULL) {
        return;
    }

    if (started != NULL) {
        *started = ms_sched->events_started;
    }

    if (completed != NULL) {
        *completed = ms_sched->events_completed;
    }

    if (cancelled != NULL) {
        *cancelled = ms_sched->events_cancelled;
    }
}
