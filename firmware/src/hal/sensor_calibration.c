/**
 * @file sensor_calibration.c
 * @brief Sensor Calibration System Implementation
 *
 * @version 2.5.0
 * @date 2026-02-12
 */

#include "sensor_calibration.h"
#include <string.h>
#include <math.h>

// GM NTC thermistor Steinhart-Hart coefficients (typical)
#define GM_THERMISTOR_A  0.001129148f
#define GM_THERMISTOR_B  0.000234125f
#define GM_THERMISTOR_C  0.0000000876741f

// GM thermistor bias resistor (typical 2.49kΩ)
#define GM_THERMISTOR_BIAS  2490.0f

/**
 * @brief Initialize calibration system
 */
void sensor_calibration_init(sensor_calibration_t* cal)
{
    if (cal == NULL) {
        return;
    }

    memset(cal, 0, sizeof(sensor_calibration_t));

    // Disable all calibrations by default
    for (int i = 0; i < SENSOR_COUNT; i++) {
        cal->calibrations[i].enabled = false;
        cal->calibrations[i].min_valid_value = -1000.0f;
        cal->calibrations[i].max_valid_value = 10000.0f;
    }

    cal->initialized = true;
}

/**
 * @brief Set linear calibration
 */
void sensor_calibration_set_linear(sensor_calibration_t* cal,
                                    sensor_type_t sensor,
                                    float v_min,
                                    float v_max,
                                    float val_min,
                                    float val_max)
{
    if (cal == NULL || sensor >= SENSOR_COUNT) {
        return;
    }

    cal->calibrations[sensor].method = CALIBRATION_LINEAR;
    cal->calibrations[sensor].params.linear.voltage_min = v_min;
    cal->calibrations[sensor].params.linear.voltage_max = v_max;
    cal->calibrations[sensor].params.linear.value_min = val_min;
    cal->calibrations[sensor].params.linear.value_max = val_max;
    cal->calibrations[sensor].enabled = true;
}

/**
 * @brief Set thermistor calibration
 */
void sensor_calibration_set_thermistor(sensor_calibration_t* cal,
                                        sensor_type_t sensor,
                                        float bias_resistor,
                                        float vref,
                                        bool pull_up,
                                        float A,
                                        float B,
                                        float C)
{
    if (cal == NULL || sensor >= SENSOR_COUNT) {
        return;
    }

    cal->calibrations[sensor].method = CALIBRATION_THERMISTOR;
    cal->calibrations[sensor].params.thermistor.bias_resistor = bias_resistor;
    cal->calibrations[sensor].params.thermistor.vref = vref;
    cal->calibrations[sensor].params.thermistor.pull_up = pull_up;
    cal->calibrations[sensor].params.thermistor.A = A;
    cal->calibrations[sensor].params.thermistor.B = B;
    cal->calibrations[sensor].params.thermistor.C = C;
    cal->calibrations[sensor].enabled = true;
}

/**
 * @brief Set table calibration
 */
void sensor_calibration_set_table(sensor_calibration_t* cal,
                                   sensor_type_t sensor,
                                   uint8_t num_points,
                                   const float* voltages,
                                   const float* values)
{
    if (cal == NULL || sensor >= SENSOR_COUNT || num_points > 32) {
        return;
    }

    cal->calibrations[sensor].method = CALIBRATION_TABLE;
    cal->calibrations[sensor].params.table.num_points = num_points;

    for (int i = 0; i < num_points; i++) {
        cal->calibrations[sensor].params.table.voltages[i] = voltages[i];
        cal->calibrations[sensor].params.table.values[i] = values[i];
    }

    cal->calibrations[sensor].enabled = true;
}

/**
 * @brief Set custom calibration function
 */
void sensor_calibration_set_custom(sensor_calibration_t* cal,
                                    sensor_type_t sensor,
                                    custom_calibration_fn func,
                                    void* context)
{
    if (cal == NULL || sensor >= SENSOR_COUNT || func == NULL) {
        return;
    }

    cal->calibrations[sensor].method = CALIBRATION_CUSTOM;
    cal->calibrations[sensor].params.custom.func = func;
    cal->calibrations[sensor].params.custom.context = context;
    cal->calibrations[sensor].enabled = true;
}

