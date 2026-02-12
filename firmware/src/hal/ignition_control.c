/**
 * @file ignition_control.c
 * @brief Ignition Coil Control System Implementation
 *
 * @version 2.5.0
 * @date 2026-02-12
 */

#include "ignition_control.h"
#include <string.h>

// Arduino/Teensy digital I/O
#ifdef ARDUINO
    #include <Arduino.h>
#else
    // Stub functions for testing
    static void pinMode(uint8_t pin, uint8_t mode) { (void)pin; (void)mode; }
    static void digitalWrite(uint8_t pin, uint8_t val) { (void)pin; (void)val; }
    #define OUTPUT 1
    #define HIGH 1
    #define LOW 0
#endif

// Default maximum dwell time (10ms)
#define DEFAULT_MAX_DWELL_US  10000

/**
 * @brief Initialize ignition control system
 */
void ignition_control_init(ignition_control_t* ignition,
                           uint8_t num_coils,
                           ignition_mode_t mode,
                           uint32_t max_dwell_us)
{
    if (ignition == NULL || num_coils > MAX_COILS) {
        return;
    }

    memset(ignition, 0, sizeof(ignition_control_t));

    ignition->num_coils = num_coils;
    ignition->mode = mode;
    ignition->max_dwell_us = (max_dwell_us > 0) ? max_dwell_us : DEFAULT_MAX_DWELL_US;

    // Initialize all coils as disabled
    for (int i = 0; i < num_coils; i++) {
        ignition->config[i].enabled = false;
        ignition->config[i].invert_polarity = false;
        ignition->config[i].paired_cylinder = 255;  // No pairing
        ignition->state[i] = COIL_STATE_IDLE;
    }

    ignition->initialized = true;
}

/**
 * @brief Configure a coil
 */
void ignition_configure(ignition_control_t* ignition,
                        uint8_t cylinder,
                        uint8_t pin,
                        uint8_t paired_cylinder,
                        bool invert_polarity)
{
    if (ignition == NULL || cylinder >= ignition->num_coils) {
        return;
    }

    ignition->config[cylinder].pin = pin;
    ignition->config[cylinder].paired_cylinder = paired_cylinder;
    ignition->config[cylinder].invert_polarity = invert_polarity;

    // Setup pin as output
    pinMode(pin, OUTPUT);

    // Set initial state (off/idle)
    uint8_t idle_level = invert_polarity ? HIGH : LOW;
    digitalWrite(pin, idle_level);
}

/**
 * @brief Enable/disable a coil
 */
void ignition_enable(ignition_control_t* ignition,
                     uint8_t cylinder,
                     bool enable)
{
    if (ignition == NULL || cylinder >= ignition->num_coils) {
        return;
    }

    ignition->config[cylinder].enabled = enable;

    // If disabling, make sure coil is off
    if (!enable && ignition->state[cylinder] == COIL_STATE_CHARGING) {
        // Emergency shut off
        uint8_t idle_level = ignition->config[cylinder].invert_polarity ? HIGH : LOW;
        digitalWrite(ignition->config[cylinder].pin, idle_level);
        ignition->state[cylinder] = COIL_STATE_IDLE;
    }
}

/**
 * @brief Start charging coil
 */
void ignition_charge_start(ignition_control_t* ignition,
                           uint8_t cylinder,
                           uint32_t timestamp)
{
    if (ignition == NULL || cylinder >= ignition->num_coils) {
        return;
    }

    if (!ignition->config[cylinder].enabled) {
        return;
    }

    // If already charging, don't restart
    if (ignition->state[cylinder] == COIL_STATE_CHARGING) {
        return;
    }

    // Turn coil ON (start charging)
    uint8_t charge_level = ignition->config[cylinder].invert_polarity ? LOW : HIGH;
    digitalWrite(ignition->config[cylinder].pin, charge_level);

    // Update state
    ignition->state[cylinder] = COIL_STATE_CHARGING;
    ignition->charge_start_time[cylinder] = timestamp;
}

/**
 * @brief Fire spark
 */
void ignition_fire(ignition_control_t* ignition,
                   uint8_t cylinder,
                   uint32_t timestamp)
{
    if (ignition == NULL || cylinder >= ignition->num_coils) {
        return;
    }

    if (!ignition->config[cylinder].enabled) {
        return;
    }

    // Turn coil OFF (fire spark)
    uint8_t idle_level = ignition->config[cylinder].invert_polarity ? HIGH : LOW;
    digitalWrite(ignition->config[cylinder].pin, idle_level);

    // Update state
    ignition->state[cylinder] = COIL_STATE_FIRED;

    // Update statistics
    if (ignition->charge_start_time[cylinder] > 0) {
        uint32_t dwell = timestamp - ignition->charge_start_time[cylinder];

        ignition->stats[cylinder].total_sparks++;
        ignition->stats[cylinder].total_dwell_us += dwell;
        ignition->stats[cylinder].last_dwell_us = dwell;
        ignition->stats[cylinder].last_charge_time = ignition->charge_start_time[cylinder];
        ignition->stats[cylinder].last_fire_time = timestamp;

        if (dwell > ignition->stats[cylinder].max_dwell_us) {
            ignition->stats[cylinder].max_dwell_us = dwell;
        }
    }

    // Reset to idle after a short delay (handled by next cycle)
    ignition->state[cylinder] = COIL_STATE_IDLE;
}

