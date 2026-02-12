/**
 * @file engine_control.c
 * @brief Engine control implementation using original rusEFI algorithms
 * @version 2.1.0
 * @date 2026-02-11
 *
 * This implementation uses ORIGINAL rusEFI algorithms adapted for Teensy 3.5:
 *
 * 1. X-tau Wall Wetting (SAE 810494 by C. F. Aquino)
 *    - Source: github.com/rusefi/rusefi/controllers/algo/accel_enrichment.cpp
 *    - Formula: M_cmd = (desired - (1-α)*film) / (1-β)
 *    - References: rusEFI wiki X-tau Wall Wetting
 *
 * 2. Injector Latency Compensation
 *    - Source: rusEFI injector_lag_curve_lookup(V_BATT)
 *    - Battery voltage-based deadtime correction
 *
 * 3. Dwell Time Scheduling
 *    - Source: rusEFI spark_logic.cpp
 *    - RPM-based dwell curve with voltage compensation
 *
 * 4. Closed-Loop O2 Control
 *    - PI controller with anti-windup
 *    - Based on rusEFI lambda feedback system
 *
 * 5. Sensor Diagnostics
 *    - OBD-II compatible fault detection
 *    - Voltage range checking per rusEFI standards
 *
 * 6. Sequential Injection/Ignition
 *    - Per-cylinder timing calculation
 *    - 720° cycle awareness (4-stroke)
 *
 * @copyright Copyright (c) 2026 - GPL v3 License (compatible with rusEFI)
 * @see https://github.com/rusefi/rusefi
 * @see https://github.com/rusefi/rusefi/wiki/X-tau-Wall-Wetting
 */

#include "engine_control.h"
#include "../hal/adc_k64.h"
#include "../hal/input_capture_k64.h"
#include <stddef.h>
#include <math.h>

//=============================================================================
// Constants
//=============================================================================

