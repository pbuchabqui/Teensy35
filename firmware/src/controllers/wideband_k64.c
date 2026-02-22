/**
 * @file wideband_k64.c
 * @brief Wideband O2 Controller for Teensy 3.5
 * @version 1.0.0
 * @date 2026-02-21
 * 
 * Wideband O2 sensor controller implementation based on rusEFI 2025 updates
 * 
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#include "wideband_k64.h"
#include "../hal/adc_k64.h"
#include "../hal/pwm_k64.h"
#include "../hal/can_k64.h"
#include <string.h>
#include <math.h>

//=============================================================================
// Global Variables
//=============================================================================

static wideband_data_t wideband_sensors[WIDEBAND_MAX_SENSORS];
static wideband_config_t wideband_configs[WIDEBAND_MAX_SENSORS];
static uint8_t wideband_initialized = 0;
static uint32_t wideband_last_update = 0;

//=============================================================================
// Private Function Prototypes
//=============================================================================

static float wideband_lambda_to_afr(float lambda, float stoich_ratio);
static float wideband_afr_to_lambda(float afr, float stoich_ratio);
static void wideband_update_sensor(uint8_t index);
static void wideband_send_can_data(uint8_t index);

//=============================================================================
// Private Functions
//=============================================================================

static float wideband_lambda_to_afr(float lambda, float stoich_ratio) {
    return lambda * stoich_ratio;
}

static float wideband_afr_to_lambda(float afr, float stoich_ratio) {
    return afr / stoich_ratio;
}

static void wideband_update_sensor(uint8_t index) {
    if (index >= WIDEBAND_MAX_SENSORS) {
        return;
    }
    
    wideband_data_t* data = &wideband_sensors[index];
    wideband_config_t* config = &wideband_configs[index];
    
    // Read ADC values (simplified for Teensy 3.5)
    uint16_t raw_lambda = adc_read(ADC_0, config->can_id_offset);     // Lambda input
    uint16_t raw_temp = adc_read(ADC_0, config->can_id_offset + 1);  // Temperature input
    
    // Convert ADC to voltage (0-3.3V range, 12-bit ADC)
    float lambda_voltage = (raw_lambda / 4096.0f) * 3.3f;
    float temp_voltage = (raw_temp / 4096.0f) * 3.3f;
    
    // Convert voltage to lambda (simplified linear conversion)
    // Real implementation would use sensor-specific curves
    data->lambda = 0.5f + (lambda_voltage / 3.3f) * 1.5f;  // 0.5 to 2.0 lambda range
    data->afr = wideband_lambda_to_afr(data->lambda, 14.7f);  // Gasoline stoich ratio
    
    // Convert temperature voltage to temperature (NTC thermistor approximation)
    float r_thermistor = (3.3f - temp_voltage) / temp_voltage * 10000.0f;  // 10k pullup
    // Simplified NTC calculation
    data->temperature = 1.0f / (1.0f/298.15f + (1.0f/3950.0f) * logf(r_thermistor/10000.0f)) - 273.15f;
    
    // Calculate pump current (simplified)
    data->pump_current = (data->lambda - 1.0f) * 10.0f;  // mA
    
    // Heater control (simplified)
    if (config->heater_enabled && data->temperature < 700.0f) {
        pwm_set_duty_cycle(PWM_FTM0, config->can_id_offset, config->heater_duty_cycle);
        data->heater_voltage = 12.0f * (config->heater_duty_cycle / 100.0f);
    } else {
        pwm_set_duty_cycle(PWM_FTM0, config->can_id_offset, 0);
        data->heater_voltage = 0.0f;
    }
    
    // Update status
    if (data->temperature < 600.0f) {
        data->status = WIDEBAND_STATUS_HEATING;
    } else if (fabsf(data->lambda - 1.0f) < 0.1f) {
        data->status = WIDEBAND_STATUS_READY;
    } else {
        data->status = WIDEBAND_STATUS_READY;  // Still ready but reading
    }
    
    data->timestamp = wideband_last_update;
}

static void wideband_send_can_data(uint8_t index) {
    if (index >= WIDEBAND_MAX_SENSORS) {
        return;
    }
    
    wideband_data_t* data = &wideband_sensors[index];
    wideband_config_t* config = &wideband_configs[index];
    
    // CAN message structure (8 bytes)
    uint8_t can_data[8];
    
    // Byte 0-1: Lambda (scaled by 1000)
    uint16_t lambda_scaled = (uint16_t)(data->lambda * 1000.0f);
    can_data[0] = (lambda_scaled >> 8) & 0xFF;
    can_data[1] = lambda_scaled & 0xFF;
    
    // Byte 2-3: Temperature (scaled by 10)
    uint16_t temp_scaled = (uint16_t)(data->temperature * 10.0f);
    can_data[2] = (temp_scaled >> 8) & 0xFF;
    can_data[3] = temp_scaled & 0xFF;
    
    // Byte 4: Status and error code
    can_data[4] = ((uint8_t)data->status & 0x0F) | ((data->error_code >> 8) & 0xF0);
    
    // Byte 5: Error code low byte
    can_data[5] = data->error_code & 0xFF;
    
    // Byte 6: Pump current (scaled by 10)
    can_data[6] = (uint8_t)(data->pump_current * 10.0f);
    
    // Byte 7: Reserved/flags
    can_data[7] = config->heater_enabled ? 0x01 : 0x00;
    
    // Send CAN message
    uint32_t can_id = WIDEBAND_CAN_ID_BASE + config->can_id_offset;
    can_send(can_id, can_data, 8);
}

//=============================================================================
// Public Functions
//=============================================================================

void wideband_init(void) {
    if (wideband_initialized) {
        return;
    }
    
    // Initialize sensor data structures
    memset(wideband_sensors, 0, sizeof(wideband_sensors));
    memset(wideband_configs, 0, sizeof(wideband_configs));
    
    // Default configuration
    for (int i = 0; i < WIDEBAND_MAX_SENSORS; i++) {
        wideband_configs[i].sensor_type = WIDEBAND_SENSOR_LSU49;
        wideband_configs[i].pump_current_offset = 0.0f;
        wideband_configs[i].temperature_offset = 0.0f;
        wideband_configs[i].can_id_offset = i;
        wideband_configs[i].heater_enabled = 1;
        wideband_configs[i].heater_duty_cycle = 50.0f;
        wideband_configs[i].calibration_enabled = 0;
        
        wideband_sensors[i].status = WIDEBAND_STATUS_INIT;
        wideband_sensors[i].lambda = 1.0f;
        wideband_sensors[i].afr = 14.7f;
        wideband_sensors[i].temperature = 25.0f;
        wideband_sensors[i].error_code = 0;
    }
    
    wideband_initialized = 1;
}

void wideband_shutdown(void) {
    if (!wideband_initialized) {
        return;
    }
    
    // Turn off heaters
    for (int i = 0; i < WIDEBAND_MAX_SENSORS; i++) {
        pwm_set_duty_cycle(PWM_FTM0, i, 0);
    }
    
    wideband_initialized = 0;
}

void wideband_set_config(uint8_t sensor_index, const wideband_config_t* config) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS || !config) {
        return;
    }
    
    memcpy(&wideband_configs[sensor_index], config, sizeof(wideband_config_t));
}

void wideband_get_config(uint8_t sensor_index, wideband_config_t* config) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS || !config) {
        return;
    }
    
    memcpy(config, &wideband_configs[sensor_index], sizeof(wideband_config_t));
}

void wideband_get_data(uint8_t sensor_index, wideband_data_t* data) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS || !data) {
        return;
    }
    
    memcpy(data, &wideband_sensors[sensor_index], sizeof(wideband_data_t));
}

float wideband_get_lambda(uint8_t sensor_index) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS) {
        return 1.0f;  // Default to stoichiometric
    }
    
    return wideband_sensors[sensor_index].lambda;
}

float wideband_get_afr(uint8_t sensor_index) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS) {
        return 14.7f;  // Default to gasoline stoich
    }
    
    return wideband_sensors[sensor_index].afr;
}

float wideband_get_temperature(uint8_t sensor_index) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS) {
        return 25.0f;  // Room temperature
    }
    
    return wideband_sensors[sensor_index].temperature;
}

wideband_status_t wideband_get_status(uint8_t sensor_index) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS) {
        return WIDEBAND_STATUS_ERROR;
    }
    
    return wideband_sensors[sensor_index].status;
}

void wideband_handle_can_frame(uint32_t can_id, const uint8_t* data, uint8_t length) {
    if (!wideband_initialized) {
        return;
    }
    
    // Handle ping requests
    if (can_id == WIDEBAND_CAN_ID_PING && length >= 1) {
        uint8_t sensor_index = data[0];
        if (sensor_index < WIDEBAND_MAX_SENSORS) {
            wideband_send_can_data(sensor_index);
        }
    }
    
    // Handle configuration commands
    if ((can_id & 0xFF0) == WIDEBAND_CAN_ID_BASE && length >= 2) {
        uint8_t sensor_index = can_id & 0x00F;
        if (sensor_index < WIDEBAND_MAX_SENSORS) {
            uint8_t command = data[0];
            uint8_t value = data[1];
            
            switch (command) {
                case 0x01:  // Set CAN offset
                    wideband_configs[sensor_index].can_id_offset = value;
                    break;
                case 0x02:  // Set sensor type
                    wideband_configs[sensor_index].sensor_type = (wideband_sensor_type_t)value;
                    break;
                case 0x03:  // Enable/disable heater
                    wideband_configs[sensor_index].heater_enabled = value ? 1 : 0;
                    break;
                default:
                    break;
            }
        }
    }
}

void wideband_send_ping(uint8_t sensor_index) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS) {
        return;
    }
    
    uint8_t ping_data[1] = {sensor_index};
    can_send(WIDEBAND_CAN_ID_PING, ping_data, 1);
}

void wideband_set_can_offset(uint8_t sensor_index, uint8_t offset) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS) {
        return;
    }
    
    wideband_configs[sensor_index].can_id_offset = offset;
}

void wideband_start_calibration(uint8_t sensor_index) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS) {
        return;
    }
    
    wideband_configs[sensor_index].calibration_enabled = 1;
    wideband_sensors[sensor_index].status = WIDEBAND_STATUS_CALIBRATING;
}

void wideband_stop_calibration(uint8_t sensor_index) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS) {
        return;
    }
    
    wideband_configs[sensor_index].calibration_enabled = 0;
    wideband_sensors[sensor_index].status = WIDEBAND_STATUS_READY;
}

uint8_t wideband_is_calibrating(uint8_t sensor_index) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS) {
        return 0;
    }
    
    return wideband_configs[sensor_index].calibration_enabled;
}

uint16_t wideband_get_error_code(uint8_t sensor_index) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS) {
        return 0xFFFF;
    }
    
    return wideband_sensors[sensor_index].error_code;
}

const char* wideband_get_error_string(uint16_t error_code) {
    switch (error_code) {
        case 0x0000: return "No error";
        case 0x0001: return "Sensor not connected";
        case 0x0002: return "Heater fault";
        case 0x0003: return "Pump current fault";
        case 0x0004: return "Temperature out of range";
        case 0x0005: return "Calibration failed";
        default: return "Unknown error";
    }
}

void wideband_clear_errors(uint8_t sensor_index) {
    if (!wideband_initialized || sensor_index >= WIDEBAND_MAX_SENSORS) {
        return;
    }
    
    wideband_sensors[sensor_index].error_code = 0;
}

//=============================================================================
// rusEFI Compatibility Functions
//=============================================================================

size_t getWidebandBus(void) {
    return 0;  // Use CAN bus 0 for wideband
}

void sendWidebandInfo(void) {
    // Send wideband status information via CAN
    for (int i = 0; i < WIDEBAND_MAX_SENSORS; i++) {
        wideband_send_can_data(i);
    }
}

void handleWidebandCan(const uint32_t can_id, const uint8_t* data, uint8_t length) {
    wideband_handle_can_frame(can_id, data, length);
}

void pingWideband(uint8_t hwIndex) {
    wideband_send_ping(hwIndex);
}

void setWidebandOffset(uint8_t hwIndex, uint8_t index) {
    wideband_set_can_offset(hwIndex, index);
}

void setWidebandSensorType(uint8_t hwIndex, uint8_t type) {
    if (hwIndex < WIDEBAND_MAX_SENSORS) {
        wideband_configs[hwIndex].sensor_type = (wideband_sensor_type_t)type;
    }
}

//=============================================================================
// Update Function (called from main loop)
//=============================================================================

void wideband_update(void) {
    if (!wideband_initialized) {
        return;
    }
    
    wideband_last_update = 0;  // Would use actual timestamp
    
    // Update all sensors
    for (int i = 0; i < WIDEBAND_MAX_SENSORS; i++) {
        wideband_update_sensor(i);
    }
    
    // Send CAN data periodically
    static uint32_t last_can_send = 0;
    if (wideband_last_update - last_can_send > 100) {  // 10Hz
        for (int i = 0; i < WIDEBAND_MAX_SENSORS; i++) {
            wideband_send_can_data(i);
        }
        last_can_send = wideband_last_update;
    }
}
