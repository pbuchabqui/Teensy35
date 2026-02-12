/**
 * @file ignition_control.h
 * @brief Ignition Coil Control System for Teensy 3.5
 *
 * Controls ignition coils with support for:
 * - Individual coil-per-cylinder (COP)
 * - Wasted spark (paired coils)
 * - Distributor (single coil)
 * - Dwell time control
 * - Spark timing
 * - Individual pin configuration
 * - Ignition statistics
 *
 * Based on rusEFI IgnitionController.
 *
 * Ignition sequence:
 * 1. Charge coil (dwell) - turn coil ON to build magnetic field
 * 2. Fire spark (trigger) - turn coil OFF to collapse field and generate spark
 *
 * @version 2.5.0
 * @date 2026-02-12
 */

#ifndef IGNITION_CONTROL_H
#define IGNITION_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of coils
#define MAX_COILS  8

/**
 * @brief Ignition mode
 */
typedef enum {
    IGNITION_MODE_INDIVIDUAL = 0,   ///< Individual coil per cylinder (COP)
    IGNITION_MODE_WASTED_SPARK,     ///< Wasted spark (paired coils)
    IGNITION_MODE_DISTRIBUTOR,      ///< Single coil with distributor
} ignition_mode_t;

/**
 * @brief Coil state
 */
typedef enum {
    COIL_STATE_IDLE = 0,            ///< Coil off
    COIL_STATE_CHARGING,            ///< Coil charging (dwell)
    COIL_STATE_FIRED,               ///< Spark fired
} coil_state_t;

/**
 * @brief Coil configuration
 */
typedef struct {
    uint8_t pin;                    ///< Output pin
    bool enabled;                   ///< Coil enabled
    bool invert_polarity;           ///< Invert output (false = charge high, fire low)
    uint8_t paired_cylinder;        ///< Paired cylinder for wasted spark (255 = none)
} coil_config_t;

/**
 * @brief Coil statistics
 */
typedef struct {
    uint32_t total_sparks;          ///< Total spark events
    uint32_t total_dwell_us;        ///< Total dwell time (µs)
    uint32_t last_dwell_us;         ///< Last dwell duration
    uint32_t last_charge_time;      ///< Last charge start timestamp
    uint32_t last_fire_time;        ///< Last fire timestamp
    uint32_t max_dwell_us;          ///< Maximum dwell seen
} coil_stats_t;

/**
 * @brief Ignition control system
 */
typedef struct {
    // Configuration
    uint8_t num_coils;              ///< Number of coils
    ignition_mode_t mode;           ///< Ignition mode
    coil_config_t config[MAX_COILS];

    // Current state
    coil_state_t state[MAX_COILS];
    uint32_t charge_start_time[MAX_COILS];

    // Statistics
    coil_stats_t stats[MAX_COILS];

    // Safety limits
    uint32_t max_dwell_us;          ///< Maximum allowed dwell time

    // Flags
    bool initialized;

} ignition_control_t;

/**
 * @brief Initialize ignition control system
 *
 * @param ignition Pointer to ignition control structure
 * @param num_coils Number of coils
 * @param mode Ignition mode
 * @param max_dwell_us Maximum dwell time (safety limit)
 */
void ignition_control_init(ignition_control_t* ignition,
                           uint8_t num_coils,
                           ignition_mode_t mode,
                           uint32_t max_dwell_us);

/**
 * @brief Configure a coil
 *
 * @param ignition Pointer to ignition control structure
 * @param cylinder Cylinder/coil number (0-based)
 * @param pin Output pin
 * @param paired_cylinder Paired cylinder for wasted spark (255 = none)
 * @param invert_polarity true to invert output
 */
void ignition_configure(ignition_control_t* ignition,
                        uint8_t cylinder,
                        uint8_t pin,
                        uint8_t paired_cylinder,
                        bool invert_polarity);

/**
 * @brief Enable/disable a coil
 *
 * @param ignition Pointer to ignition control structure
 * @param cylinder Cylinder number
 * @param enable true to enable
 */
void ignition_enable(ignition_control_t* ignition,
                     uint8_t cylinder,
                     bool enable);

/**
 * @brief Start charging coil (dwell start)
 *
 * Turns coil ON to begin building magnetic field.
 *
 * @param ignition Pointer to ignition control structure
 * @param cylinder Cylinder number
 * @param timestamp Current timestamp (microseconds)
 */
void ignition_charge_start(ignition_control_t* ignition,
                           uint8_t cylinder,
                           uint32_t timestamp);

/**
 * @brief Fire spark (dwell end, spark trigger)
 *
 * Turns coil OFF to collapse magnetic field and generate spark.
 *
 * @param ignition Pointer to ignition control structure
 * @param cylinder Cylinder number
 * @param timestamp Current timestamp (microseconds)
 */
void ignition_fire(ignition_control_t* ignition,
                   uint8_t cylinder,
                   uint32_t timestamp);

/**
 * @brief Check if coil is charging
 *
 * @param ignition Pointer to ignition control structure
 * @param cylinder Cylinder number
 * @return true if coil is charging
 */
bool ignition_is_charging(const ignition_control_t* ignition,
                          uint8_t cylinder);

/**
 * @brief Get current dwell time for active coil
 *
 * @param ignition Pointer to ignition control structure
 * @param cylinder Cylinder number
 * @param current_time Current timestamp
 * @return Current dwell duration (µs), or 0 if not charging
 */
uint32_t ignition_get_current_dwell(const ignition_control_t* ignition,
                                    uint8_t cylinder,
                                    uint32_t current_time);

/**
 * @brief Set default pin configuration for common setups
 *
 * Default pin assignments (4-cylinder):
 * - Cyl 1: Pin 6
 * - Cyl 2: Pin 7
 * - Cyl 3: Pin 8
 * - Cyl 4: Pin 9
 *
 * @param ignition Pointer to ignition control structure
 */
void ignition_set_default_pins(ignition_control_t* ignition);

/**
 * @brief Get coil statistics
 *
 * @param ignition Pointer to ignition control structure
 * @param cylinder Cylinder number
 * @return Pointer to statistics structure (NULL if invalid)
 */
const coil_stats_t* ignition_get_stats(const ignition_control_t* ignition,
                                        uint8_t cylinder);

/**
 * @brief Get total spark count (all coils)
 *
 * @param ignition Pointer to ignition control structure
 * @return Total number of sparks
 */
uint32_t ignition_get_total_sparks(const ignition_control_t* ignition);

/**
 * @brief Reset statistics
 *
 * @param ignition Pointer to ignition control structure
 */
void ignition_reset_stats(ignition_control_t* ignition);

/**
 * @brief Emergency shutdown (turn off all coils)
 *
 * @param ignition Pointer to ignition control structure
 */
void ignition_emergency_shutdown(ignition_control_t* ignition);

/**
 * @brief Check for over-dwell condition
 *
 * Checks if any coil has been charging too long (safety check).
 *
 * @param ignition Pointer to ignition control structure
 * @param current_time Current timestamp
 * @return Cylinder number with over-dwell, or 255 if none
 */
uint8_t ignition_check_over_dwell(ignition_control_t* ignition,
                                  uint32_t current_time);

#ifdef __cplusplus
}
#endif

#endif // IGNITION_CONTROL_H
