/**
 * @file engine_controller.c
 * @brief Main Engine Controller Implementation
 *
 * @version 2.5.0
 * @date 2026-02-12
 */

#include "engine_controller.h"
#include <string.h>
#include <math.h>

// Forward declarations for subsystems (to be implemented in later phases)
extern void fuel_calculator_calculate(engine_controller_t* controller);
extern void timing_calculator_calculate(engine_controller_t* controller);
extern void schedule_injection_events(engine_controller_t* controller);
extern void schedule_ignition_events(engine_controller_t* controller);

/**
 * @brief Initialize engine controller
 */
void engine_controller_init(engine_controller_t* controller,
                            const engine_config_t* config)
{
    if (controller == NULL || config == NULL) {
        return;
    }

    // Clear structure
    memset(controller, 0, sizeof(engine_controller_t));

    // Copy configuration
    memcpy(&controller->config, config, sizeof(engine_config_t));

    // Initialize state
    controller->state = ENGINE_STATE_STOPPED;
    controller->rpm = 0;
    controller->current_cycle_angle = 0;
    controller->next_cylinder_to_fire = 0;

    // Initialize sensor validity flags
    controller->sensors.map_valid = false;
    controller->sensors.tps_valid = false;
    controller->sensors.clt_valid = false;
    controller->sensors.iat_valid = false;
    controller->sensors.lambda_valid = false;
    controller->sensors.battery_valid = false;

    // Default corrections to 1.0 (no correction)
    controller->calc.clt_fuel_correction = 1.0f;
    controller->calc.iat_fuel_correction = 1.0f;
    controller->calc.accel_fuel_correction = 1.0f;
    controller->calc.battery_correction = 1.0f;

    controller->calc.clt_timing_correction = 0;
    controller->calc.accel_timing_correction = 0;

    controller->initialized = true;
}

/**
 * @brief Main engine control update
 *
 * This is the heart of the ECU - called periodically to update all calculations.
 */
void engine_controller_update(engine_controller_t* controller,
                              uint16_t rpm,
                              uint16_t cycle_angle,
                              uint32_t timestamp_us)
{
    if (controller == NULL || !controller->initialized) {
        return;
    }

    // Calculate update period
    if (controller->last_update_time_us > 0) {
        controller->update_period_us = timestamp_us - controller->last_update_time_us;
    }
    controller->last_update_time_us = timestamp_us;

    // Update current values
    controller->rpm = rpm;
    controller->current_cycle_angle = cycle_angle;

    // Step 1: Update engine state machine
    engine_controller_update_state(controller, rpm);

    // Step 2: Read sensors (will be implemented when sensor_adc is ready)
    // For now, sensors must be updated externally via sensor_adc module

    // Step 3: Calculate load
    controller->calc.engine_load_percent = engine_controller_calculate_load(controller);

    // Step 4: Calculate fuel requirements
    // To be implemented in Phase 5
    // fuel_calculator_calculate(controller);

    // Step 5: Calculate timing
    // To be implemented in Phase 6
    // timing_calculator_calculate(controller);

    // Step 6: Schedule injection events
    // To be implemented when fuel calculator is ready
    // schedule_injection_events(controller);

    // Step 7: Schedule ignition events
    // To be implemented when timing calculator is ready
    // schedule_ignition_events(controller);

    // Step 8: Update auxiliary outputs (fuel pump, cooling fan, etc.)
    // To be implemented with aux_outputs module
}

/**
 * @brief Update engine state machine
 */
