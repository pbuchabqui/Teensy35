/**
 * @file sensor_integration.h
 * @brief Sensor Integration Layer
 *
 * Integrates ADC reading and calibration with engine controller.
 * This module:
 * 1. Reads sensors via ADC
 * 2. Applies calibrations
 * 3. Updates engine controller sensor struct
 * 4. Validates readings
 *
 * @version 2.5.0
 * @date 2026-02-12
 */

#ifndef SENSOR_INTEGRATION_H
#define SENSOR_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>
#include "sensor_adc.h"
#include "sensor_calibration.h"
#include "../controllers/engine_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sensor integration system
 */
typedef struct {
    sensor_adc_t* adc;
    sensor_calibration_t* calibration;
    engine_controller_t* controller;
    bool initialized;
} sensor_integration_t;

/**
 * @brief Initialize sensor integration
 *
 * @param integration Pointer to integration structure
 * @param adc Pointer to ADC system
 * @param calibration Pointer to calibration system
 * @param controller Pointer to engine controller
 */
void sensor_integration_init(sensor_integration_t* integration,
                             sensor_adc_t* adc,
                             sensor_calibration_t* calibration,
                             engine_controller_t* controller);

/**
 * @brief Update all sensors
 *
 * 1. Update ADC readings
 * 2. Apply calibrations
 * 3. Update engine controller
 *
 * Call this periodically (e.g., every 1ms).
 *
 * @param integration Pointer to integration structure
 * @param current_time_us Current timestamp (microseconds)
 */
void sensor_integration_update(sensor_integration_t* integration,
                               uint32_t current_time_us);

/**
 * @brief Update MAP sensor
 *
 * @param integration Pointer to integration structure
 * @return true if successful
 */
bool sensor_integration_update_map(sensor_integration_t* integration);

/**
 * @brief Update TPS sensor
 *
 * @param integration Pointer to integration structure
 * @return true if successful
 */
bool sensor_integration_update_tps(sensor_integration_t* integration);

/**
 * @brief Update CLT sensor
 *
 * @param integration Pointer to integration structure
 * @return true if successful
 */
bool sensor_integration_update_clt(sensor_integration_t* integration);

/**
 * @brief Update IAT sensor
 *
 * @param integration Pointer to integration structure
 * @return true if successful
 */
bool sensor_integration_update_iat(sensor_integration_t* integration);

/**
 * @brief Update O2 sensor
 *
 * @param integration Pointer to integration structure
 * @return true if successful
 */
bool sensor_integration_update_o2(sensor_integration_t* integration);

/**
 * @brief Update battery voltage sensor
 *
 * @param integration Pointer to integration structure
 * @return true if successful
 */
bool sensor_integration_update_battery(sensor_integration_t* integration);

/**
 * @brief Calculate air density
 *
 * Uses ideal gas law: ρ = P / (R * T)
 * where:
 * - P = pressure (Pa)
 * - R = specific gas constant for air (287.05 J/(kg·K))
 * - T = temperature (K)
 *
 * @param map_kpa Manifold pressure (kPa)
 * @param iat_celsius Intake air temperature (°C)
 * @return Air density (kg/m³)
 */
float sensor_integration_calculate_air_density(uint16_t map_kpa,
                                                int16_t iat_celsius);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_INTEGRATION_H
