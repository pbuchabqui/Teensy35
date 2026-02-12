/**
 * @file injector_control.h
 * @brief Fuel Injector Control System for Teensy 3.5
 *
 * Controls fuel injectors with support for:
 * - Sequential injection (each cylinder independent)
 * - Batch injection (banks)
 * - Simultaneous injection (all cylinders)
 * - Individual pin configuration
 * - Injection statistics
 *
 * Based on rusEFI InjectorController.
 *
 * @version 2.5.0
 * @date 2026-02-12
 */

#ifndef INJECTOR_CONTROL_H
#define INJECTOR_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of injectors (cylinders)
#define MAX_INJECTORS  8

/**
 * @brief Injection mode
 */
typedef enum {
    INJECTION_MODE_SEQUENTIAL = 0,  ///< Sequential (each cylinder separate)
    INJECTION_MODE_BATCH,           ///< Batch (alternating banks)
    INJECTION_MODE_SIMULTANEOUS,    ///< Simultaneous (all at once)
} injection_mode_t;

/**
 * @brief Injector state
 */
typedef enum {
    INJECTOR_STATE_IDLE = 0,        ///< Injector off
    INJECTOR_STATE_ACTIVE,          ///< Injector on
} injector_state_t;

/**
 * @brief Injector configuration
 */
typedef struct {
    uint8_t pin;                    ///< Output pin
    bool enabled;                   ///< Injector enabled
    bool invert_polarity;           ///< Invert output (false = active high)
    uint8_t bank;                   ///< Bank number (0 or 1 for batch mode)
} injector_config_t;

/**
 * @brief Injector statistics
 */
typedef struct {
    uint32_t total_pulses;          ///< Total injection pulses
    uint32_t total_duration_us;     ///< Total injection time (µs)
    uint32_t last_duration_us;      ///< Last pulse duration
    uint32_t last_start_time;       ///< Last pulse start timestamp
} injector_stats_t;

/**
 * @brief Injector control system
 */
typedef struct {
    // Configuration
    uint8_t num_injectors;          ///< Number of injectors
    injection_mode_t mode;          ///< Injection mode
    injector_config_t config[MAX_INJECTORS];

    // Current state
    injector_state_t state[MAX_INJECTORS];
    uint32_t start_time[MAX_INJECTORS];

    // Statistics
    injector_stats_t stats[MAX_INJECTORS];

    // Flags
    bool initialized;

} injector_control_t;

/**
 * @brief Initialize injector control system
 *
 * @param injectors Pointer to injector control structure
 * @param num_injectors Number of injectors (cylinders)
 * @param mode Injection mode
 */
void injector_control_init(injector_control_t* injectors,
                           uint8_t num_injectors,
                           injection_mode_t mode);

/**
 * @brief Configure an injector
 *
 * @param injectors Pointer to injector control structure
 * @param cylinder Cylinder/injector number (0-based)
 * @param pin Output pin
 * @param bank Bank number (0 or 1, for batch mode)
 * @param invert_polarity true to invert output
 */
void injector_configure(injector_control_t* injectors,
                        uint8_t cylinder,
                        uint8_t pin,
                        uint8_t bank,
                        bool invert_polarity);

/**
 * @brief Enable/disable an injector
 *
 * @param injectors Pointer to injector control structure
 * @param cylinder Cylinder number
 * @param enable true to enable
 */
void injector_enable(injector_control_t* injectors,
                     uint8_t cylinder,
                     bool enable);

/**
 * @brief Turn on injector (start injection)
 *
 * @param injectors Pointer to injector control structure
 * @param cylinder Cylinder number
 * @param timestamp Current timestamp (microseconds)
 */
void injector_on(injector_control_t* injectors,
                 uint8_t cylinder,
                 uint32_t timestamp);

/**
 * @brief Turn off injector (end injection)
 *
 * @param injectors Pointer to injector control structure
 * @param cylinder Cylinder number
 * @param timestamp Current timestamp (microseconds)
 */
void injector_off(injector_control_t* injectors,
                  uint8_t cylinder,
                  uint32_t timestamp);

/**
 * @brief Check if injector is active
 *
 * @param injectors Pointer to injector control structure
 * @param cylinder Cylinder number
 * @return true if injector is on
 */
bool injector_is_active(const injector_control_t* injectors,
                        uint8_t cylinder);

/**
 * @brief Set default pin configuration for common setups
 *
 * Default pin assignments (4-cylinder):
 * - Cyl 1: Pin 2
 * - Cyl 2: Pin 3
 * - Cyl 3: Pin 4
 * - Cyl 4: Pin 5
 *
 * @param injectors Pointer to injector control structure
 */
void injector_set_default_pins(injector_control_t* injectors);

/**
 * @brief Get injector statistics
 *
 * @param injectors Pointer to injector control structure
 * @param cylinder Cylinder number
 * @return Pointer to statistics structure (NULL if invalid)
 */
const injector_stats_t* injector_get_stats(const injector_control_t* injectors,
                                            uint8_t cylinder);

/**
 * @brief Get total injection time (all cylinders)
 *
 * @param injectors Pointer to injector control structure
 * @return Total injection time in microseconds
 */
uint32_t injector_get_total_time(const injector_control_t* injectors);

/**
 * @brief Reset statistics
 *
 * @param injectors Pointer to injector control structure
 */
void injector_reset_stats(injector_control_t* injectors);

/**
 * @brief Emergency shutdown (turn off all injectors)
 *
 * @param injectors Pointer to injector control structure
 */
void injector_emergency_shutdown(injector_control_t* injectors);

#ifdef __cplusplus
}
#endif

#endif // INJECTOR_CONTROL_H
