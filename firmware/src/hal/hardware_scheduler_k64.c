/**
 * @file hardware_scheduler_k64.c
 * @brief Hardware Timer-Based Event Scheduler Implementation
 *
 * Implements precise hardware timer scheduling using FTM Output Compare mode.
 *
 * @version 2.3.0
 * @date 2026-02-12
 */

#include "hardware_scheduler_k64.h"
#include "clock_k64.h"
#include <string.h>

// FTM registers (from pwm_k64.c)
extern FTM_Type* pwm_get_regs(pwm_ftm_t ftm);

// Global hardware scheduler instance (for ISR access)
static hw_scheduler_t* g_hw_sched = NULL;

// FTM channel allocation (track which channels are used for scheduling)
static bool ftm_channel_allocated[4][8] = {{false}};  // 4 FTMs, 8 channels each

/**
 * @brief Initialize hardware scheduler
 */
bool hw_scheduler_init(hw_scheduler_t* sched)
{
    if (sched == NULL) {
        return false;
    }

    memset(sched, 0, sizeof(hw_scheduler_t));

    // Clear all events
    for (uint8_t i = 0; i < HW_SCHEDULER_MAX_EVENTS; i++) {
        sched->events[i].active = false;
    }

    sched->initialized = true;
    g_hw_sched = sched;  // Store for ISR access

    return true;
}

/**
 * @brief Find free FTM channel for scheduling
 */
static bool find_free_ftm_channel(pwm_ftm_t* ftm, pwm_channel_t* channel)
{
    // Try FTM1 and FTM2 first (FTM0 may be used for crank sensor)
    for (uint8_t f = 1; f <= 2; f++) {
        for (uint8_t ch = 0; ch < 8; ch++) {
            if (!ftm_channel_allocated[f][ch]) {
                *ftm = (pwm_ftm_t)f;
                *channel = (pwm_channel_t)ch;
                ftm_channel_allocated[f][ch] = true;
                return true;
            }
        }
    }

    return false;  // No free channels
}

/**
 * @brief Setup FTM channel for Output Compare mode
 */
static void setup_ftm_output_compare(pwm_ftm_t ftm, pwm_channel_t channel,
                                     uint32_t match_time_ticks)
{
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs == NULL) {
        return;
    }

    // Configure channel for Output Compare mode
    // CnSC: MSA=0, MSB=1, ELSB=0, ELSA=0, CHIE=1 (interrupt enabled)
    ftm_regs->CONTROLS[channel].CnSC = 0x50;  // Output Compare + interrupt

    // Set compare value (when to fire interrupt)
    ftm_regs->CONTROLS[channel].CnV = match_time_ticks;

    // Enable FTM interrupt in NVIC
    // FTM1: IRQ 42, FTM2: IRQ 43
    if (ftm == PWM_FTM1) {
        NVIC_ENABLE_IRQ(IRQ_FTM1);
    } else if (ftm == PWM_FTM2) {
        NVIC_ENABLE_IRQ(IRQ_FTM2);
    }
}

/**
 * @brief Convert microseconds to FTM ticks
 */
static uint32_t us_to_ftm_ticks(uint32_t us)
{
    // FTM clock = bus clock / prescaler
    // Assuming prescaler = 1 (set by PWM init)
    uint32_t bus_clock = clock_get_bus_freq();
    uint64_t ticks = ((uint64_t)us * bus_clock) / 1000000;
    return (uint32_t)ticks;
}

/**
 * @brief Convert FTM ticks to microseconds
 */
static uint32_t ftm_ticks_to_us(uint32_t ticks)
{
    uint32_t bus_clock = clock_get_bus_freq();
    uint64_t us = ((uint64_t)ticks * 1000000) / bus_clock;
    return (uint32_t)us;
}

/**
 * @brief Schedule an event at absolute time (hardware timer)
 */
int8_t hw_scheduler_schedule(hw_scheduler_t* sched,
                             uint32_t absolute_time_us,
                             hw_event_callback_t callback,
                             void* context)
{
    if (sched == NULL || callback == NULL) {
        return -1;
    }

    // Find free event slot
    int8_t event_id = -1;
    for (uint8_t i = 0; i < HW_SCHEDULER_MAX_EVENTS; i++) {
        if (!sched->events[i].active) {
            event_id = i;
            break;
        }
    }

    if (event_id < 0) {
        return -1;  // Queue full
    }

    // Find free FTM channel
    pwm_ftm_t ftm;
    pwm_channel_t channel;
    if (!find_free_ftm_channel(&ftm, &channel)) {
        return -1;  // No free channels
    }

    // Setup event
    sched->events[event_id].active = true;
    sched->events[event_id].scheduled_time_us = absolute_time_us;
    sched->events[event_id].callback = callback;
    sched->events[event_id].context = context;
    sched->events[event_id].ftm = ftm;
    sched->events[event_id].channel = channel;

    // Calculate match time in FTM ticks
    uint32_t match_ticks = us_to_ftm_ticks(absolute_time_us);

    // Setup hardware timer
    setup_ftm_output_compare(ftm, channel, match_ticks);

    sched->num_active++;

    return event_id;
}