/**
 * @brief Check if coil is charging
 */
bool ignition_is_charging(const ignition_control_t* ignition,
                          uint8_t cylinder)
{
    if (ignition == NULL || cylinder >= ignition->num_coils) {
        return false;
    }

    return (ignition->state[cylinder] == COIL_STATE_CHARGING);
}

/**
 * @brief Get current dwell time
 */
uint32_t ignition_get_current_dwell(const ignition_control_t* ignition,
                                    uint8_t cylinder,
                                    uint32_t current_time)
{
    if (ignition == NULL || cylinder >= ignition->num_coils) {
        return 0;
    }

    if (ignition->state[cylinder] != COIL_STATE_CHARGING) {
        return 0;
    }

    if (ignition->charge_start_time[cylinder] == 0) {
        return 0;
    }

    return current_time - ignition->charge_start_time[cylinder];
}

/**
 * @brief Set default pin configuration
 */
void ignition_set_default_pins(ignition_control_t* ignition)
{
    if (ignition == NULL) {
        return;
    }

    // Default pin assignments for up to 8 cylinders
    const uint8_t default_pins[MAX_COILS] = {
        6,   // Cylinder 1 → Pin 6
        7,   // Cylinder 2 → Pin 7
        8,   // Cylinder 3 → Pin 8
        9,   // Cylinder 4 → Pin 9
        10,  // Cylinder 5 → Pin 10
        11,  // Cylinder 6 → Pin 11
        12,  // Cylinder 7 → Pin 12
        24   // Cylinder 8 → Pin 24
    };

    for (int i = 0; i < ignition->num_coils; i++) {
        uint8_t paired = 255;  // No pairing by default

        // For wasted spark mode, pair cylinders
        if (ignition->mode == IGNITION_MODE_WASTED_SPARK) {
            // Typical pairing: 1-4, 2-3 for 4-cylinder
            if (ignition->num_coils == 4) {
                if (i == 0) paired = 3;      // Cyl 1 paired with 4
                else if (i == 1) paired = 2; // Cyl 2 paired with 3
                else if (i == 2) paired = 1; // Cyl 3 paired with 2
                else if (i == 3) paired = 0; // Cyl 4 paired with 1
            }
        }

        ignition_configure(ignition, i, default_pins[i], paired, false);
        ignition_enable(ignition, i, true);
    }
}

/**
 * @brief Get coil statistics
 */
const coil_stats_t* ignition_get_stats(const ignition_control_t* ignition,
                                        uint8_t cylinder)
{
    if (ignition == NULL || cylinder >= ignition->num_coils) {
        return NULL;
    }

    return &ignition->stats[cylinder];
}

/**
 * @brief Get total spark count
 */
uint32_t ignition_get_total_sparks(const ignition_control_t* ignition)
{
    if (ignition == NULL) {
        return 0;
    }

    uint32_t total = 0;
    for (int i = 0; i < ignition->num_coils; i++) {
        total += ignition->stats[i].total_sparks;
    }

    return total;
}

/**
 * @brief Reset statistics
 */
void ignition_reset_stats(ignition_control_t* ignition)
{
    if (ignition == NULL) {
        return;
    }

    for (int i = 0; i < ignition->num_coils; i++) {
        memset(&ignition->stats[i], 0, sizeof(coil_stats_t));
    }
}

/**
 * @brief Emergency shutdown
 */
void ignition_emergency_shutdown(ignition_control_t* ignition)
{
    if (ignition == NULL) {
        return;
    }

    // Turn off all coils immediately
    for (int i = 0; i < ignition->num_coils; i++) {
        if (ignition->config[i].enabled) {
            uint8_t idle_level = ignition->config[i].invert_polarity ? HIGH : LOW;
            digitalWrite(ignition->config[i].pin, idle_level);
            ignition->state[i] = COIL_STATE_IDLE;
        }
    }
}

/**
 * @brief Check for over-dwell condition
 */
uint8_t ignition_check_over_dwell(ignition_control_t* ignition,
                                  uint32_t current_time)
{
    if (ignition == NULL) {
        return 255;
    }

    for (int i = 0; i < ignition->num_coils; i++) {
        if (ignition->state[i] == COIL_STATE_CHARGING) {
            uint32_t dwell = ignition_get_current_dwell(ignition, i, current_time);

            if (dwell > ignition->max_dwell_us) {
                // Over-dwell detected! Fire immediately to prevent coil damage
                ignition_fire(ignition, i, current_time);
                return i;
            }
        }
    }

    return 255;  // No over-dwell
}
