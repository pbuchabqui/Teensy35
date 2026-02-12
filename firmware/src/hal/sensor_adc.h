/**
 * @file sensor_adc.h
 * @brief ADC Sensor Input System for Teensy 3.5
 *
 * Handles all analog sensor inputs using Teensy 3.5 ADC.
 * Supports multiple sensors with configurable pins and calibrations.
 *
 * Teensy 3.5 ADC specifications:
 * - 2x 16-bit ADCs
 * - 3.3V reference
 * - Resolution: 0-4095 (12-bit default) or 0-65535 (16-bit)
 * - Averaging supported
 *
 * @version 2.5.0
 * @date 2026-02-12
 */

#ifndef SENSOR_ADC_H
#define SENSOR_ADC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC sensor types
 */
typedef enum {
    SENSOR_MAP = 0,                 ///< Manifold Absolute Pressure
    SENSOR_TPS,                     ///< Throttle Position Sensor
    SENSOR_CLT,                     ///< Coolant Temperature
    SENSOR_IAT,                     ///< Intake Air Temperature
    SENSOR_O2,                      ///< Oxygen/Lambda sensor
    SENSOR_BATTERY,                 ///< Battery voltage
    SENSOR_MAF,                     ///< Mass Air Flow (optional)
    SENSOR_OIL_PRESSURE,            ///< Oil pressure (optional)
    SENSOR_FUEL_PRESSURE,           ///< Fuel pressure (optional)
    SENSOR_COUNT                    ///< Total number of sensors
} sensor_type_t;

/**
 * @brief ADC configuration per sensor
 */
typedef struct {
    uint8_t adc_pin;                ///< ADC pin number (A0-A21 on Teensy 3.5)
    uint8_t resolution_bits;        ///< ADC resolution (8, 10, 12, or 16 bits)
    uint8_t averaging;              ///< Number of samples to average (0-32)
    bool enabled;                   ///< Sensor enabled
    uint32_t sample_interval_us;    ///< Sampling interval (microseconds)
    uint32_t last_sample_time;      ///< Last sample timestamp
} adc_config_t;

/**
 * @brief Raw ADC reading
 */
typedef struct {
    uint16_t raw_value;             ///< Raw ADC value (0-4095 or 0-65535)
    float voltage;                  ///< Converted voltage (0-3.3V)
    uint32_t timestamp_us;          ///< Sample timestamp
    bool valid;                     ///< Reading valid
} adc_reading_t;

/**
 * @brief ADC sensor system state
 */
typedef struct {
    adc_config_t config[SENSOR_COUNT];
    adc_reading_t readings[SENSOR_COUNT];
    bool initialized;
    uint32_t total_samples;
    uint32_t error_count;
} sensor_adc_t;

/**
 * @brief Initialize ADC sensor system
 *
 * @param adc Pointer to ADC system structure
 */
void sensor_adc_init(sensor_adc_t* adc);

/**
 * @brief Configure a sensor
 *
 * @param adc Pointer to ADC system structure
 * @param sensor Sensor type
 * @param pin ADC pin number
 * @param resolution ADC resolution (8, 10, 12, or 16 bits)
 * @param averaging Number of samples to average
 * @param sample_interval_us Minimum time between samples (µs)
 */
void sensor_adc_configure(sensor_adc_t* adc,
                          sensor_type_t sensor,
                          uint8_t pin,
                          uint8_t resolution,
                          uint8_t averaging,
                          uint32_t sample_interval_us);

/**
 * @brief Enable/disable a sensor
 *
 * @param adc Pointer to ADC system structure
 * @param sensor Sensor type
 * @param enable true to enable, false to disable
 */
void sensor_adc_enable(sensor_adc_t* adc,
                       sensor_type_t sensor,
                       bool enable);

/**
 * @brief Update all enabled sensors
 *
 * Reads all sensors that are due for sampling based on their intervals.
 * Call this periodically (e.g., every 1ms).
 *
 * @param adc Pointer to ADC system structure
 * @param current_time_us Current timestamp (microseconds)
 */
void sensor_adc_update_all(sensor_adc_t* adc,
                           uint32_t current_time_us);

/**
 * @brief Read a specific sensor immediately
 *
 * Forces an immediate read regardless of sample interval.
 *
 * @param adc Pointer to ADC system structure
 * @param sensor Sensor type
 * @param current_time_us Current timestamp (microseconds)
 * @return true if read successful
 */
bool sensor_adc_read_sensor(sensor_adc_t* adc,
                            sensor_type_t sensor,
                            uint32_t current_time_us);

/**
 * @brief Get raw ADC reading
 *
 * @param adc Pointer to ADC system structure
 * @param sensor Sensor type
 * @return Pointer to reading structure (NULL if invalid)
 */
const adc_reading_t* sensor_adc_get_reading(const sensor_adc_t* adc,
                                            sensor_type_t sensor);

/**
 * @brief Get sensor voltage
 *
 * @param adc Pointer to ADC system structure
 * @param sensor Sensor type
 * @return Voltage (0-3.3V), or -1.0f if invalid
 */
float sensor_adc_get_voltage(const sensor_adc_t* adc,
                             sensor_type_t sensor);

/**
 * @brief Get sensor raw value
 *
 * @param adc Pointer to ADC system structure
 * @param sensor Sensor type
 * @return Raw ADC value, or 0 if invalid
 */
uint16_t sensor_adc_get_raw(const sensor_adc_t* adc,
                            sensor_type_t sensor);

/**
 * @brief Check if sensor reading is valid
 *
 * @param adc Pointer to ADC system structure
 * @param sensor Sensor type
 * @return true if valid
 */
bool sensor_adc_is_valid(const sensor_adc_t* adc,
                         sensor_type_t sensor);

/**
 * @brief Set default pin configuration for common setups
 *
 * Default pin assignments for Teensy 3.5:
 * - MAP:  A0 (14)
 * - TPS:  A1 (15)
 * - CLT:  A2 (16)
 * - IAT:  A3 (17)
 * - O2:   A4 (18)
 * - Vbat: A5 (19)
 * - MAF:  A6 (20) - optional
 * - Oil:  A7 (21) - optional
 *
 * @param adc Pointer to ADC system structure
 */
void sensor_adc_set_default_config(sensor_adc_t* adc);

/**
 * @brief Get sensor name string
 *
 * @param sensor Sensor type
 * @return Sensor name string
 */
const char* sensor_adc_get_name(sensor_type_t sensor);

/**
 * @brief Get ADC statistics
 *
 * @param adc Pointer to ADC system structure
 * @param total_samples Output: total samples taken
 * @param error_count Output: number of errors
 */
void sensor_adc_get_stats(const sensor_adc_t* adc,
                          uint32_t* total_samples,
                          uint32_t* error_count);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_ADC_H
