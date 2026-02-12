/**
 * @file engine_controller.h
 * @brief Main Engine Controller for Teensy 3.5 ECU
 *
 * This is the main control loop that ties together all ECU subsystems:
 * - Sensor reading
 * - Fuel calculation
 * - Timing calculation
 * - Actuator control
 * - Safety monitoring
 *
 * Based on rusEFI MainController and EngineState.
 *
 * @version 2.5.0
 * @date 2026-02-12
 */

#ifndef ENGINE_CONTROLLER_H
#define ENGINE_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Engine operation states
 */
typedef enum {
    ENGINE_STATE_STOPPED = 0,       ///< Engine not running
    ENGINE_STATE_CRANKING,          ///< Cranking (starting)
    ENGINE_STATE_RUNNING,           ///< Normal operation
    ENGINE_STATE_WARMUP,            ///< Warming up
    ENGINE_STATE_IDLE,              ///< Idling
    ENGINE_STATE_DECEL_FUEL_CUT,    ///< Deceleration fuel cut
    ENGINE_STATE_LIMP_MODE,         ///< Limp mode (failure)
} engine_state_t;

/**
 * @brief Engine load calculation methods
 */
typedef enum {
    LOAD_METHOD_SPEED_DENSITY = 0,  ///< Use MAP sensor
    LOAD_METHOD_ALPHA_N,            ///< Use TPS only (no MAP)
    LOAD_METHOD_MAF,                ///< Use MAF sensor
} load_method_t;

/**
 * @brief Sensor readings structure
 *
 * All sensor values in physical units.
 */
typedef struct {
    // Analog sensors
    uint16_t map_kpa;               ///< Manifold pressure (kPa)
    uint16_t tps_percent;           ///< Throttle position (0-100%)
    int16_t clt_celsius;            ///< Coolant temperature (°C)
    int16_t iat_celsius;            ///< Intake air temperature (°C)
    float lambda;                   ///< Lambda (AFR/Stoich), 1.0 = stoich
    float battery_volts;            ///< Battery voltage (V)

    // Optional sensors
    uint16_t maf_grams_sec;         ///< MAF reading (g/s) - optional
    uint16_t oil_pressure_kpa;      ///< Oil pressure (kPa) - optional

    // Computed values
    float air_density;              ///< Air density (kg/m³)

    // Validity flags
    bool map_valid;
    bool tps_valid;
    bool clt_valid;
    bool iat_valid;
    bool lambda_valid;
    bool battery_valid;

} sensor_readings_t;

/**
 * @brief Calculated engine parameters
 */
typedef struct {
    // Load calculation
    float engine_load_percent;      ///< Engine load (0-100%)
    load_method_t load_method;      ///< Load calculation method

    // Fuel
    float air_mass_grams;           ///< Air mass per cycle (g)
    float fuel_mass_grams;          ///< Fuel mass needed (g)
    uint32_t injection_duration_us; ///< Injection pulse width (µs)
    float target_afr;               ///< Target AFR
    float target_lambda;            ///< Target lambda

    // Timing
    int16_t spark_advance_deg;      ///< Spark advance (° BTDC)
    uint16_t spark_angle;           ///< Absolute spark angle (0-720°)
    uint32_t dwell_time_us;         ///< Coil dwell time (µs)
    uint16_t dwell_angle;           ///< Dwell start angle (0-720°)

    // Corrections applied
    float clt_fuel_correction;      ///< Coolant temp fuel multiplier
    float iat_fuel_correction;      ///< Air temp fuel multiplier
    float accel_fuel_correction;    ///< Acceleration enrichment
    float battery_correction;       ///< Battery voltage correction

    int16_t clt_timing_correction;  ///< Coolant temp timing offset (°)
    int16_t accel_timing_correction;///< Acceleration timing offset (°)

} engine_calculations_t;

/**
 * @brief Engine configuration
 */
typedef struct {
    // Basic engine parameters
    uint16_t displacement_cc;       ///< Displacement (cc)
    uint8_t cylinder_count;         ///< Number of cylinders
    uint8_t firing_order[8];        ///< Firing order

    // Operating parameters
    uint16_t cranking_rpm;          ///< Cranking RPM threshold
    uint16_t idle_rpm_target;       ///< Target idle RPM
    uint16_t rev_limit_rpm;         ///< Rev limiter RPM

    // Load calculation
    load_method_t load_method;      ///< Load calculation method

    // Fuel system
    float injector_flow_cc_min;     ///< Injector flow rate (cc/min @ 3 bar)
    uint16_t injector_dead_time_us; ///< Injector dead time (µs)
    float fuel_stoich_afr;          ///< Stoichiometric AFR (14.7 for gasoline)

    // Ignition system
    uint32_t coil_dwell_us;         ///< Base coil dwell time (µs)

} engine_config_t;