/**
 * @brief Set validation range
 */
void sensor_calibration_set_range(sensor_calibration_t* cal,
                                   sensor_type_t sensor,
                                   float min_value,
                                   float max_value)
{
    if (cal == NULL || sensor >= SENSOR_COUNT) {
        return;
    }

    cal->calibrations[sensor].min_valid_value = min_value;
    cal->calibrations[sensor].max_valid_value = max_value;
}

/**
 * @brief Convert voltage to physical value
 */
bool sensor_calibration_convert(const sensor_calibration_t* cal,
                                sensor_type_t sensor,
                                float voltage,
                                float* value)
{
    if (cal == NULL || sensor >= SENSOR_COUNT || value == NULL) {
        return false;
    }

    if (!cal->calibrations[sensor].enabled) {
        return false;
    }

    const sensor_calibration_config_t* cfg = &cal->calibrations[sensor];
    float result = 0.0f;

    // Apply calibration based on method
    switch (cfg->method) {
        case CALIBRATION_LINEAR: {
            const linear_calibration_t* lin = &cfg->params.linear;

            // Linear interpolation: y = y0 + (y1-y0) * (x-x0) / (x1-x0)
            float voltage_range = lin->voltage_max - lin->voltage_min;
            float value_range = lin->value_max - lin->value_min;

            if (voltage_range != 0.0f) {
                result = lin->value_min +
                        value_range * (voltage - lin->voltage_min) / voltage_range;
            }
            break;
        }

        case CALIBRATION_THERMISTOR: {
            const thermistor_calibration_t* therm = &cfg->params.thermistor;

            // Calculate resistance from voltage
            float resistance = sensor_calibration_resistance_from_voltage(
                voltage,
                therm->vref,
                therm->bias_resistor,
                therm->pull_up
            );

            // Apply Steinhart-Hart equation
            result = sensor_calibration_thermistor_temp(
                resistance,
                therm->A,
                therm->B,
                therm->C
            );
            break;
        }

        case CALIBRATION_TABLE: {
            const table_calibration_t* table = &cfg->params.table;

            // Find voltage range in table
            if (table->num_points < 2) {
                return false;
            }

            // Check bounds
            if (voltage <= table->voltages[0]) {
                result = table->values[0];
            } else if (voltage >= table->voltages[table->num_points - 1]) {
                result = table->values[table->num_points - 1];
            } else {
                // Linear interpolation between points
                for (int i = 0; i < table->num_points - 1; i++) {
                    if (voltage >= table->voltages[i] &&
                        voltage <= table->voltages[i + 1]) {

                        float v0 = table->voltages[i];
                        float v1 = table->voltages[i + 1];
                        float val0 = table->values[i];
                        float val1 = table->values[i + 1];

                        float t = (voltage - v0) / (v1 - v0);
                        result = val0 + t * (val1 - val0);
                        break;
                    }
                }
            }
            break;
        }

        case CALIBRATION_CUSTOM: {
            result = cfg->params.custom.func(voltage, cfg->params.custom.context);
            break;
        }

        default:
            return false;
    }

    // Validate range
    if (result < cfg->min_valid_value || result > cfg->max_valid_value) {
        return false;
    }

    *value = result;
    return true;
}

/**
 * @brief Load default calibrations
 */
