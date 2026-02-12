/**
 * @file aux_outputs.h
 * @brief Auxiliary Output Control System for Teensy 3.5
 *
 * Controls auxiliary outputs:
 * - Fuel pump (on/off or PWM)
 * - IAC valve (Idle Air Control - PWM)
 * - VVT solenoid (Variable Valve Timing - PWM)
 * - Check engine light / MIL
 * - Cooling fan (on/off or PWM)
 * - Tachometer output
 * - Other generic outputs
 *
 * Based on rusEFI auxiliary outputs.
 *
 * @version 2.5.0
 * @date 2026-02-12
 */

#ifndef AUX_OUTPUTS_H
#define AUX_OUTPUTS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Auxiliary output types
 */
typedef enum {
    AUX_OUTPUT_FUEL_PUMP = 0,       ///< Fuel pump
    AUX_OUTPUT_IAC,                 ///< Idle Air Control valve
    AUX_OUTPUT_VVT,                 ///< VVT solenoid
    AUX_OUTPUT_CHECK_ENGINE,        ///< Check engine light (MIL)
    AUX_OUTPUT_FAN,                 ///< Cooling fan
    AUX_OUTPUT_TACH,                ///< Tachometer output
    AUX_OUTPUT_BOOST_SOLENOID,      ///< Boost control solenoid
    AUX_OUTPUT_IDLE_UP,             ///< Idle-up solenoid (AC, etc.)
    AUX_OUTPUT_COUNT                ///< Total number of outputs
} aux_output_type_t;

/**
 * @brief Output mode
 */
typedef enum {
    AUX_MODE_ON_OFF = 0,            ///< Simple on/off
    AUX_MODE_PWM,                   ///< PWM control (0-100%)
} aux_output_mode_t;

/**
 * @brief Auxiliary output configuration
 */
typedef struct {
    uint8_t pin;                    ///< Output pin
    aux_output_mode_t mode;         ///< Output mode
    bool enabled;                   ///< Output enabled
    bool invert_polarity;           ///< Invert output
    uint16_t pwm_frequency_hz;      ///< PWM frequency (if PWM mode)
} aux_output_config_t;

/**
 * @brief Auxiliary output state
 */
typedef struct {
    bool active;                    ///< Output active
    float duty_cycle;               ///< PWM duty cycle (0-100%)
    uint32_t on_time_us;            ///< Total time active (µs)
    uint32_t last_change_time;      ///< Last state change timestamp
} aux_output_state_t;

/**
 * @brief Auxiliary outputs system
 */
typedef struct {
    aux_output_config_t config[AUX_OUTPUT_COUNT];
    aux_output_state_t state[AUX_OUTPUT_COUNT];
    bool initialized;
} aux_outputs_t;

/**
 * @brief Initialize auxiliary outputs system
 *
 * @param aux Pointer to auxiliary outputs structure
 */
void aux_outputs_init(aux_outputs_t* aux);

/**
 * @brief Configure an auxiliary output
 *
 * @param aux Pointer to auxiliary outputs structure
 * @param output Output type
 * @param pin Output pin
 * @param mode Output mode (on/off or PWM)
 * @param pwm_freq_hz PWM frequency (if PWM mode)
 * @param invert_polarity true to invert output
 */
void aux_output_configure(aux_outputs_t* aux,
                          aux_output_type_t output,
                          uint8_t pin,
                          aux_output_mode_t mode,
                          uint16_t pwm_freq_hz,
                          bool invert_polarity);

/**
 * @brief Enable/disable an output
 *
 * @param aux Pointer to auxiliary outputs structure
 * @param output Output type
 * @param enable true to enable
 */
void aux_output_enable(aux_outputs_t* aux,
                       aux_output_type_t output,
                       bool enable);

/**
 * @brief Set output state (on/off)
 *
 * For on/off mode outputs.
 *
 * @param aux Pointer to auxiliary outputs structure
 * @param output Output type
 * @param on true to turn on
 * @param timestamp Current timestamp (microseconds)
 */
void aux_output_set(aux_outputs_t* aux,
                    aux_output_type_t output,
                    bool on,
                    uint32_t timestamp);

/**
 * @brief Set PWM duty cycle
 *
 * For PWM mode outputs.
 *
 * @param aux Pointer to auxiliary outputs structure
 * @param output Output type
 * @param duty_percent Duty cycle (0-100%)
 */
void aux_output_set_pwm(aux_outputs_t* aux,
                        aux_output_type_t output,
                        float duty_percent);

/**
 * @brief Fuel pump control
 */
void aux_fuel_pump_on(aux_outputs_t* aux, uint32_t timestamp);
void aux_fuel_pump_off(aux_outputs_t* aux, uint32_t timestamp);

/**
 * @brief IAC valve control (PWM)
 */
void aux_iac_set_duty(aux_outputs_t* aux, float duty_percent);

/**
 * @brief VVT solenoid control (PWM)
 */
void aux_vvt_set_duty(aux_outputs_t* aux, float duty_percent);

/**
 * @brief Check engine light control
 */
void aux_check_engine_light(aux_outputs_t* aux, bool on, uint32_t timestamp);

/**
 * @brief Cooling fan control
 */
void aux_fan_on(aux_outputs_t* aux, uint32_t timestamp);
void aux_fan_off(aux_outputs_t* aux, uint32_t timestamp);
void aux_fan_set_speed(aux_outputs_t* aux, float speed_percent);

/**
 * @brief Tachometer output (generates pulses for tach)
 *
 * Call this on each ignition event to generate tach signal.
 *
 * @param aux Pointer to auxiliary outputs structure
 */
void aux_tach_pulse(aux_outputs_t* aux);

/**
 * @brief Get output state
 *
 * @param aux Pointer to auxiliary outputs structure
 * @param output Output type
 * @return Pointer to state structure (NULL if invalid)
 */
const aux_output_state_t* aux_output_get_state(const aux_outputs_t* aux,
                                                aux_output_type_t output);

/**
 * @brief Check if output is active
 *
 * @param aux Pointer to auxiliary outputs structure
 * @param output Output type
 * @return true if active
 */
bool aux_output_is_active(const aux_outputs_t* aux,
                          aux_output_type_t output);

/**
 * @brief Set default pin configuration
 *
 * Default pin assignments:
 * - Fuel pump: Pin 23
 * - IAC: Pin 20 (PWM capable)
 * - VVT: Pin 21 (PWM capable)
 * - Check engine light: Pin 13 (onboard LED)
 * - Fan: Pin 22
 * - Tach: Pin 25
 *
 * @param aux Pointer to auxiliary outputs structure
 */
void aux_outputs_set_default_pins(aux_outputs_t* aux);

/**
 * @brief Emergency shutdown (turn off all outputs)
 *
 * @param aux Pointer to auxiliary outputs structure
 */
void aux_outputs_emergency_shutdown(aux_outputs_t* aux);

/**
 * @brief Get output name string
 *
 * @param output Output type
 * @return Output name
 */
const char* aux_output_get_name(aux_output_type_t output);

#ifdef __cplusplus
}
#endif

#endif // AUX_OUTPUTS_H