/**
 * @brief Cancel a scheduled event
 */
bool hw_scheduler_cancel(hw_scheduler_t* sched, int8_t event_id)
{
    if (sched == NULL || event_id < 0 || event_id >= HW_SCHEDULER_MAX_EVENTS) {
        return false;
    }

    if (!sched->events[event_id].active) {
        return false;  // Event not active
    }

    // Disable FTM channel interrupt
    pwm_ftm_t ftm = sched->events[event_id].ftm;
    pwm_channel_t channel = sched->events[event_id].channel;

    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs != NULL) {
        // Disable channel interrupt
        ftm_regs->CONTROLS[channel].CnSC &= ~0x40;  // Clear CHIE bit
    }

    // Free FTM channel
    ftm_channel_allocated[ftm][channel] = false;

    // Mark event as inactive
    sched->events[event_id].active = false;
    sched->num_active--;

    return true;
}

/**
 * @brief Cancel all scheduled events
 */
void hw_scheduler_cancel_all(hw_scheduler_t* sched)
{
    if (sched == NULL) {
        return;
    }

    for (uint8_t i = 0; i < HW_SCHEDULER_MAX_EVENTS; i++) {
        if (sched->events[i].active) {
            hw_scheduler_cancel(sched, i);
        }
    }
}

/**
 * @brief Get current time in microseconds
 */
uint32_t hw_scheduler_micros(void)
{
    // Use FTM0 counter as timebase (assuming it's running)
    FTM_Type* ftm0 = pwm_get_regs(PWM_FTM0);
    if (ftm0 == NULL) {
        return 0;
    }

    uint32_t cnt = ftm0->CNT;
    return ftm_ticks_to_us(cnt);
}

/**
 * @brief Check if event is still scheduled
 */
bool hw_scheduler_is_scheduled(const hw_scheduler_t* sched, int8_t event_id)
{
    if (sched == NULL || event_id < 0 || event_id >= HW_SCHEDULER_MAX_EVENTS) {
        return false;
    }

    return sched->events[event_id].active;
}

/**
 * @brief Get scheduler statistics
 */
void hw_scheduler_get_stats(const hw_scheduler_t* sched,
                            uint32_t* fired,
                            uint32_t* missed)
{
    if (sched == NULL) {
        return;
    }

    if (fired != NULL) {
        *fired = sched->events_fired;
    }

    if (missed != NULL) {
        *missed = sched->events_missed;
    }
}

/**
 * @brief FTM interrupt handler (internal use)
 *
 * Called automatically by hardware when scheduled time arrives.
 */
void hw_scheduler_ftm_isr(pwm_ftm_t ftm, pwm_channel_t channel)
{
    if (g_hw_sched == NULL) {
        return;
    }

    // Find event that matches this FTM/channel
    for (uint8_t i = 0; i < HW_SCHEDULER_MAX_EVENTS; i++) {
        if (g_hw_sched->events[i].active &&
            g_hw_sched->events[i].ftm == ftm &&
            g_hw_sched->events[i].channel == channel) {

            // Fire event callback
            if (g_hw_sched->events[i].callback != NULL) {
                g_hw_sched->events[i].callback(g_hw_sched->events[i].context);
            }

            // Update statistics
            g_hw_sched->events_fired++;

            // Check if event fired late
            uint32_t current_time = hw_scheduler_micros();
            if (current_time > g_hw_sched->events[i].scheduled_time_us + 100) {
                // More than 100µs late
                g_hw_sched->events_missed++;
            }

            // Mark event as complete and free resources
            hw_scheduler_cancel(g_hw_sched, i);

            break;
        }
    }

    // Clear interrupt flag
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    if (ftm_regs != NULL) {
        ftm_regs->CONTROLS[channel].CnSC &= ~0x80;  // Clear CHF flag
    }
}

/**
 * @brief FTM1 interrupt handler
 */
void FTM1_IRQHandler(void)
{
    FTM_Type* ftm1 = pwm_get_regs(PWM_FTM1);
    if (ftm1 == NULL) {
        return;
    }

    // Check which channel generated interrupt
    for (uint8_t ch = 0; ch < 8; ch++) {
        if (ftm1->CONTROLS[ch].CnSC & 0x80) {  // CHF flag set
            hw_scheduler_ftm_isr(PWM_FTM1, (pwm_channel_t)ch);
        }
    }
}

/**
 * @brief FTM2 interrupt handler
 */
void FTM2_IRQHandler(void)
{
    FTM_Type* ftm2 = pwm_get_regs(PWM_FTM2);
    if (ftm2 == NULL) {
        return;
    }

    // Check which channel generated interrupt
    for (uint8_t ch = 0; ch < 8; ch++) {
        if (ftm2->CONTROLS[ch].CnSC & 0x80) {  // CHF flag set
            hw_scheduler_ftm_isr(PWM_FTM2, (pwm_channel_t)ch);
        }
    }
}
