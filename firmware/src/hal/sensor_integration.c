/**
 * @file sensor_integration.c
 * @brief Sensor Integration Layer Implementation
 *
 * @version 2.5.0
 * @date 2026-02-12
 */

#include "sensor_integration.h"
#include <math.h>

// Specific gas constant for air (J/(kg·K))
#define AIR_GAS_CONSTANT  287.05f

/**
 * @brief Initialize sensor integration
 */
void sensor_integration_init(sensor_integration_t* integration,
                             sensor_adc_t* adc,
                             sensor_calibration_t* calibration,
                             engine_controller_t* controller)
{
    if (integration == NULL || adc == NULL ||
        calibration == NULL || controller == NULL) {
        return;
    }

    integration->adc = adc;
    integration->calibration = calibration;
    integration->controller = controller;
    integration->initialized = true;
}

/**
 * @brief Update all sensors
 */
void sensor_integration_update(sensor_integration_t* integration,
                               uint32_t current_time_us)
{
    if (integration == NULL || !integration->initialized) {
        return;
    }

    // Update ADC readings (respects sample intervals)
    sensor_adc_update_all(integration->adc, current_time_us);

    // Update each sensor in the engine controller
    sensor_integration_update_map(integration);
    sensor_integration_update_tps(integration);
    sensor_integration_update_clt(integration);
    sensor_integration_update_iat(integration);
    sensor_integration_update_o2(integration);
    sensor_integration_update_battery(integration);

    // Calculate derived values
    if (integration->controller->sensors.map_valid &&
        integration->controller->sensors.iat_valid) {

        integration->controller->sensors.air_density =
            sensor_integration_calculate_air_density(
                integration->controller->sensors.map_kpa,
                integration->controller->sensors.iat_celsius
            );
    }
}

/**
 * @brief Update MAP sensor
 */
bool sensor_integration_update_map(sensor_integration_t* integration)
{
    if (integration == NULL) {
        return false;
    }

    // Get voltage from ADC
    float voltage = sensor_adc_get_voltage(integration->adc, SENSOR_MAP);
    if (voltage < 0.0f) {
        integration->controller->sensors.map_valid = false;
        return false;
    }

    // Apply calibration
    float map_kpa;
    bool valid = sensor_calibration_convert(
        integration->calibration,
        SENSOR_MAP,
        voltage,
        &map_kpa
    );

    if (valid) {
        integration->controller->sensors.map_kpa = (uint16_t)map_kpa;
        integration->controller->sensors.map_valid = true;
        return true;
    } else {
        integration->controller->sensors.map_valid = false;
        return false;
    }
}

/**
 * @brief Update TPS sensor
 */
bool sensor_integration_update_tps(sensor_integration_t* integration)
{
    if (integration == NULL) {
        return false;
    }

    float voltage = sensor_adc_get_voltage(integration->adc, SENSOR_TPS);
    if (voltage < 0.0f) {
        integration->controller->sensors.tps_valid = false;
        return false;
    }

    float tps_percent;
    bool valid = sensor_calibration_convert(
        integration->calibration,
        SENSOR_TPS,
        voltage,
        &tps_percent
    );

    if (valid) {
        integration->controller->sensors.tps_percent = (uint16_t)tps_percent;
        integration->controller->sensors.tps_valid = true;
        return true;
    } else {
        integration->controller->sensors.tps_valid = false;
        return false;
    }
}

/**
 * @brief Update CLT sensor
 */
bool sensor_integration_update_clt(sensor_integration_t* integration)
{
    if (integration == NULL) {
        return false;
    }

    float voltage = sensor_adc_get_voltage(integration->adc, SENSOR_CLT);
    if (voltage < 0.0f) {
        integration->controller->sensors.clt_valid = false;
        return false;
    }

    float clt_celsius;
    bool valid = sensor_calibration_convert(
        integration->calibration,
        SENSOR_CLT,
        voltage,
        &clt_celsius
    );

    if (valid) {
        integration->controller->sensors.clt_celsius = (int16_t)clt_celsius;
        integration->controller->sensors.clt_valid = true;
        return true;
    } else {
        integration->controller->sensors.clt_valid = false;
        return false;
    }
}

/**
 * @brief Update IAT sensor
 */
bool sensor_integration_update_iat(sensor_integration_t* integration)
{
    if (integration == NULL) {
        return false;
    }

    float voltage = sensor_adc_get_voltage(integration->adc, SENSOR_IAT);
    if (voltage < 0.0f) {
        integration->controller->sensors.iat_valid = false;
        return false;
    }

    float iat_celsius;
    bool valid = sensor_calibration_convert(
        integration->calibration,
        SENSOR_IAT,
        voltage,
        &iat_celsius
    );

    if (valid) {
        integration->controller->sensors.iat_celsius = (int16_t)iat_celsius;
        integration->controller->sensors.iat_valid = true;
        return true;
    } else {
        integration->controller->sensors.iat_valid = false;
        return false;
    }
}

/**
 * @brief Update O2 sensor
 */
bool sensor_integration_update_o2(sensor_integration_t* integration)
{
    if (integration == NULL) {
        return false;
    }

    float voltage = sensor_adc_get_voltage(integration->adc, SENSOR_O2);
    if (voltage < 0.0f) {
        integration->controller->sensors.lambda_valid = false;
        return false;
    }

    float lambda;
    bool valid = sensor_calibration_convert(
        integration->calibration,
        SENSOR_O2,
        voltage,
        &lambda
    );

    if (valid) {
        integration->controller->sensors.lambda = lambda;
        integration->controller->sensors.lambda_valid = true;
        return true;
    } else {
        integration->controller->sensors.lambda_valid = false;
        return false;
    }
}

/**
 * @brief Update battery voltage sensor
 */
bool sensor_integration_update_battery(sensor_integration_t* integration)
{
    if (integration == NULL) {
        return false;
    }

    float voltage = sensor_adc_get_voltage(integration->adc, SENSOR_BATTERY);
    if (voltage < 0.0f) {
        integration->controller->sensors.battery_valid = false;
        return false;
    }

    float battery_volts;
    bool valid = sensor_calibration_convert(
        integration->calibration,
        SENSOR_BATTERY,
        voltage,
        &battery_volts
    );

    if (valid) {
        integration->controller->sensors.battery_volts = battery_volts;
        integration->controller->sensors.battery_valid = true;
        return true;
    } else {
        integration->controller->sensors.battery_valid = false;
        return false;
    }
}

/**
 * @brief Calculate air density
 *
 * Uses ideal gas law: ρ = P / (R * T)
 */
float sensor_integration_calculate_air_density(uint16_t map_kpa,
                                                int16_t iat_celsius)
{
    // Convert to SI units
    float pressure_pa = (float)map_kpa * 1000.0f;  // kPa → Pa
    float temp_kelvin = (float)iat_celsius + 273.15f;  // °C → K

    // Ideal gas law: ρ = P / (R * T)
    float density = pressure_pa / (AIR_GAS_CONSTANT * temp_kelvin);

    return density;  // kg/m³
}
