/**
 * @file injector_control.c
 * @brief Fuel Injector Control System Implementation
 *
 * @version 2.5.0
 * @date 2026-02-12
 */

#include "injector_control.h"
#include <string.h>

// Arduino/Teensy digital I/O (available when compiled with Arduino)
#ifdef ARDUINO
    #include <Arduino.h>
#else
    // Stub functions for testing without Arduino
    static void pinMode(uint8_t pin, uint8_t mode) { (void)pin; (void)mode; }
    static void digitalWrite(uint8_t pin, uint8_t val) { (void)pin; (void)val; }
    #define OUTPUT 1
    #define HIGH 1
    #define LOW 0
#endif

/**
 * @brief Initialize injector control system
 */
void injector_control_init(injector_control_t* injectors,
                           uint8_t num_injectors,
                           injection_mode_t mode)
{
    if (injectors == NULL || num_injectors > MAX_INJECTORS) {
        return;
    }

    memset(injectors, 0, sizeof(injector_control_t));

    injectors->num_injectors = num_injectors;
    injectors->mode = mode;

    // Initialize all injectors as disabled
    for (int i = 0; i < num_injectors; i++) {
        injectors->config[i].enabled = false;
        injectors->config[i].invert_polarity = false;
        injectors->config[i].bank = i % 2;  // Alternate banks (0, 1, 0, 1, ...)
        injectors->state[i] = INJECTOR_STATE_IDLE;
    }

    injectors->initialized = true;
}

/**
 * @brief Configure an injector
 */
void injector_configure(injector_control_t* injectors,
                        uint8_t cylinder,
                        uint8_t pin,
                        uint8_t bank,
                        bool invert_polarity)
{
    if (injectors == NULL || cylinder >= injectors->num_injectors) {
        return;
    }

    injectors->config[cylinder].pin = pin;
    injectors->config[cylinder].bank = bank;
    injectors->config[cylinder].invert_polarity = invert_polarity;

    // Setup pin as output
    pinMode(pin, OUTPUT);

    // Set initial state (off)
    uint8_t off_level = invert_polarity ? HIGH : LOW;
    digitalWrite(pin, off_level);
}

/**
 * @brief Enable/disable an injector
 */
void injector_enable(injector_control_t* injectors,
                     uint8_t cylinder,
                     bool enable)
{
    if (injectors == NULL || cylinder >= injectors->num_injectors) {
        return;
    }

    injectors->config[cylinder].enabled = enable;

    // If disabling, make sure it's off
    if (!enable && injectors->state[cylinder] == INJECTOR_STATE_ACTIVE) {
        injector_off(injectors, cylinder, 0);
    }
}

/**
 * @brief Turn on injector
 */
void injector_on(injector_control_t* injectors,
                 uint8_t cylinder,
                 uint32_t timestamp)
{
    if (injectors == NULL || cylinder >= injectors->num_injectors) {
        return;
    }

    if (!injectors->config[cylinder].enabled) {
        return;
    }

    // Set pin high (or low if inverted)
    uint8_t on_level = injectors->config[cylinder].invert_polarity ? LOW : HIGH;
    digitalWrite(injectors->config[cylinder].pin, on_level);

    // Update state
    injectors->state[cylinder] = INJECTOR_STATE_ACTIVE;
    injectors->start_time[cylinder] = timestamp;
}

/**
 * @brief Turn off injector
 */
void injector_off(injector_control_t* injectors,
                  uint8_t cylinder,
                  uint32_t timestamp)
{
    if (injectors == NULL || cylinder >= injectors->num_injectors) {
        return;
    }

    if (!injectors->config[cylinder].enabled) {
        return;
    }

    // Set pin low (or high if inverted)
    uint8_t off_level = injectors->config[cylinder].invert_polarity ? HIGH : LOW;
    digitalWrite(injectors->config[cylinder].pin, off_level);

    // Update state
    injectors->state[cylinder] = INJECTOR_STATE_IDLE;

    // Update statistics
    if (injectors->start_time[cylinder] > 0) {
        uint32_t duration = timestamp - injectors->start_time[cylinder];
        injectors->stats[cylinder].total_pulses++;
        injectors->stats[cylinder].total_duration_us += duration;
        injectors->stats[cylinder].last_duration_us = duration;
        injectors->stats[cylinder].last_start_time = injectors->start_time[cylinder];
    }
}

/**
 * @brief Check if injector is active
 */
bool injector_is_active(const injector_control_t* injectors,
                        uint8_t cylinder)
{
    if (injectors == NULL || cylinder >= injectors->num_injectors) {
        return false;
    }

    return (injectors->state[cylinder] == INJECTOR_STATE_ACTIVE);
}

/**
 * @brief Set default pin configuration
 */
void injector_set_default_pins(injector_control_t* injectors)
{
    if (injectors == NULL) {
        return;
    }

    // Default pin assignments for up to 8 cylinders
    const uint8_t default_pins[MAX_INJECTORS] = {
        2,  // Cylinder 1 → Pin 2
        3,  // Cylinder 2 → Pin 3
        4,  // Cylinder 3 → Pin 4
        5,  // Cylinder 4 → Pin 5
        6,  // Cylinder 5 → Pin 6
        7,  // Cylinder 6 → Pin 7
        8,  // Cylinder 7 → Pin 8
        9   // Cylinder 8 → Pin 9
    };

    for (int i = 0; i < injectors->num_injectors; i++) {
        injector_configure(injectors, i, default_pins[i], i % 2, false);
        injector_enable(injectors, i, true);
    }
}

/**
 * @brief Get injector statistics
 */
const injector_stats_t* injector_get_stats(const injector_control_t* injectors,
                                            uint8_t cylinder)
{
    if (injectors == NULL || cylinder >= injectors->num_injectors) {
        return NULL;
    }

    return &injectors->stats[cylinder];
}

/**
 * @brief Get total injection time
 */
uint32_t injector_get_total_time(const injector_control_t* injectors)
{
    if (injectors == NULL) {
        return 0;
    }

    uint32_t total = 0;
    for (int i = 0; i < injectors->num_injectors; i++) {
        total += injectors->stats[i].total_duration_us;
    }

    return total;
}

/**
 * @brief Reset statistics
 */
void injector_reset_stats(injector_control_t* injectors)
{
    if (injectors == NULL) {
        return;
    }

    for (int i = 0; i < injectors->num_injectors; i++) {
        memset(&injectors->stats[i], 0, sizeof(injector_stats_t));
    }
}

/**
 * @brief Emergency shutdown
 */
void injector_emergency_shutdown(injector_control_t* injectors)
{
    if (injectors == NULL) {
        return;
    }

    // Turn off all injectors immediately
    for (int i = 0; i < injectors->num_injectors; i++) {
        if (injectors->config[i].enabled) {
            uint8_t off_level = injectors->config[i].invert_polarity ? HIGH : LOW;
            digitalWrite(injectors->config[i].pin, off_level);
            injectors->state[i] = INJECTOR_STATE_IDLE;
        }
    }
}
