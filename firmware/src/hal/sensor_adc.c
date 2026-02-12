/**
 * @file sensor_adc.c
 * @brief ADC Sensor Input System Implementation
 *
 * @version 2.5.0
 * @date 2026-02-12
 */

#include "sensor_adc.h"
#include <string.h>

// Arduino/Teensy ADC functions (will be available when compiled with Arduino)
#ifdef ARDUINO
    #include <Arduino.h>
#else
    // Stub functions for testing without Arduino
    static uint16_t analogRead(uint8_t pin) { (void)pin; return 0; }
    static void analogReadResolution(uint8_t bits) { (void)bits; }
    static void analogReadAveraging(uint8_t avg) { (void)avg; }
#endif

// ADC reference voltage (Teensy 3.5)
#define ADC_VREF  3.3f

// Sensor names for debugging
static const char* SENSOR_NAMES[SENSOR_COUNT] = {
    "MAP",
    "TPS",
    "CLT",
    "IAT",
    "O2",
    "Battery",
    "MAF",
    "Oil Pressure",
    "Fuel Pressure"
};

/**
 * @brief Initialize ADC sensor system
 */
void sensor_adc_init(sensor_adc_t* adc)
{
    if (adc == NULL) {
        return;
    }

    memset(adc, 0, sizeof(sensor_adc_t));

    // Set default configurations for all sensors
    for (int i = 0; i < SENSOR_COUNT; i++) {
        adc->config[i].resolution_bits = 12;  // Default 12-bit
        adc->config[i].averaging = 4;         // Average 4 samples
        adc->config[i].sample_interval_us = 10000;  // 10ms default
        adc->config[i].enabled = false;
    }

    adc->initialized = true;
}

/**
 * @brief Configure a sensor
 */
void sensor_adc_configure(sensor_adc_t* adc,
                          sensor_type_t sensor,
                          uint8_t pin,
                          uint8_t resolution,
                          uint8_t averaging,
                          uint32_t sample_interval_us)
{
    if (adc == NULL || sensor >= SENSOR_COUNT) {
        return;
    }

    adc->config[sensor].adc_pin = pin;
    adc->config[sensor].resolution_bits = resolution;
    adc->config[sensor].averaging = averaging;
    adc->config[sensor].sample_interval_us = sample_interval_us;
}

/**
 * @brief Enable/disable a sensor
 */
void sensor_adc_enable(sensor_adc_t* adc,
                       sensor_type_t sensor,
                       bool enable)
{
    if (adc == NULL || sensor >= SENSOR_COUNT) {
        return;
    }
    adc->config[sensor].enabled = enable;
}

/**
 * @brief Update all enabled sensors
 */
void sensor_adc_update_all(sensor_adc_t* adc,
                           uint32_t current_time_us)
{
    if (adc == NULL || !adc->initialized) {
        return;
    }

    // Update each enabled sensor if interval has elapsed
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (!adc->config[i].enabled) {
            continue;
        }

        uint32_t time_since_last = current_time_us - adc->config[i].last_sample_time;

        if (time_since_last >= adc->config[i].sample_interval_us) {
            sensor_adc_read_sensor(adc, (sensor_type_t)i, current_time_us);
        }
    }
}

/**
 * @brief Read a specific sensor immediately
 */
bool sensor_adc_read_sensor(sensor_adc_t* adc,
                            sensor_type_t sensor,
                            uint32_t current_time_us)
{
    if (adc == NULL || sensor >= SENSOR_COUNT) {
        return false;
    }

    if (!adc->config[sensor].enabled) {
        return false;
    }

    adc_config_t* cfg = &adc->config[sensor];
    adc_reading_t* reading = &adc->readings[sensor];

    // Configure ADC
    #ifdef ARDUINO
    analogReadResolution(cfg->resolution_bits);
    analogReadAveraging(cfg->averaging);
    #endif

    // Read ADC
    uint16_t raw = analogRead(cfg->adc_pin);

    // Calculate max value based on resolution
    uint16_t max_value = (1 << cfg->resolution_bits) - 1;

    // Convert to voltage
    float voltage = ((float)raw / (float)max_value) * ADC_VREF;

    // Store reading
    reading->raw_value = raw;
    reading->voltage = voltage;
    reading->timestamp_us = current_time_us;
    reading->valid = true;

    // Update statistics
    cfg->last_sample_time = current_time_us;
    adc->total_samples++;

    return true;
}

/**
 * @brief Get raw ADC reading
 */