void engine_controller_update_state(engine_controller_t* controller,
                                    uint16_t rpm)
{
    if (controller == NULL) {
        return;
    }

    engine_state_t previous_state = controller->state;
    engine_state_t new_state = controller->state;

    // State transitions based on RPM and conditions
    if (rpm == 0) {
        // Engine stopped
        new_state = ENGINE_STATE_STOPPED;

    } else if (rpm < controller->config.cranking_rpm) {
        // Cranking (starting)
        new_state = ENGINE_STATE_CRANKING;

    } else {
        // Engine running
        if (previous_state == ENGINE_STATE_CRANKING ||
            previous_state == ENGINE_STATE_STOPPED) {
            // Just started - enter warmup if coolant is cold
            if (controller->sensors.clt_valid &&
                controller->sensors.clt_celsius < 60) {
                new_state = ENGINE_STATE_WARMUP;
            } else {
                new_state = ENGINE_STATE_RUNNING;
            }
        } else if (previous_state == ENGINE_STATE_WARMUP) {
            // Check if warmed up
            if (controller->sensors.clt_valid &&
                controller->sensors.clt_celsius >= 80) {
                new_state = ENGINE_STATE_RUNNING;
            } else {
                new_state = ENGINE_STATE_WARMUP;
            }
        } else {
            // Check for idle or running
            int16_t rpm_diff = abs((int16_t)rpm - (int16_t)controller->config.idle_rpm_target);

            if (rpm_diff < 200 && controller->sensors.tps_percent < 5) {
                new_state = ENGINE_STATE_IDLE;
            } else if (controller->sensors.tps_percent == 0 &&
                      rpm > controller->config.idle_rpm_target + 500) {
                // Deceleration fuel cut conditions
                new_state = ENGINE_STATE_DECEL_FUEL_CUT;
            } else {
                new_state = ENGINE_STATE_RUNNING;
            }
        }
    }

    // Update state if changed
    if (new_state != previous_state) {
        controller->state = new_state;
        controller->state_entry_time = controller->last_update_time_us;
    }
}

/**
 * @brief Calculate engine load
 *
 * Implements different load calculation methods.
 */
float engine_controller_calculate_load(engine_controller_t* controller)
{
    if (controller == NULL) {
        return 0.0f;
    }

    float load = 0.0f;

    switch (controller->config.load_method) {
        case LOAD_METHOD_SPEED_DENSITY:
            // Load based on MAP sensor (Speed-Density)
            // load = (MAP / Barometric) * 100%
            // Assuming barometric = 101.325 kPa at sea level
            if (controller->sensors.map_valid) {
                load = ((float)controller->sensors.map_kpa / 101.325f) * 100.0f;

                // Clamp to valid range
                if (load < 0.0f) load = 0.0f;
                if (load > 200.0f) load = 200.0f;  // Allow up to 200% (boost)
            }
            break;

        case LOAD_METHOD_ALPHA_N:
            // Load based on TPS only (Alpha-N)
            // Used when MAP sensor is not available or unreliable
            if (controller->sensors.tps_valid) {
                load = (float)controller->sensors.tps_percent;
            }
            break;

        case LOAD_METHOD_MAF:
            // Load based on MAF sensor
            // To be implemented when MAF support is added
            load = 0.0f;
            break;

        default:
            load = 0.0f;
            break;
    }

    return load;
}

/**
 * @brief Get current engine state
 */
engine_state_t engine_controller_get_state(const engine_controller_t* controller)
{
    if (controller == NULL) {
        return ENGINE_STATE_STOPPED;
    }
    return controller->state;
}

/**
 * @brief Check if engine is running
 */
bool engine_controller_is_running(const engine_controller_t* controller)
{
    if (controller == NULL) {
        return false;
    }

    return (controller->state == ENGINE_STATE_RUNNING ||
            controller->state == ENGINE_STATE_WARMUP ||
            controller->state == ENGINE_STATE_IDLE ||
            controller->state == ENGINE_STATE_DECEL_FUEL_CUT);
}

/**
 * @brief Check if engine is cranking
 */
bool engine_controller_is_cranking(const engine_controller_t* controller)
{
    if (controller == NULL) {
        return false;
    }
    return (controller->state == ENGINE_STATE_CRANKING);
}

/**
 * @brief Enable/disable closed-loop control
 */
void engine_controller_set_closed_loop(engine_controller_t* controller,
                                       bool enable)
{
    if (controller == NULL) {
        return;
    }
    controller->closed_loop_active = enable;
}

/**
 * @brief Get sensor readings
 */
const sensor_readings_t* engine_controller_get_sensors(
    const engine_controller_t* controller)
{
    if (controller == NULL) {
        return NULL;
    }
    return &controller->sensors;
}

/**
 * @brief Get calculated values
 */
const engine_calculations_t* engine_controller_get_calculations(
    const engine_controller_t* controller)
{
    if (controller == NULL) {
        return NULL;
    }
    return &controller->calc;
}

/**
 * @brief Emergency shutdown
 */
void engine_controller_emergency_shutdown(engine_controller_t* controller)
{
    if (controller == NULL) {
        return;
    }

    // Set state to stopped
    controller->state = ENGINE_STATE_STOPPED;

    // Disable all outputs
    // To be implemented when actuator control is ready
    // - Disable fuel injection
    // - Disable ignition
    // - Stop fuel pump (after delay)
}
