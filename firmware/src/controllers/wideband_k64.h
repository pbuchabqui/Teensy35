/**
 * @file wideband_k64.h
 * @brief Wideband O2 Controller for Teensy 3.5
 * @version 1.0.0
 * @date 2026-02-21
 * 
 * Wideband O2 sensor controller implementation based on rusEFI 2025 updates
 * Supports LSU 4.9 and LSU 4.2 sensors with CAN communication
 * 
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#ifndef WIDEBAND_K64_H
#define WIDEBAND_K64_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Wideband Configuration
//=============================================================================

#define WIDEBAND_MAX_SENSORS    2
#define WIDEBAND_CAN_ID_BASE     0x180
#define WIDEBAND_CAN_ID_PING     0x100
#define WIDEBAND_CAN_ID_ACK      0x727573  // "rus" in ASCII

//=============================================================================
// Wideband Sensor Types
//=============================================================================

typedef enum {
    WIDEBAND_SENSOR_LSU42 = 0,
    WIDEBAND_SENSOR_LSU49 = 1,
    WIDEBAND_SENSOR_NTK = 2,
    WIDEBAND_SENSOR_BOSCH = 3
} wideband_sensor_type_t;

//=============================================================================
// Wideband Status
//=============================================================================

typedef enum {
    WIDEBAND_STATUS_INIT = 0,
    WIDEBAND_STATUS_HEATING = 1,
    WIDEBAND_STATUS_READY = 2,
    WIDEBAND_STATUS_ERROR = 3,
    WIDEBAND_STATUS_CALIBRATING = 4
} wideband_status_t;

//=============================================================================
// Wideband Data Structure
//=============================================================================

typedef struct {
    float lambda;              // Lambda value (normalized)
    float afr;                 // Air-Fuel Ratio
    float o2_percent;         // O2 percentage
    float temperature;         // Sensor temperature (°C)
    float pump_current;        // Pump current (mA)
    float heater_voltage;      // Heater voltage (V)
    uint16_t error_code;      // Error code
    wideband_status_t status;  // Sensor status
    uint32_t timestamp;       // Last update timestamp (ms)
} wideband_data_t;

//=============================================================================
// Wideband Configuration Structure
//=============================================================================

typedef struct {
    wideband_sensor_type_t sensor_type;
    float pump_current_offset;
    float temperature_offset;
    uint8_t can_id_offset;
    uint8_t heater_enabled;
    float heater_duty_cycle;
    uint8_t calibration_enabled;
} wideband_config_t;

//=============================================================================
// Wideband Function Prototypes
//=============================================================================

// Initialization
void wideband_init(void);
void wideband_shutdown(void);

// Configuration
void wideband_set_config(uint8_t sensor_index, const wideband_config_t* config);
void wideband_get_config(uint8_t sensor_index, wideband_config_t* config);

// Data access
void wideband_get_data(uint8_t sensor_index, wideband_data_t* data);
float wideband_get_lambda(uint8_t sensor_index);
float wideband_get_afr(uint8_t sensor_index);
float wideband_get_temperature(uint8_t sensor_index);
wideband_status_t wideband_get_status(uint8_t sensor_index);

// CAN communication
void wideband_handle_can_frame(uint32_t can_id, const uint8_t* data, uint8_t length);
void wideband_send_ping(uint8_t sensor_index);
void wideband_set_can_offset(uint8_t sensor_index, uint8_t offset);

// Calibration
void wideband_start_calibration(uint8_t sensor_index);
void wideband_stop_calibration(uint8_t sensor_index);
uint8_t wideband_is_calibrating(uint8_t sensor_index);

// Diagnostics
uint16_t wideband_get_error_code(uint8_t sensor_index);
const char* wideband_get_error_string(uint16_t error_code);
void wideband_clear_errors(uint8_t sensor_index);

//=============================================================================
// rusEFI Compatibility Functions
//=============================================================================

// rusEFI-style interface for compatibility
size_t getWidebandBus(void);
void sendWidebandInfo(void);
void handleWidebandCan(const uint32_t can_id, const uint8_t* data, uint8_t length);
void pingWideband(uint8_t hwIndex);
void setWidebandOffset(uint8_t hwIndex, uint8_t index);
void setWidebandSensorType(uint8_t hwIndex, uint8_t type);

#ifdef __cplusplus
}
#endif

#endif // WIDEBAND_K64_H
