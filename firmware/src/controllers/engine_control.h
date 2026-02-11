/**
 * @file engine_control.h
 * @brief rusEFI-compatible engine control structures and algorithms
 * @version 2.0.0
 * @date 2026-02-11
 *
 * This file provides rusEFI-compatible engine control with:
 * - Injector latency compensation tables
 * - Dwell time scheduling based on battery voltage
 * - Wall wetting/transient fuel compensation
 * - Sequential fuel and ignition timing per cylinder
 * - Sensor diagnostics (open/short circuit detection)
 * - PI-based closed-loop O2 feedback control
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

// Sensor diagnostics (rusEFI-compatible)
typedef struct {
    bool tps_fault;              // TPS open/short circuit
    bool map_fault;              // MAP sensor fault
    bool clt_fault;              // CLT sensor fault
    bool iat_fault;              // IAT sensor fault
    bool o2_fault;               // O2 sensor fault
    bool battery_fault;          // Battery voltage fault
    uint16_t fault_code;         // DTC fault code
} sensor_diagnostics_t;

// Closed-loop O2 control (rusEFI-compatible)
typedef struct {
    float proportional_gain;     // P gain for O2 control
    float integral_gain;         // I gain for O2 control
    float integral_error;        // Accumulated integral error
    float correction;            // Current O2 correction factor
    bool closed_loop_active;     // Closed-loop enabled flag
} closed_loop_fuel_t;

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

    // rusEFI-compatible diagnostics
    sensor_diagnostics_t diagnostics;

    // rusEFI-compatible closed-loop control
    closed_loop_fuel_t closed_loop;
} sensor_data_t;

//=============================================================================
// Fuel Control
//=============================================================================

// Injector latency compensation (rusEFI-compatible)
typedef struct {
    float voltage[8];            // Battery voltage breakpoints (V)
    float latency_us[8];         // Injector latency at each voltage (µs)
} injector_latency_table_t;

// Wall wetting compensation (rusEFI-compatible)
typedef struct {
    float tau;                   // Time constant for fuel film (ms)
    float beta;                  // Fraction of fuel that sticks to wall (0-1)
    float fuel_film_mass;        // Current fuel film mass estimate (mg)
    float prev_map_kpa;          // Previous MAP for delta calculation
} wall_wetting_t;

typedef struct {
    uint32_t base_pulse_us;      // Base injection pulse width (µs)
    float ve_table[16][16];      // Volumetric efficiency table
    float afr_target;            // Target air-fuel ratio
    float fuel_pressure_kpa;     // Fuel pressure (kPa)
    float injector_flow_cc;      // Injector flow rate (cc/min)

    // rusEFI-compatible injector compensation
    injector_latency_table_t latency_table;

    // rusEFI-compatible wall wetting
    wall_wetting_t wall_wetting;

    // Corrections
    float clt_correction;        // Coolant temp correction
    float iat_correction;        // Intake air temp correction
    float accel_enrichment;      // Acceleration enrichment
    float o2_correction;         // Closed-loop O2 correction

    // Sequential injection state (per-cylinder)
    uint32_t cylinder_pulse_us[8];  // Individual cylinder pulse widths
    uint8_t next_injection_cylinder; // Next cylinder to inject
} fuel_control_t;

//=============================================================================
// Ignition Control
//=============================================================================

// Dwell time table (rusEFI-compatible)
typedef struct {
    float voltage[8];            // Battery voltage breakpoints (V)
    float dwell_us[8];           // Dwell time at each voltage (µs)
} dwell_table_t;

typedef struct {
    uint8_t base_timing_deg;     // Base timing (degrees BTDC)
    float timing_table[16][16];  // Timing advance table
    uint16_t dwell_time_us;      // Coil dwell time (µs)

    // rusEFI-compatible dwell scheduling
    dwell_table_t dwell_table;

    // Corrections
    float clt_advance;           // Coolant temp advance
    float iat_advance;           // Intake air temp advance
    float knock_retard;          // Knock sensor retard

    // Sequential ignition state (per-cylinder)
    uint8_t cylinder_timing_deg[8];  // Individual cylinder timing
    uint8_t next_spark_cylinder;     // Next cylinder to spark
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

//=============================================================================
// rusEFI-Compatible Advanced Functions
//=============================================================================

/**
 * @brief Calculate injector latency compensation
 *
 * Compensates for injector opening/closing delay based on battery voltage
 *
 * @param table Pointer to latency table
 * @param battery_voltage Current battery voltage
 * @return Latency compensation in microseconds
 */
float calculate_injector_latency(const injector_latency_table_t* table,
                                 float battery_voltage);

/**
 * @brief Calculate dwell time based on battery voltage
 *
 * Ensures proper coil saturation across voltage range
 *
 * @param table Pointer to dwell table
 * @param battery_voltage Current battery voltage
 * @return Dwell time in microseconds
 */
float calculate_dwell_time(const dwell_table_t* table,
                          float battery_voltage);

/**
 * @brief Update wall wetting compensation
 *
 * Implements transient fuel compensation for throttle changes
 *
 * @param ww Pointer to wall wetting structure
 * @param base_fuel_mg Base fuel mass in mg
 * @param map_kpa Current manifold pressure
 * @param dt Time delta in milliseconds
 * @return Compensated fuel mass in mg
 */
float update_wall_wetting(wall_wetting_t* ww, float base_fuel_mg,
                         float map_kpa, float dt);

/**
 * @brief Update closed-loop O2 control
 *
 * PI controller for air-fuel ratio correction
 *
 * @param cl Pointer to closed-loop structure
 * @param target_afr Target air-fuel ratio
 * @param actual_afr Measured air-fuel ratio
 * @param dt Time delta in seconds
 */
void update_closed_loop_fuel(closed_loop_fuel_t* cl, float target_afr,
                            float actual_afr, float dt);

/**
 * @brief Diagnose sensor faults
 *
 * Detects open/short circuits and out-of-range conditions
 *
 * @param sensors Pointer to sensor data structure
 */
void diagnose_sensors(sensor_data_t* sensors);

/**
 * @brief Calculate sequential injection timing
 *
 * Determines injection timing for each cylinder based on firing order
 *
 * @param ecu Pointer to ECU state
 * @param cylinder Cylinder number (0-based)
 * @return Injection timing in crank degrees
 */
float calculate_injection_timing(ecu_state_t* ecu, uint8_t cylinder);

/**
 * @brief Calculate sequential ignition timing
 *
 * Determines spark timing for each cylinder based on firing order
 *
 * @param ecu Pointer to ECU state
 * @param cylinder Cylinder number (0-based)
 * @return Spark timing in degrees BTDC
 */
float calculate_spark_timing(ecu_state_t* ecu, uint8_t cylinder);

#endif // ENGINE_CONTROL_H