void sensor_calibration_load_defaults(sensor_calibration_t* cal)
{
    if (cal == NULL) {
        return;
    }

    // MAP Sensor: MPXH6400A (20-400 kPa, 0.5-4.5V)
    sensor_calibration_set_linear(cal, SENSOR_MAP,
                                   0.5f, 4.5f,   // 0.5V-4.5V
                                   20.0f, 400.0f); // 20-400 kPa
    sensor_calibration_set_range(cal, SENSOR_MAP, 10.0f, 450.0f);

    // TPS: Standard potentiometer (0-3.3V → 0-100%)
    sensor_calibration_set_linear(cal, SENSOR_TPS,
                                   0.0f, 3.3f,   // 0-3.3V
                                   0.0f, 100.0f); // 0-100%
    sensor_calibration_set_range(cal, SENSOR_TPS, 0.0f, 100.0f);

    // CLT: GM NTC thermistor (pull-up)
    sensor_calibration_set_thermistor(cal, SENSOR_CLT,
                                       GM_THERMISTOR_BIAS,  // 2.49kΩ
                                       3.3f,                // 3.3V ref
                                       true,                // pull-up
                                       GM_THERMISTOR_A,
                                       GM_THERMISTOR_B,
                                       GM_THERMISTOR_C);
    sensor_calibration_set_range(cal, SENSOR_CLT, -40.0f, 150.0f);

    // IAT: GM NTC thermistor (pull-up)
    sensor_calibration_set_thermistor(cal, SENSOR_IAT,
                                       GM_THERMISTOR_BIAS,
                                       3.3f,
                                       true,
                                       GM_THERMISTOR_A,
                                       GM_THERMISTOR_B,
                                       GM_THERMISTOR_C);
    sensor_calibration_set_range(cal, SENSOR_IAT, -40.0f, 150.0f);

    // O2 Sensor: Narrowband (0-1V → 0.68-1.36 lambda)
    // 0.1V = lean (1.36), 0.9V = rich (0.68)
    sensor_calibration_set_linear(cal, SENSOR_O2,
                                   0.1f, 0.9f,     // 0.1-0.9V
                                   1.36f, 0.68f);  // 1.36-0.68 lambda (inverted!)
    sensor_calibration_set_range(cal, SENSOR_O2, 0.5f, 1.5f);

    // Battery: 4:1 voltage divider (0-16.5V → 0-3.3V)
    // Measured voltage * 5 = battery voltage
    sensor_calibration_set_linear(cal, SENSOR_BATTERY,
                                   0.0f, 3.3f,    // 0-3.3V measured
                                   0.0f, 16.5f);  // 0-16.5V actual
    sensor_calibration_set_range(cal, SENSOR_BATTERY, 8.0f, 18.0f);

    // MAF sensor (optional) - disabled by default
    // Will be configured when MAF is used

    // Oil pressure (optional) - disabled by default
    // Typical: 0.5-4.5V → 0-1000 kPa
    sensor_calibration_set_linear(cal, SENSOR_OIL_PRESSURE,
                                   0.5f, 4.5f,
                                   0.0f, 1000.0f);
    sensor_calibration_set_range(cal, SENSOR_OIL_PRESSURE, 0.0f, 1200.0f);

    // Fuel pressure (optional) - disabled by default
    sensor_calibration_set_linear(cal, SENSOR_FUEL_PRESSURE,
                                   0.5f, 4.5f,
                                   0.0f, 1000.0f);
    sensor_calibration_set_range(cal, SENSOR_FUEL_PRESSURE, 0.0f, 1200.0f);
}

/**
 * @brief Calculate thermistor temperature (Steinhart-Hart)
 */
float sensor_calibration_thermistor_temp(float resistance,
                                          float A,
                                          float B,
                                          float C)
{
    if (resistance <= 0.0f) {
        return -273.15f;  // Invalid
    }

    // Steinhart-Hart equation:
    // 1/T = A + B*ln(R) + C*(ln(R))^3
    // T in Kelvin

    float ln_r = logf(resistance);
    float ln_r_cubed = ln_r * ln_r * ln_r;

    float inv_t = A + B * ln_r + C * ln_r_cubed;

    if (inv_t <= 0.0f) {
        return -273.15f;  // Invalid
    }

    float temp_kelvin = 1.0f / inv_t;
    float temp_celsius = temp_kelvin - 273.15f;

    return temp_celsius;
}

/**
 * @brief Calculate resistance from voltage divider
 */
float sensor_calibration_resistance_from_voltage(float voltage,
                                                  float vref,
                                                  float bias_resistor,
                                                  bool pull_up)
{
    if (voltage <= 0.0f || voltage >= vref) {
        return 0.0f;  // Invalid
    }

    float resistance;

    if (pull_up) {
        // Pull-up configuration:
        // Vout = Vref * Rt / (Rbias + Rt)
        // Rt = Rbias * Vout / (Vref - Vout)
        resistance = bias_resistor * voltage / (vref - voltage);
    } else {
        // Pull-down configuration:
        // Vout = Vref * Rbias / (Rbias + Rt)
        // Rt = Rbias * (Vref - Vout) / Vout
        resistance = bias_resistor * (vref - voltage) / voltage;
    }

    return resistance;
}
