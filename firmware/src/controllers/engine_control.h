/**
 * @file engine_control.h
 * @brief Engine control structures and basic algorithms
 * @version 1.0.0
 * @date 2026-02-10
 *
 * This file provides the core engine control data structures and
 * basic fuel/ignition algorithms for the Teensy 3.5 ECU.
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#ifndef ENGINE_CONTROL_H
#define ENGINE_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// Engine Configuration
//=============================================================================

typedef struct {
    uint8_t num_cylinders;       // Number of cylinders (4, 6, 8)
    uint16_t displacement_cc;    // Engine displacement in cc
    uint16_t crank_teeth;        // Crank wheel teeth (e.g., 36)
    uint16_t missing_teeth;      // Missing teeth (e.g., 1 for 36-1)
    uint8_t firing_order[8];     // Firing order (e.g., {1,3,4,2} for 4-cyl)
} engine_config_t;

//=============================================================================
// Sensor Data
//=============================================================================

typedef struct {
    // Analog sensors (voltage)
    float tps_voltage;           // Throttle position (0-5V)
    float map_voltage;           // Manifold pressure (0-5V)
    float clt_voltage;           // Coolant temperature (0-5V)
    float iat_voltage;           // Intake air temperature (0-5V)
    float o2_voltage;            // Oxygen sensor (0-1V)
    float battery_voltage;       // Battery voltage (0-16V)

    // Processed sensor values
    float tps_percent;           // Throttle position (0-100%)
    float map_kpa;               // Manifold pressure (kPa)
    float clt_celsius;           // Coolant temp (°C)
    float iat_celsius;           // Intake air temp (°C)
    float afr;                   // Air-fuel ratio (14.7 = stoich)

    // Engine state
    uint16_t rpm;                // Engine RPM
    uint16_t current_tooth;      // Current crank position
    bool engine_running;         // Engine running flag
    bool sync_locked;            // Position sync status
} sensor_data_t;

//=============================================================================
// Fuel Control
//=============================================================================

typedef struct {
    uint32_t base_pulse_us;      // Base injection pulse width (µs)
    float ve_table[16][16];      // Volumetric efficiency table
    float afr_target;            // Target air-fuel ratio
    float fuel_pressure_kpa;     // Fuel pressure (kPa)
    float injector_flow_cc;      // Injector flow rate (cc/min)

    // Corrections
    float clt_correction;        // Coolant temp correction
    float iat_correction;        // Intake air temp correction
    float accel_enrichment;      // Acceleration enrichment
    float o2_correction;         // Closed-loop O2 correction
} fuel_control_t;

//=============================================================================
// Ignition Control
//=============================================================================

typedef struct {
    uint8_t base_timing_deg;     // Base timing (degrees BTDC)
    float timing_table[16][16];  // Timing advance table
    uint16_t dwell_time_us;      // Coil dwell time (µs)

    // Corrections
    float clt_advance;           // Coolant temp advance
    float iat_advance;           // Intake air temp advance
    float knock_retard;          // Knock sensor retard
} ignition_control_t;

//=============================================================================
// ECU State
//=============================================================================

typedef struct {
    engine_config_t config;
    sensor_data_t sensors;
    fuel_control_t fuel;
    ignition_control_t ignition;

    // Runtime state
    uint32_t loop_count;         // Main loop iterations
    uint32_t last_update_ms;     // Last sensor update timestamp
    bool error_state;            // Error flag
} ecu_state_t;

//=============================================================================
// Function Prototypes
//=============================================================================

/**
 * @brief Initialize ECU with engine configuration
 *
 * @param ecu Pointer to ECU state structure
 * @param config Pointer to engine configuration
 */
void ecu_init(ecu_state_t* ecu, const engine_config_t* config);

/**
 * @brief Update sensor readings
 *
 * Reads all analog sensors and processes values
 *
 * @param ecu Pointer to ECU state
 */
void ecu_update_sensors(ecu_state_t* ecu);

/**
 * @brief Calculate fuel injection pulse width
 *
 * @param ecu Pointer to ECU state
 * @return Injection pulse width in microseconds
 */
uint32_t calculate_fuel_pulse(ecu_state_t* ecu);

/**
 * @brief Calculate ignition timing advance
 *
 * @param ecu Pointer to ECU state
 * @return Ignition timing in degrees BTDC
 */
uint8_t calculate_ignition_timing(ecu_state_t* ecu);

/**
 * @brief Convert TPS voltage to percentage
 *
 * @param voltage TPS voltage (0-5V)
 * @return Throttle position (0-100%)
 */
float convert_tps_voltage(float voltage);

/**
 * @brief Convert MAP voltage to kPa
 *
 * @param voltage MAP sensor voltage (0-5V)
 * @return Manifold pressure in kPa
 */
float convert_map_voltage(float voltage);

/**
 * @brief Convert temperature sensor voltage to Celsius
 *
 * Uses NTC thermistor curve (typically GM sensors)
 *
 * @param voltage Sensor voltage (0-5V)
 * @return Temperature in Celsius
 */
float convert_temp_voltage(float voltage);

/**
 * @brief Convert O2 sensor voltage to AFR
 *
 * @param voltage O2 sensor voltage (0-1V for narrowband)
 * @return Air-fuel ratio
 */
float convert_o2_voltage(float voltage);

/**
 * @brief Lookup value from 2D table
 *
 * Performs bilinear interpolation on lookup table
 *
 * @param table Pointer to 16x16 table
 * @param x X-axis value (e.g., RPM)
 * @param y Y-axis value (e.g., MAP)
 * @param x_min Minimum X value
 * @param x_max Maximum X value
 * @param y_min Minimum Y value
 * @param y_max Maximum Y value
 * @return Interpolated value from table
 */
float lookup_table_2d(const float table[16][16],
                     float x, float y,
                     float x_min, float x_max,
                     float y_min, float y_max);

#endif // ENGINE_CONTROL_H
