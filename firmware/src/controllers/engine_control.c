/**
 * @file engine_control.c
 * @brief Engine control implementation
 * @version 1.0.0
 * @date 2026-02-10
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#include "engine_control.h"
#include "../hal/adc_k64.h"
#include "../hal/input_capture_k64.h"
#include <math.h>

//=============================================================================
// Constants
//=============================================================================

#define STOICH_AFR              14.7f   // Stoichiometric AFR for gasoline
#define AIR_DENSITY_KG_M3       1.225f  // Air density at STP
#define FUEL_DENSITY_G_CC       0.75f   // Gasoline density

//=============================================================================
// Public Functions
//=============================================================================

void ecu_init(ecu_state_t* ecu, const engine_config_t* config) {
    if (ecu == NULL || config == NULL) {
        return;
    }

    // Copy configuration
    ecu->config = *config;

    // Initialize fuel control with defaults
    ecu->fuel.afr_target = STOICH_AFR;
    ecu->fuel.fuel_pressure_kpa = 300.0f;  // 3 bar typical
    ecu->fuel.injector_flow_cc = 300.0f;   // 300cc/min injector

    // Initialize VE table with reasonable defaults (80% VE)
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            ecu->fuel.ve_table[i][j] = 0.80f;
        }
    }

    // Initialize ignition with defaults
    ecu->ignition.base_timing_deg = 10;  // 10° BTDC base
    ecu->ignition.dwell_time_us = 3000;  // 3ms dwell

    // Initialize timing table with reasonable values
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            // Simple linear timing: more advance at higher RPM
            ecu->ignition.timing_table[i][j] = 10.0f + (i * 2.0f);
        }
    }

    // Initialize runtime state
    ecu->loop_count = 0;
    ecu->error_state = false;
}

void ecu_update_sensors(ecu_state_t* ecu) {
    if (ecu == NULL) {
        return;
    }

    // Read analog sensors (ADC)
    ecu->sensors.tps_voltage = adc_read_voltage(ADC_0, ADC0_DP0);
    ecu->sensors.map_voltage = adc_read_voltage(ADC_0, ADC0_DP1);
    ecu->sensors.clt_voltage = adc_read_voltage(ADC_0, ADC0_DM0);
    ecu->sensors.iat_voltage = adc_read_voltage(ADC_0, ADC0_DM1);
    ecu->sensors.o2_voltage = adc_read_voltage(ADC_0, ADC0_DP2);
    ecu->sensors.battery_voltage = adc_read_voltage(ADC_0, ADC0_DP3) * 5.0f;

    // Convert sensor voltages to engineering units
    ecu->sensors.tps_percent = convert_tps_voltage(ecu->sensors.tps_voltage);
    ecu->sensors.map_kpa = convert_map_voltage(ecu->sensors.map_voltage);
    ecu->sensors.clt_celsius = convert_temp_voltage(ecu->sensors.clt_voltage);
    ecu->sensors.iat_celsius = convert_temp_voltage(ecu->sensors.iat_voltage);
    ecu->sensors.afr = convert_o2_voltage(ecu->sensors.o2_voltage);

    // Get engine position and RPM
    ecu->sensors.rpm = get_engine_rpm();
    ecu->sensors.sync_locked = is_engine_synced();
    ecu->sensors.engine_running = (ecu->sensors.rpm > 100);

    engine_position_t* pos = get_engine_position();
    if (pos != NULL) {
        ecu->sensors.current_tooth = pos->tooth_count;
    }
}

uint32_t calculate_fuel_pulse(ecu_state_t* ecu) {
    if (ecu == NULL || !ecu->sensors.engine_running) {
        return 0;
    }

    // Basic fuel calculation using Speed-Density method
    // Fuel = (Displacement * RPM * MAP * VE) / (AFR * Air_Density)

    float displacement_liters = ecu->config.displacement_cc / 1000.0f;
    float rpm = (float)ecu->sensors.rpm;
    float map_kpa = ecu->sensors.map_kpa;

    // Lookup VE from table (simplified - use MAP and RPM)
    float ve = lookup_table_2d(ecu->fuel.ve_table,
                              rpm, map_kpa,
                              500.0f, 7000.0f,    // RPM range
                              20.0f, 100.0f);      // MAP range (kPa)

    // Calculate air mass per cycle (grams)
    float air_mass_g = (map_kpa * displacement_liters * ve) /
                       (0.287f * (ecu->sensors.iat_celsius + 273.15f));

    // Calculate required fuel mass (grams)
    float fuel_mass_g = air_mass_g / ecu->fuel.afr_target;

    // Convert to injector pulse width
    // pulse_width = (fuel_mass / injector_flow_rate) * 60,000,000 µs/min
    float fuel_cc = fuel_mass_g / FUEL_DENSITY_G_CC;
    float pulse_us = (fuel_cc / ecu->fuel.injector_flow_cc) * 60000000.0f;

    // Apply corrections
    pulse_us *= ecu->fuel.clt_correction;
    pulse_us *= ecu->fuel.iat_correction;
    pulse_us += ecu->fuel.accel_enrichment;

    // Clamp to reasonable range (0.5ms - 20ms)
    if (pulse_us < 500.0f) pulse_us = 500.0f;
    if (pulse_us > 20000.0f) pulse_us = 20000.0f;

    return (uint32_t)pulse_us;
}

uint8_t calculate_ignition_timing(ecu_state_t* ecu) {
    if (ecu == NULL || !ecu->sensors.engine_running) {
        return 10;  // Default 10° BTDC
    }

    // Lookup base timing from table
    float rpm = (float)ecu->sensors.rpm;
    float map_kpa = ecu->sensors.map_kpa;

    float base_timing = lookup_table_2d(ecu->ignition.timing_table,
                                       rpm, map_kpa,
                                       500.0f, 7000.0f,
                                       20.0f, 100.0f);

    // Apply corrections
    base_timing += ecu->ignition.clt_advance;
    base_timing += ecu->ignition.iat_advance;
    base_timing -= ecu->ignition.knock_retard;

    // Clamp to safe range (0-40° BTDC)
    if (base_timing < 0.0f) base_timing = 0.0f;
    if (base_timing > 40.0f) base_timing = 40.0f;

    return (uint8_t)base_timing;
}

//=============================================================================
// Sensor Conversion Functions
//=============================================================================

float convert_tps_voltage(float voltage) {
    // Linear conversion: 0V = 0%, 5V = 100%
    float percent = (voltage / 5.0f) * 100.0f;

    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;

    return percent;
}

float convert_map_voltage(float voltage) {
    // GM 3-bar MAP sensor: 0.5V = 0 kPa, 4.5V = 300 kPa
    float kpa = ((voltage - 0.5f) / 4.0f) * 300.0f;

    if (kpa < 0.0f) kpa = 0.0f;
    if (kpa > 300.0f) kpa = 300.0f;

    return kpa;
}

float convert_temp_voltage(float voltage) {
    // GM temperature sensor (NTC thermistor)
    // Simplified Steinhart-Hart equation

    // Convert voltage to resistance (assuming 2.49kΩ pullup)
    float r_pullup = 2490.0f;
    float resistance = (voltage * r_pullup) / (5.0f - voltage);

    // Steinhart-Hart coefficients for GM sensor
    float A = 0.001129148f;
    float B = 0.000234125f;
    float C = 0.0000000876741f;

    float log_r = logf(resistance);
    float temp_kelvin = 1.0f / (A + B * log_r + C * log_r * log_r * log_r);
    float temp_celsius = temp_kelvin - 273.15f;

    return temp_celsius;
}

float convert_o2_voltage(float voltage) {
    // Narrowband O2 sensor: 0.1V = lean, 0.9V = rich
    // Linear approximation: 0V = AFR 20, 1V = AFR 10

    float afr = 20.0f - (voltage * 10.0f);

    if (afr < 10.0f) afr = 10.0f;
    if (afr > 20.0f) afr = 20.0f;

    return afr;
}

//=============================================================================
// Table Lookup
//=============================================================================

float lookup_table_2d(const float table[16][16],
                     float x, float y,
                     float x_min, float x_max,
                     float y_min, float y_max) {
    // Normalize inputs to 0-15 range
    float x_norm = ((x - x_min) / (x_max - x_min)) * 15.0f;
    float y_norm = ((y - y_min) / (y_max - y_min)) * 15.0f;

    // Clamp to valid range
    if (x_norm < 0.0f) x_norm = 0.0f;
    if (x_norm > 15.0f) x_norm = 15.0f;
    if (y_norm < 0.0f) y_norm = 0.0f;
    if (y_norm > 15.0f) y_norm = 15.0f;

    // Get table indices
    int x_idx = (int)x_norm;
    int y_idx = (int)y_norm;

    // Handle edge case
    if (x_idx >= 15) x_idx = 14;
    if (y_idx >= 15) y_idx = 14;

    // Bilinear interpolation
    float x_frac = x_norm - x_idx;
    float y_frac = y_norm - y_idx;

    float v00 = table[y_idx][x_idx];
    float v10 = table[y_idx][x_idx + 1];
    float v01 = table[y_idx + 1][x_idx];
    float v11 = table[y_idx + 1][x_idx + 1];

    float v0 = v00 + (v10 - v00) * x_frac;
    float v1 = v01 + (v11 - v01) * x_frac;

    return v0 + (v1 - v0) * y_frac;
}