/**
 * @brief Main engine controller state
 */
typedef struct {
    // Configuration
    engine_config_t config;

    // Current state
    engine_state_t state;
    uint32_t state_entry_time;      ///< Time when entered current state

    // Current readings
    sensor_readings_t sensors;

    // Calculated values
    engine_calculations_t calc;

    // RPM (from rpm_calculator)
    uint16_t rpm;

    // Cycle tracking
    uint16_t current_cycle_angle;   ///< 0-720° position
    uint8_t next_cylinder_to_fire;  ///< Next cylinder for injection/spark

    // Timing
    uint32_t last_update_time_us;   ///< Last update timestamp
    uint32_t update_period_us;      ///< Time between updates

    // Flags
    bool initialized;
    bool closed_loop_active;        ///< Closed-loop O2 control active
    bool idle_control_active;       ///< Idle control active

} engine_controller_t;

/**
 * @brief Initialize engine controller
 *
 * @param controller Pointer to controller structure
 * @param config Pointer to engine configuration
 */
void engine_controller_init(engine_controller_t* controller,
                            const engine_config_t* config);

/**
 * @brief Main engine control update
 *
 * This is the main control loop. Call this periodically (e.g., every tooth event
 * or at fixed intervals like 1ms).
 *
 * Steps:
 * 1. Update engine state
 * 2. Read sensors
 * 3. Calculate load
 * 4. Calculate fuel requirements
 * 5. Calculate timing
 * 6. Schedule injection events
 * 7. Schedule ignition events
 * 8. Update auxiliary outputs
 *
 * @param controller Pointer to controller structure
 * @param rpm Current RPM (from rpm_calculator)
 * @param cycle_angle Current cycle angle 0-720° (from cam_sync)
 * @param timestamp_us Current timestamp (microseconds)
 */
void engine_controller_update(engine_controller_t* controller,
                              uint16_t rpm,
                              uint16_t cycle_angle,
                              uint32_t timestamp_us);

/**
 * @brief Update engine state machine
 *
 * Determines current engine state based on RPM and other conditions.
 *
 * @param controller Pointer to controller structure
 * @param rpm Current RPM
 */
void engine_controller_update_state(engine_controller_t* controller,
                                    uint16_t rpm);

/**
 * @brief Calculate engine load
 *
 * Uses configured method (speed-density, alpha-N, or MAF).
 *
 * @param controller Pointer to controller structure
 * @return Engine load (0-100%)
 */
float engine_controller_calculate_load(engine_controller_t* controller);

/**
 * @brief Get current engine state
 *
 * @param controller Pointer to controller structure
 * @return Current engine state
 */
engine_state_t engine_controller_get_state(const engine_controller_t* controller);

/**
 * @brief Check if engine is running
 *
 * @param controller Pointer to controller structure
 * @return true if engine is running
 */
bool engine_controller_is_running(const engine_controller_t* controller);

/**
 * @brief Check if engine is cranking
 *
 * @param controller Pointer to controller structure
 * @return true if engine is cranking
 */
bool engine_controller_is_cranking(const engine_controller_t* controller);

/**
 * @brief Enable/disable closed-loop control
 *
 * @param controller Pointer to controller structure
 * @param enable true to enable closed-loop
 */
void engine_controller_set_closed_loop(engine_controller_t* controller,
                                       bool enable);

/**
 * @brief Get sensor readings
 *
 * @param controller Pointer to controller structure
 * @return Pointer to sensor readings structure
 */
const sensor_readings_t* engine_controller_get_sensors(
    const engine_controller_t* controller);

/**
 * @brief Get calculated values
 *
 * @param controller Pointer to controller structure
 * @return Pointer to calculations structure
 */
const engine_calculations_t* engine_controller_get_calculations(
    const engine_controller_t* controller);

/**
 * @brief Emergency shutdown
 *
 * Stops all injection and ignition immediately.
 *
 * @param controller Pointer to controller structure
 */
void engine_controller_emergency_shutdown(engine_controller_t* controller);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_CONTROLLER_H