const adc_reading_t* sensor_adc_get_reading(const sensor_adc_t* adc,
                                            sensor_type_t sensor)
{
    if (adc == NULL || sensor >= SENSOR_COUNT) {
        return NULL;
    }

    if (!adc->readings[sensor].valid) {
        return NULL;
    }

    return &adc->readings[sensor];
}

/**
 * @brief Get sensor voltage
 */
float sensor_adc_get_voltage(const sensor_adc_t* adc,
                             sensor_type_t sensor)
{
    if (adc == NULL || sensor >= SENSOR_COUNT) {
        return -1.0f;
    }

    if (!adc->readings[sensor].valid) {
        return -1.0f;
    }

    return adc->readings[sensor].voltage;
}

/**
 * @brief Get sensor raw value
 */
uint16_t sensor_adc_get_raw(const sensor_adc_t* adc,
                            sensor_type_t sensor)
{
    if (adc == NULL || sensor >= SENSOR_COUNT) {
        return 0;
    }

    if (!adc->readings[sensor].valid) {
        return 0;
    }

    return adc->readings[sensor].raw_value;
}

/**
 * @brief Check if sensor reading is valid
 */
bool sensor_adc_is_valid(const sensor_adc_t* adc,
                         sensor_type_t sensor)
{
    if (adc == NULL || sensor >= SENSOR_COUNT) {
        return false;
    }

    return adc->readings[sensor].valid;
}

/**
 * @brief Set default pin configuration
 */
void sensor_adc_set_default_config(sensor_adc_t* adc)
{
    if (adc == NULL) {
        return;
    }

    // Default pin assignments for Teensy 3.5
    // Using A0-A7 (pins 14-21)

    // MAP sensor - A0 (pin 14)
    sensor_adc_configure(adc, SENSOR_MAP, 14, 12, 4, 10000);  // 10ms
    sensor_adc_enable(adc, SENSOR_MAP, true);

    // TPS sensor - A1 (pin 15)
    sensor_adc_configure(adc, SENSOR_TPS, 15, 12, 4, 10000);  // 10ms
    sensor_adc_enable(adc, SENSOR_TPS, true);

    // CLT sensor - A2 (pin 16)
    sensor_adc_configure(adc, SENSOR_CLT, 16, 12, 8, 100000);  // 100ms (slow)
    sensor_adc_enable(adc, SENSOR_CLT, true);

    // IAT sensor - A3 (pin 17)
    sensor_adc_configure(adc, SENSOR_IAT, 17, 12, 8, 100000);  // 100ms (slow)
    sensor_adc_enable(adc, SENSOR_IAT, true);

    // O2 sensor - A4 (pin 18)
    sensor_adc_configure(adc, SENSOR_O2, 18, 12, 4, 50000);  // 50ms
    sensor_adc_enable(adc, SENSOR_O2, true);

    // Battery voltage - A5 (pin 19)
    sensor_adc_configure(adc, SENSOR_BATTERY, 19, 12, 8, 100000);  // 100ms
    sensor_adc_enable(adc, SENSOR_BATTERY, true);

    // Optional sensors (disabled by default)
    // MAF - A6 (pin 20)
    sensor_adc_configure(adc, SENSOR_MAF, 20, 12, 4, 10000);
    sensor_adc_enable(adc, SENSOR_MAF, false);

    // Oil pressure - A7 (pin 21)
    sensor_adc_configure(adc, SENSOR_OIL_PRESSURE, 21, 12, 4, 50000);
    sensor_adc_enable(adc, SENSOR_OIL_PRESSURE, false);

    // Fuel pressure - A8 (pin 22)
    sensor_adc_configure(adc, SENSOR_FUEL_PRESSURE, 22, 12, 4, 50000);
    sensor_adc_enable(adc, SENSOR_FUEL_PRESSURE, false);
}

/**
 * @brief Get sensor name string
 */
const char* sensor_adc_get_name(sensor_type_t sensor)
{
    if (sensor >= SENSOR_COUNT) {
        return "Unknown";
    }
    return SENSOR_NAMES[sensor];
}

/**
 * @brief Get ADC statistics
 */
void sensor_adc_get_stats(const sensor_adc_t* adc,
                          uint32_t* total_samples,
                          uint32_t* error_count)
{
    if (adc == NULL) {
        return;
    }

    if (total_samples != NULL) {
        *total_samples = adc->total_samples;
    }

    if (error_count != NULL) {
        *error_count = adc->error_count;
    }
}
