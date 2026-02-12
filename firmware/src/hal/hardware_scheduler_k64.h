/**
 * @file hardware_scheduler_k64.h
 * @brief Hardware Timer-Based Event Scheduler for Teensy 3.5 (MK64FX512)
 *
 * Implements precise hardware timer scheduling using FTM Output Compare mode.
 * Provides microsecond-precision event firing without polling overhead.
 *
 * @version 2.3.0
 * @date 2026-02-12
 *
 * Based on rusEFI:
 * - firmware/hw_layer/ports/kinetis/microsecond_timer_kinetis.cpp
 * - firmware/controllers/scheduling/single_timer_executor.cpp
 * - Uses hardware timers for precise event execution
 *
 * References:
 * - MK64FX512 Reference Manual: Chapter 36 (FlexTimer Module)
 * - rusEFI hardware timer implementation
 */

#ifndef HARDWARE_SCHEDULER_K64_H
#define HARDWARE_SCHEDULER_K64_H

#include <stdint.h>
#include <stdbool.h>
#include "pwm_k64.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of hardware-scheduled events
 *
 * Limited by number of available FTM channels.
 * Teensy 3.5 has multiple FTM modules with 8 channels each.
 */
#define HW_SCHEDULER_MAX_EVENTS  8

/**
 * @brief Hardware scheduler event callback type
 *
 * Called automatically by FTM interrupt when scheduled time arrives.
 */
typedef void (*hw_event_callback_t)(void* context);

/**
 * @brief Hardware scheduled event structure
 */
typedef struct {
    bool active;                      ///< Event is scheduled
    uint32_t scheduled_time_us;       ///< Absolute execution time (µs)
    hw_event_callback_t callback;     ///< Function to call
    void* context;                    ///< User context data
    pwm_ftm_t ftm;                    ///< FTM module used
    pwm_channel_t channel;            ///< FTM channel used
} hw_scheduled_event_t;

/**
 * @brief Hardware scheduler structure
 */
typedef struct {
    hw_scheduled_event_t events[HW_SCHEDULER_MAX_EVENTS];
    uint8_t num_active;               ///< Number of active events
    uint32_t events_fired;            ///< Total events fired
    uint32_t events_missed;           ///< Events that fired late
    bool initialized;                 ///< Scheduler initialized
} hw_scheduler_t;

/**
 * @brief Initialize hardware scheduler
 *
 * Sets up FTM modules for Output Compare mode with interrupts.
 * Must be called before scheduling any events.
 *
 * @param sched Pointer to hardware scheduler structure
 * @return true if initialized successfully, false on error
 */
bool hw_scheduler_init(hw_scheduler_t* sched);

/**
 * @brief Schedule an event at absolute time (hardware timer)
 *
 * Uses FTM Output Compare to fire callback at exact microsecond time.
 * Automatically handled by hardware interrupt - no polling needed.
 *
 * Example:
 *   uint32_t fire_time = micros() + 1000;  // Fire in 1ms
 *   hw_scheduler_schedule(&sched, fire_time, fire_injector, &inj_data);
 *
 * @param sched Pointer to hardware scheduler structure
 * @param absolute_time_us Absolute time in microseconds (from micros())
 * @param callback Function to call at scheduled time
 * @param context User context pointer (passed to callback)
 * @return Event ID (0-7) if scheduled, -1 if queue full
 */
int8_t hw_scheduler_schedule(hw_scheduler_t* sched,
                             uint32_t absolute_time_us,
                             hw_event_callback_t callback,
                             void* context);

/**
 * @brief Cancel a scheduled event
 *
 * Stops the hardware timer and removes event from queue.
 *
 * @param sched Pointer to hardware scheduler structure
 * @param event_id Event ID returned from hw_scheduler_schedule()
 * @return true if canceled successfully, false if event not found
 */
bool hw_scheduler_cancel(hw_scheduler_t* sched, int8_t event_id);

/**
 * @brief Cancel all scheduled events
 *
 * Stops all hardware timers and clears the queue.
 * Use when engine sync is lost.
 *
 * @param sched Pointer to hardware scheduler structure
 */
void hw_scheduler_cancel_all(hw_scheduler_t* sched);

/**
 * @brief Get current time in microseconds
 *
 * Uses FTM counter for precise timing.
 * Compatible with Arduino micros() but more accurate.
 *
 * @return Current time in microseconds
 */
uint32_t hw_scheduler_micros(void);

/**
 * @brief Check if event is still scheduled
 *
 * @param sched Pointer to hardware scheduler structure
 * @param event_id Event ID to check
 * @return true if event is active, false otherwise
 */
bool hw_scheduler_is_scheduled(const hw_scheduler_t* sched, int8_t event_id);

/**
 * @brief Get scheduler statistics
 *
 * @param sched Pointer to hardware scheduler structure
 * @param fired Output: total events fired
 * @param missed Output: events that fired late
 */
void hw_scheduler_get_stats(const hw_scheduler_t* sched,
                            uint32_t* fired,
                            uint32_t* missed);

/**
 * @brief FTM interrupt handler (internal use)
 *
 * Called automatically by hardware when scheduled time arrives.
 * Do not call directly.
 *
 * @param ftm FTM module that generated interrupt
 * @param channel FTM channel that matched
 */
void hw_scheduler_ftm_isr(pwm_ftm_t ftm, pwm_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif // HARDWARE_SCHEDULER_K64_H
