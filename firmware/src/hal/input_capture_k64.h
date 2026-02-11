/**
 * @file input_capture_k64.h
 * @brief Input Capture driver for Kinetis K64 (Teensy 3.5)
 * @version 1.0.0
 * @date 2026-02-10
 *
 * This file provides input capture functionality for reading crankshaft
 * and camshaft position sensors using FlexTimer (FTM) modules.
 *
 * Features:
 * - Capture pulse timing with microsecond precision
 * - Rising/falling edge detection
 * - Interrupt-driven callbacks
 * - RPM calculation
 * - Support for VR (variable reluctance) and Hall effect sensors
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#ifndef INPUT_CAPTURE_K64_H
#define INPUT_CAPTURE_K64_H

#include <stdint.h>
#include <stdbool.h>
#include "pwm_k64.h"  // Reuse FTM module and channel enums

//=============================================================================
// Input Capture Edge Selection
//=============================================================================

typedef enum {
    IC_EDGE_RISING  = 0,
    IC_EDGE_FALLING = 1,
    IC_EDGE_BOTH    = 2,
} ic_edge_t;

//=============================================================================
// Input Capture Configuration
//=============================================================================

typedef struct {
    ic_edge_t edge;              // Edge to capture (rising/falling/both)
    bool enable_interrupt;       // Enable capture interrupt
    bool enable_filter;          // Enable input filter (debounce)
} ic_config_t;

//=============================================================================
// Input Capture Callback Function Type
//=============================================================================

/**
 * @brief Callback function type for input capture events
 * @param timestamp Timer value at capture (in timer ticks)
 */
typedef void (*ic_callback_t)(uint32_t timestamp);

//=============================================================================
// Crank/Cam Sensor Types
//=============================================================================

typedef enum {
    SENSOR_TYPE_VR,          // Variable Reluctance (inductive)
    SENSOR_TYPE_HALL,        // Hall Effect (digital)
    SENSOR_TYPE_OPTICAL,     // Optical sensor
} sensor_type_t;

//=============================================================================
// Engine Position Data
//=============================================================================

typedef struct {
    uint32_t last_tooth_time;    // Timestamp of last tooth (µs)
    uint32_t tooth_period;       // Period between teeth (µs)
    uint16_t tooth_count;        // Current tooth number
    uint16_t rpm;                // Calculated engine RPM
    bool sync_locked;            // Engine sync status
} engine_position_t;

//=============================================================================
// Function Prototypes
//=============================================================================

/**
 * @brief Initialize input capture channel
 *
 * @param ftm FlexTimer module
 * @param channel Input capture channel
 * @param config Pointer to configuration structure
 * @return true if initialization successful
 */
bool ic_init(pwm_ftm_t ftm, pwm_channel_t channel, const ic_config_t* config);

/**
 * @brief Register callback for input capture event
 *
 * @param ftm FlexTimer module
 * @param channel Input capture channel
 * @param callback Function to call on capture event
 */
void ic_register_callback(pwm_ftm_t ftm, pwm_channel_t channel,
                          ic_callback_t callback);

/**
 * @brief Get last captured timestamp
 *
 * @param ftm FlexTimer module
 * @param channel Input capture channel
 * @return Captured timer value in ticks
 */
uint32_t ic_get_capture_value(pwm_ftm_t ftm, pwm_channel_t channel);

/**
 * @brief Get time between last two captures in microseconds
 *
 * @param ftm FlexTimer module
 * @param channel Input capture channel
 * @return Period in microseconds
 */
uint32_t ic_get_period_us(pwm_ftm_t ftm, pwm_channel_t channel);

/**
 * @brief Calculate RPM from capture period
 *
 * Calculates engine RPM based on time between pulses and teeth per revolution
 *
 * @param period_us Period between pulses in microseconds
 * @param teeth_per_rev Number of teeth per revolution
 * @return Engine RPM
 */
uint16_t ic_calculate_rpm(uint32_t period_us, uint16_t teeth_per_rev);

/**
 * @brief Enable input capture
 *
 * @param ftm FlexTimer module
 * @param channel Input capture channel
 */
void ic_enable(pwm_ftm_t ftm, pwm_channel_t channel);

/**
 * @brief Disable input capture
 *
 * @param ftm FlexTimer module
 * @param channel Input capture channel
 */
void ic_disable(pwm_ftm_t ftm, pwm_channel_t channel);

/**
 * @brief Check if new capture event occurred
 *
 * @param ftm FlexTimer module
 * @param channel Input capture channel
 * @return true if capture event flag is set
 */
bool ic_event_occurred(pwm_ftm_t ftm, pwm_channel_t channel);

/**
 * @brief Clear capture event flag
 *
 * @param ftm FlexTimer module
 * @param channel Input capture channel
 */
void ic_clear_event(pwm_ftm_t ftm, pwm_channel_t channel);

//=============================================================================
// High-Level Crank/Cam Functions
//=============================================================================

/**
 * @brief Initialize crankshaft position sensor
 *
 * @param teeth_per_rev Number of teeth on crank wheel (e.g., 36, 60)
 * @param missing_teeth Number of missing teeth (e.g., 1 for 36-1)
 * @param sensor_type Type of sensor (VR, Hall, Optical)
 */
void crank_sensor_init(uint16_t teeth_per_rev, uint16_t missing_teeth,
                       sensor_type_t sensor_type);

/**
 * @brief Initialize camshaft position sensor
 *
 * @param teeth_per_rev Number of cam pulses per revolution
 * @param sensor_type Type of sensor
 */
void cam_sensor_init(uint16_t teeth_per_rev, sensor_type_t sensor_type);

/**
 * @brief Get current engine position data
 *
 * @return Pointer to engine position structure
 */
engine_position_t* get_engine_position(void);

/**
 * @brief Get current engine RPM
 *
 * @return Engine RPM (0 if not synchronized)
 */
uint16_t get_engine_rpm(void);

/**
 * @brief Check if engine is synchronized
 *
 * @return true if engine position is locked
 */
bool is_engine_synced(void);

#endif // INPUT_CAPTURE_K64_H
