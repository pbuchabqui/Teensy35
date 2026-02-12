/**
 * @file sensor_calibration.h
 * @brief Sensor Calibration System
 *
 * Converts raw sensor voltages to physical units:
 * - MAP sensor: voltage → kPa
 * - TPS sensor: voltage → percent
 * - Temperature sensors: voltage → °C (thermistor curves)
 * - O2 sensor: voltage → lambda
 * - Battery: voltage → volts
 *
 * Supports multiple calibration methods:
 * - Linear interpolation
 * - Steinhart-Hart equation (thermistors)
 * - Custom transfer functions
 *
 * @version 2.5.0
 * @date 2026-02-12
 */

#ifndef SENSOR_CALIBRATION_H
#define SENSOR_CALIBRATION_H

#include <stdint.h>
#include <stdbool.h>
#include "sensor_adc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calibration method types
 */
typedef enum {
    CALIBRATION_LINEAR = 0,         ///< Linear interpolation
    CALIBRATION_THERMISTOR,         ///< Steinhart-Hart (NTC thermistor)
    CALIBRATION_TABLE,              ///< Lookup table with interpolation
    CALIBRATION_CUSTOM,             ///< Custom function
} calibration_method_t;

/**
 * @brief Linear calibration parameters
 */
typedef struct {
    float voltage_min;              ///< Minimum voltage
    float voltage_max;              ///< Maximum voltage
    float value_min;                ///< Value at voltage_min
    float value_max;                ///< Value at voltage_max
} linear_calibration_t;

/**
 * @brief Thermistor calibration parameters (Steinhart-Hart)
 */
typedef struct {
    float bias_resistor;            ///< Pull-up/pull-down resistor (Ω)
    float vref;                     ///< Reference voltage (V)
    bool pull_up;                   ///< true if pull-up, false if pull-down

    // Steinhart-Hart coefficients
    float A;                        ///< Coefficient A
    float B;                        ///< Coefficient B
    float C;                        ///< Coefficient C
} thermistor_calibration_t;

/**
 * @brief Table calibration (voltage-value pairs)
 */
typedef struct {
    uint8_t num_points;             ///< Number of calibration points
    float voltages[32];             ///< Voltage points
    float values[32];               ///< Corresponding values
} table_calibration_t;

/**
 * @brief Custom calibration function pointer
 */
typedef float (*custom_calibration_fn)(float voltage, void* context);

/**
 * @brief Sensor calibration configuration
 */
typedef struct {
    calibration_method_t method;
    bool enabled;

    union {
        linear_calibration_t linear;
        thermistor_calibration_t thermistor;
        table_calibration_t table;
        struct {
            custom_calibration_fn func;
            void* context;
        } custom;
    } params;

    // Range validation
    float min_valid_value;          ///< Minimum valid output
    float max_valid_value;          ///< Maximum valid output

} sensor_calibration_config_t;

/**
 * @brief Calibration system state
 */
typedef struct {
    sensor_calibration_config_t calibrations[SENSOR_COUNT];
    bool initialized;
} sensor_calibration_t;

/**
 * @brief Initialize calibration system
 *
 * @param cal Pointer to calibration system
 */
void sensor_calibration_init(sensor_calibration_t* cal);

/**
 * @brief Set linear calibration
 *
 * @param cal Pointer to calibration system
 * @param sensor Sensor type
 * @param v_min Minimum voltage
 * @param v_max Maximum voltage
 * @param val_min Value at v_min
 * @param val_max Value at v_max
 */
void sensor_calibration_set_linear(sensor_calibration_t* cal,
                                    sensor_type_t sensor,
                                    float v_min,
                                    float v_max,
                                    float val_min,
                                    float val_max);

/**
 * @brief Set thermistor calibration (Steinhart-Hart)
 *
 * @param cal Pointer to calibration system
 * @param sensor Sensor type
 * @param bias_resistor Pull-up/pull-down resistor value (Ω)
 * @param vref Reference voltage (V)
 * @param pull_up true if pull-up, false if pull-down
 * @param A Steinhart-Hart coefficient A
 * @param B Steinhart-Hart coefficient B
 * @param C Steinhart-Hart coefficient C
 */
void sensor_calibration_set_thermistor(sensor_calibration_t* cal,
                                        sensor_type_t sensor,
                                        float bias_resistor,
                                        float vref,
                                        bool pull_up,
                                        float A,
                                        float B,
                                        float C);

/**
 * @brief Set table calibration
 *
 * @param cal Pointer to calibration system
 * @param sensor Sensor type
 * @param num_points Number of calibration points
 * @param voltages Array of voltage points
 * @param values Array of corresponding values
 */
void sensor_calibration_set_table(sensor_calibration_t* cal,
                                   sensor_type_t sensor,
                                   uint8_t num_points,
                                   const float* voltages,
                                   const float* values);

/**
 * @brief Set custom calibration function
 *
 * @param cal Pointer to calibration system
 * @param sensor Sensor type
 * @param func Custom calibration function
 * @param context Context pointer passed to function
 */
void sensor_calibration_set_custom(sensor_calibration_t* cal,
                                    sensor_type_t sensor,
                                    custom_calibration_fn func,
                                    void* context);

/**
 * @brief Set validation range
 *
 * @param cal Pointer to calibration system
 * @param sensor Sensor type
 * @param min_value Minimum valid value
 * @param max_value Maximum valid value
 */
void sensor_calibration_set_range(sensor_calibration_t* cal,
                                   sensor_type_t sensor,
                                   float min_value,
                                   float max_value);

/**
 * @brief Convert voltage to physical value
 *
 * @param cal Pointer to calibration system
 * @param sensor Sensor type
 * @param voltage Input voltage
 * @param value Output: calibrated value
 * @return true if successful and valid
 */
bool sensor_calibration_convert(const sensor_calibration_t* cal,
                                sensor_type_t sensor,
                                float voltage,
                                float* value);

/**
 * @brief Load default calibrations for common sensors
 *
 * Default calibrations:
 * - MAP: MPXH6400A (20-400 kPa, 0.5-4.5V)
 * - TPS: Standard potentiometer (0-5V → 0-100%)
 * - CLT: GM NTC thermistor
 * - IAT: GM NTC thermistor
 * - O2: Bosch LSU 4.9 (narrowband 0-1V → 0.68-1.36 lambda)
 * - Battery: 4:1 voltage divider (0-16.5V → 0-3.3V)
 *
 * @param cal Pointer to calibration system
 */
void sensor_calibration_load_defaults(sensor_calibration_t* cal);

/**
 * @brief Helper: Calculate thermistor temperature (Steinhart-Hart)
 *
 * @param resistance Thermistor resistance (Ω)
 * @param A Coefficient A
 * @param B Coefficient B
 * @param C Coefficient C
 * @return Temperature in °C
 */
float sensor_calibration_thermistor_temp(float resistance,
                                          float A,
                                          float B,
                                          float C);

/**
 * @brief Helper: Calculate resistance from voltage divider
 *
 * @param voltage Measured voltage
 * @param vref Reference voltage
 * @param bias_resistor Known resistor value
 * @param pull_up true if pull-up, false if pull-down
 * @return Calculated resistance
 */
float sensor_calibration_resistance_from_voltage(float voltage,
                                                  float vref,
                                                  float bias_resistor,
                                                  bool pull_up);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_CALIBRATION_H