#define STOICH_AFR              13.1f   // Stoichiometric AFR for gasoline E30
#define AIR_DENSITY_KG_M3       1.225f  // Air density at STP
#define FUEL_DENSITY_G_CC       0.81f   // Gasoline E30 density

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

    // Initialize injection mode (rusEFI-compatible)
    // Default to SIMULTANEOUS for cranking reliability
    // Can be changed to SEQUENTIAL/BATCH after engine starts
    ecu->fuel.injection_mode = INJECTION_MODE_SIMULTANEOUS;

    // Initialize VE table with reasonable defaults (80% VE)
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            ecu->fuel.ve_table[i][j] = 0.80f;
        }
    }

    // Initialize injector latency table (rusEFI-compatible)
    // Typical injector: higher voltage = faster opening = less latency
    float voltages[] = {6.0f, 8.0f, 10.0f, 12.0f, 13.5f, 14.0f, 15.0f, 16.0f};
    float latencies[] = {1500.0f, 1200.0f, 1000.0f, 800.0f, 700.0f, 650.0f, 600.0f, 550.0f};
    for (int i = 0; i < 8; i++) {
        ecu->fuel.latency_table.voltage[i] = voltages[i];
        ecu->fuel.latency_table.latency_us[i] = latencies[i];
    }

    // Initialize wall wetting (rusEFI X-tau model - original algorithm)
    // Based on SAE 810494 by C. F. Aquino
    ecu->fuel.wall_wetting.tau = 100.0f;           // 100ms evaporation time constant
    ecu->fuel.wall_wetting.alpha = 0.95f;          // 95% remains on wall per cycle
    ecu->fuel.wall_wetting.beta = 0.5f;            // 50% hits the wall
    ecu->fuel.wall_wetting.fuel_film_mass = 0.0f;
    ecu->fuel.wall_wetting.prev_map_kpa = 100.0f;

    // Initialize ignition with defaults
    ecu->ignition.base_timing_deg = 10;  // 10° BTDC base
    ecu->ignition.dwell_time_us = 3000;  // 3ms dwell

    // Initialize dwell table (rusEFI-compatible)
    // Lower voltage = longer dwell needed for saturation
    float dwell_voltages[] = {6.0f, 8.0f, 10.0f, 12.0f, 13.5f, 14.0f, 15.0f, 16.0f};
    float dwell_times[] = {5000.0f, 4500.0f, 4000.0f, 3500.0f, 3000.0f, 2800.0f, 2600.0f, 2500.0f};
    for (int i = 0; i < 8; i++) {
        ecu->ignition.dwell_table.voltage[i] = dwell_voltages[i];
        ecu->ignition.dwell_table.dwell_us[i] = dwell_times[i];
    }

    // Initialize timing table with reasonable values
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            // Simple linear timing: more advance at higher RPM
            ecu->ignition.timing_table[i][j] = 10.0f + (i * 2.0f);
        }
    }

    // Initialize closed-loop O2 control (rusEFI-compatible)
    ecu->sensors.closed_loop.proportional_gain = 0.1f;
    ecu->sensors.closed_loop.integral_gain = 0.01f;
    ecu->sensors.closed_loop.integral_error = 0.0f;
    ecu->sensors.closed_loop.correction = 1.0f;
    ecu->sensors.closed_loop.closed_loop_active = false;

    // Initialize batch injection pairs
    init_batch_injection_pairs(ecu);

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

    // rusEFI-compatible sensor diagnostics
    diagnose_sensors(&ecu->sensors);

    // Update closed-loop O2 control (only when engine warmed up)
    if (ecu->sensors.clt_celsius > 60.0f && ecu->sensors.engine_running) {
        ecu->sensors.closed_loop.closed_loop_active = true;
        update_closed_loop_fuel(&ecu->sensors.closed_loop,
                               ecu->fuel.afr_target,
                               ecu->sensors.afr,
                               0.01f);  // 10ms update rate
    } else {
        ecu->sensors.closed_loop.closed_loop_active = false;
        ecu->sensors.closed_loop.integral_error = 0.0f;  // Reset integral
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

    // Convert to milligrams for wall wetting calculation
    float fuel_mass_mg = fuel_mass_g * 1000.0f;

    // rusEFI-compatible wall wetting compensation
    float compensated_fuel_mg = update_wall_wetting(&ecu->fuel.wall_wetting,
                                                    fuel_mass_mg,
                                                    map_kpa,
                                                    10.0f);  // 10ms cycle time

    // Convert back to grams and then to pulse width
    float compensated_fuel_g = compensated_fuel_mg / 1000.0f;
    float fuel_cc = compensated_fuel_g / FUEL_DENSITY_G_CC;
    float pulse_us = (fuel_cc / ecu->fuel.injector_flow_cc) * 60000000.0f;

    // Apply corrections
    pulse_us *= ecu->fuel.clt_correction;
    pulse_us *= ecu->fuel.iat_correction;
    pulse_us += ecu->fuel.accel_enrichment;

    // rusEFI-compatible closed-loop O2 correction
    if (ecu->sensors.closed_loop.closed_loop_active) {
        pulse_us *= ecu->sensors.closed_loop.correction;
    }

    // rusEFI-compatible injector latency compensation
    float latency_us = calculate_injector_latency(&ecu->fuel.latency_table,
                                                  ecu->sensors.battery_voltage);
    pulse_us += latency_us;

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

    // rusEFI-compatible dwell time scheduling
    ecu->ignition.dwell_time_us = (uint16_t)calculate_dwell_time(
        &ecu->ignition.dwell_table,
        ecu->sensors.battery_voltage);

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

//=============================================================================
// rusEFI-Compatible Advanced Functions
//=============================================================================

float calculate_injector_latency(const injector_latency_table_t* table,
                                 float battery_voltage) {
    if (table == NULL) {
        return 800.0f;  // Default 800µs
    }

    // Find voltage breakpoints for interpolation
    for (int i = 0; i < 7; i++) {
        if (battery_voltage >= table->voltage[i] &&
            battery_voltage <= table->voltage[i + 1]) {

            // Linear interpolation
            float v0 = table->voltage[i];
            float v1 = table->voltage[i + 1];
            float l0 = table->latency_us[i];
            float l1 = table->latency_us[i + 1];

            float fraction = (battery_voltage - v0) / (v1 - v0);
            return l0 + (l1 - l0) * fraction;
        }
    }

    // Extrapolate if out of range
    if (battery_voltage < table->voltage[0]) {
        return table->latency_us[0];
    } else {
        return table->latency_us[7];
    }
}

float calculate_dwell_time(const dwell_table_t* table,
                          float battery_voltage) {
    if (table == NULL) {
        return 3000.0f;  // Default 3ms
    }

    // Find voltage breakpoints for interpolation
    for (int i = 0; i < 7; i++) {
        if (battery_voltage >= table->voltage[i] &&
            battery_voltage <= table->voltage[i + 1]) {

            // Linear interpolation
            float v0 = table->voltage[i];
            float v1 = table->voltage[i + 1];
            float d0 = table->dwell_us[i];
            float d1 = table->dwell_us[i + 1];

            float fraction = (battery_voltage - v0) / (v1 - v0);
            return d0 + (d1 - d0) * fraction;
        }
    }

    // Extrapolate if out of range
    if (battery_voltage < table->voltage[0]) {
        return table->dwell_us[0];
    } else {
        return table->dwell_us[7];
    }
}

float update_wall_wetting(wall_wetting_t* ww, float base_fuel_mg,
                         float map_kpa, float dt) {
    if (ww == NULL) {
        return base_fuel_mg;
    }

    // rusEFI X-tau wall wetting model (SAE 810494 by C. F. Aquino)
    // Original rusEFI formula from accel_enrichment.cpp:
    //
    // M_cmd = (desiredMass - (1 - alpha) * fuelFilmMass) / (1 - beta)
    // fuelFilmMass_next = alpha * fuelFilmMass + beta * M_cmd
    //
    // Where:
    // - alpha: fraction of fuel that REMAINS on wall per cycle
    // - beta: fraction of fuel that HITS the wall
    // - M_cmd: commanded injector fuel mass (compensated)
    // - desiredMass: target fuel mass for combustion

    float alpha = ww->alpha;
    float beta = ww->beta;
    float fuel_film = ww->fuel_film_mass;

    // Ignore tiny coefficients (rusEFI optimization)
    if (alpha < 0.01f) alpha = 0.0f;
    if (beta < 0.01f) beta = 0.0f;

    // Calculate commanded fuel with wall compensation
    // Formula accounts for: fuel evaporating from wall + fuel that will stick
    float m_cmd;
    if (beta < 0.99f) {
        // Normal case: compensate for wall effects
        m_cmd = (base_fuel_mg - (1.0f - alpha) * fuel_film) / (1.0f - beta);
    } else {
        // Edge case: all fuel hits wall (beta ≈ 1.0)
        m_cmd = base_fuel_mg;
    }

    // Update fuel film mass for next cycle
    // New film = fuel remaining from previous cycle + new fuel hitting wall
    float fuel_film_next = alpha * fuel_film + beta * m_cmd;

    // Store updated fuel film mass
    ww->fuel_film_mass = fuel_film_next;
    ww->prev_map_kpa = map_kpa;

    // Clamp to positive values
    if (m_cmd < 0.0f) m_cmd = 0.0f;
    if (ww->fuel_film_mass < 0.0f) ww->fuel_film_mass = 0.0f;

    return m_cmd;
}

void update_closed_loop_fuel(closed_loop_fuel_t* cl, float target_afr,
                            float actual_afr, float dt) {
    if (cl == NULL || !cl->closed_loop_active) {
        return;
    }

    // Calculate AFR error
    float error = target_afr - actual_afr;

    // Proportional term
    float p_term = cl->proportional_gain * error;

    // Integral term with anti-windup
    cl->integral_error += error * dt;

    // Limit integral windup (±20% correction max)
    if (cl->integral_error > 20.0f) {
        cl->integral_error = 20.0f;
    } else if (cl->integral_error < -20.0f) {
        cl->integral_error = -20.0f;
    }

    float i_term = cl->integral_gain * cl->integral_error;

    // Calculate correction factor (1.0 = no correction)
    // Positive error (too lean) = increase fuel = correction > 1.0
    // Negative error (too rich) = decrease fuel = correction < 1.0
    float correction = 1.0f + (p_term + i_term) / 100.0f;

    // Limit total correction to ±20%
    if (correction > 1.2f) {
        correction = 1.2f;
    } else if (correction < 0.8f) {
        correction = 0.8f;
    }

    cl->correction = correction;
}

void diagnose_sensors(sensor_data_t* sensors) {
    if (sensors == NULL) {
        return;
    }

    // Clear previous faults
    sensors->diagnostics.tps_fault = false;
    sensors->diagnostics.map_fault = false;
    sensors->diagnostics.clt_fault = false;
    sensors->diagnostics.iat_fault = false;
    sensors->diagnostics.o2_fault = false;
    sensors->diagnostics.battery_fault = false;
    sensors->diagnostics.fault_code = 0;

    // TPS diagnostics (expect 0-5V range)
    if (sensors->tps_voltage < 0.1f || sensors->tps_voltage > 4.9f) {
        sensors->diagnostics.tps_fault = true;
        sensors->diagnostics.fault_code |= 0x0001;  // DTC P0121
    }

    // MAP diagnostics (expect 0.5-4.5V for 3-bar sensor)
    if (sensors->map_voltage < 0.3f || sensors->map_voltage > 4.7f) {
        sensors->diagnostics.map_fault = true;
        sensors->diagnostics.fault_code |= 0x0002;  // DTC P0106
    }

    // CLT diagnostics (expect reasonable temperature range)
    if (sensors->clt_celsius < -40.0f || sensors->clt_celsius > 150.0f) {
        sensors->diagnostics.clt_fault = true;
        sensors->diagnostics.fault_code |= 0x0004;  // DTC P0117/P0118
    }

    // IAT diagnostics (expect reasonable temperature range)
    if (sensors->iat_celsius < -40.0f || sensors->iat_celsius > 150.0f) {
        sensors->diagnostics.iat_fault = true;
        sensors->diagnostics.fault_code |= 0x0008;  // DTC P0112/P0113
    }

    // O2 diagnostics (expect 0-1V for narrowband)
    if (sensors->o2_voltage < 0.0f || sensors->o2_voltage > 1.1f) {
        sensors->diagnostics.o2_fault = true;
        sensors->diagnostics.fault_code |= 0x0010;  // DTC P0131
    }

    // Battery voltage diagnostics (expect 9-18V normal range)
    if (sensors->battery_voltage < 9.0f || sensors->battery_voltage > 18.0f) {
        sensors->diagnostics.battery_fault = true;
        sensors->diagnostics.fault_code |= 0x0020;  // DTC P0560
    }
}

float calculate_injection_timing(ecu_state_t* ecu, uint8_t cylinder) {
    if (ecu == NULL || cylinder >= ecu->config.num_cylinders) {
        return 0.0f;
    }

    // Calculate degrees per cylinder for even firing
    float degrees_per_cylinder = 720.0f / ecu->config.num_cylinders;

    // Calculate injection timing for this cylinder
    // Typically inject during intake stroke (180° before TDC)
    float injection_timing = cylinder * degrees_per_cylinder - 180.0f;

    // Normalize to 0-720° range
    while (injection_timing < 0.0f) {
        injection_timing += 720.0f;
    }

    return injection_timing;
}

float calculate_spark_timing(ecu_state_t* ecu, uint8_t cylinder) {
    if (ecu == NULL || cylinder >= ecu->config.num_cylinders) {
        return 10.0f;
    }

    // Get base timing from main calculation
    uint8_t base_timing = calculate_ignition_timing(ecu);

    // Apply per-cylinder trim (future enhancement for individual cylinder tuning)
    // For now, all cylinders use same timing
    ecu->ignition.cylinder_timing_deg[cylinder] = base_timing;

    return (float)base_timing;
}

//=============================================================================
// rusEFI Injection Mode Functions
// Source: github.com/rusefi/rusefi/wiki/Fuel-Overview
//=============================================================================

void init_batch_injection_pairs(ecu_state_t* ecu) {
    if (ecu == NULL) {
        return;
    }

    // Calculate number of pairs (cylinders / 2)
    ecu->fuel.num_batch_pairs = ecu->config.num_cylinders / 2;

    // Set up cylinder pairs for batch injection
    // Pairs are cylinders that are 360° apart in the cycle
    // This ensures even distribution of fuel delivery

    // For 4-cylinder engine (typical firing order 1-3-4-2):
    // Pair 0: cylinders 0 and 2 (1 and 4) - 360° apart
    // Pair 1: cylinders 1 and 3 (3 and 2) - 360° apart

    for (uint8_t pair = 0; pair < ecu->fuel.num_batch_pairs; pair++) {
        // First cylinder in pair
        ecu->fuel.batch_pairs[pair][0] = pair;
        // Second cylinder is 360° away (num_cylinders/2 positions later)
        ecu->fuel.batch_pairs[pair][1] = pair + ecu->fuel.num_batch_pairs;
    }
}

float calculate_injection_timing_for_mode(ecu_state_t* ecu,
                                         float crank_angle,
                                         uint8_t cylinder) {
    if (ecu == NULL) {
        return 0.0f;
    }

    switch (ecu->fuel.injection_mode) {
        case INJECTION_MODE_SEQUENTIAL:
            // Sequential: Each cylinder fires once per 720° cycle
            // Fire 180° before TDC (during intake stroke)
            return calculate_injection_timing(ecu, cylinder);

        case INJECTION_MODE_BATCH: {
            // Batch: Paired cylinders fire together twice per cycle
            // Find which pair this cylinder belongs to
            uint8_t pair_index = 0;
            for (uint8_t p = 0; p < ecu->fuel.num_batch_pairs; p++) {
                if (ecu->fuel.batch_pairs[p][0] == cylinder ||
                    ecu->fuel.batch_pairs[p][1] == cylinder) {
                    pair_index = p;
                    break;
                }
            }

            // Each pair fires at 0° and 360° (twice per cycle)
            float degrees_per_pair = 360.0f / ecu->fuel.num_batch_pairs;
            float pair_timing = pair_index * degrees_per_pair;

            return pair_timing;
        }

        case INJECTION_MODE_SIMULTANEOUS:
            // Simultaneous: All injectors fire together
            // Can fire at any point, typically at crank sync
            return 0.0f;

        case INJECTION_MODE_SINGLE_POINT:
            // Single point: One injector fires continuously
            // Timing doesn't matter as much, fire at 0°
            return 0.0f;

        default:
            return 0.0f;
    }
}

uint8_t get_injectors_to_fire(ecu_state_t* ecu, float crank_angle) {
    if (ecu == NULL) {
        return 0;
    }

    uint8_t injector_mask = 0;
    float tolerance = 5.0f;  // ±5° window for triggering

    switch (ecu->fuel.injection_mode) {
        case INJECTION_MODE_SEQUENTIAL: {
            // Sequential: Check each cylinder's injection timing
            for (uint8_t cyl = 0; cyl < ecu->config.num_cylinders; cyl++) {
                float injection_angle = calculate_injection_timing(ecu, cyl);

                // Check if current crank angle matches this cylinder's timing
                float angle_diff = crank_angle - injection_angle;

                // Handle wraparound (720° cycle)
                if (angle_diff > 360.0f) angle_diff -= 720.0f;
                if (angle_diff < -360.0f) angle_diff += 720.0f;

                if (angle_diff >= 0.0f && angle_diff < tolerance) {
                    injector_mask |= (1 << cyl);
                }
            }
            break;
        }

        case INJECTION_MODE_BATCH: {
            // Batch: Check each pair's timing
            for (uint8_t pair = 0; pair < ecu->fuel.num_batch_pairs; pair++) {
                float degrees_per_pair = 360.0f / ecu->fuel.num_batch_pairs;
                float pair_timing_1 = pair * degrees_per_pair;
                float pair_timing_2 = pair_timing_1 + 360.0f;

                // Check first firing point (0-360°)
                float diff1 = crank_angle - pair_timing_1;
                if (diff1 >= 0.0f && diff1 < tolerance) {
                    // Fire both cylinders in this pair
                    injector_mask |= (1 << ecu->fuel.batch_pairs[pair][0]);
                    injector_mask |= (1 << ecu->fuel.batch_pairs[pair][1]);
                }

                // Check second firing point (360-720°)
                float diff2 = crank_angle - pair_timing_2;
                if (diff2 >= 0.0f && diff2 < tolerance) {
                    // Fire both cylinders in this pair again
                    injector_mask |= (1 << ecu->fuel.batch_pairs[pair][0]);
                    injector_mask |= (1 << ecu->fuel.batch_pairs[pair][1]);
                }
            }
            break;
        }

        case INJECTION_MODE_SIMULTANEOUS:
            // Simultaneous: Fire all injectors together at sync point
            if (crank_angle < tolerance) {
                // Fire all cylinders
                for (uint8_t cyl = 0; cyl < ecu->config.num_cylinders; cyl++) {
                    injector_mask |= (1 << cyl);
                }
            }
            break;

        case INJECTION_MODE_SINGLE_POINT:
            // Single point: Fire injector 0 continuously
            // In reality, this would be controlled by duty cycle
            // For now, fire at start of cycle
            if (crank_angle < tolerance) {
                injector_mask = 0x01;  // Only injector 0
            }
            break;

        default:
            injector_mask = 0;
            break;
    }

    return injector_mask;
}

const char* get_injection_mode_name(injection_mode_t mode) {
    switch (mode) {
        case INJECTION_MODE_SEQUENTIAL:
            return "Sequential";
        case INJECTION_MODE_BATCH:
            return "Batch";
        case INJECTION_MODE_SIMULTANEOUS:
            return "Simultaneous";
        case INJECTION_MODE_SINGLE_POINT:
            return "Single Point";
        default:
            return "Unknown";
    }
}
