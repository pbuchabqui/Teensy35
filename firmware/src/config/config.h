/**
 * @file config.h
 * @brief Configuration System for Teensy 3.5 rusEFI
 * @version 1.0.0
 * @date 2026-02-22
 *
 * Configuration storage and management system for rusEFI Teensy 3.5
 * Compatible with TunerStudio page-based configuration
 * 
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Configuration Page Definitions
//=============================================================================

#define CONFIG_PAGE_SETTINGS           0x0000
#define CONFIG_PAGE_SCATTER_OFFSETS    0x0100
#define CONFIG_PAGE_LTFT_TRIMS         0x0200
#define CONFIG_PAGE_FUEL_TRIMS         0x0300
#define CONFIG_PAGE_IGN_TRIMS         0x0400
#define CONFIG_PAGE_VE_TABLE          0x0500
#define CONFIG_PAGE_SPARK_TABLE       0x0600
#define CONFIG_PAGE_WBO_CONFIG        0x0700
#define CONFIG_PAGE_CAN_CONFIG        0x0800

#define CONFIG_PAGE_SIZE              1024  // 1KB per page
#define CONFIG_TOTAL_PAGES           8     // Total pages available

//=============================================================================
// Configuration Data Structures
//=============================================================================

typedef struct {
    // Engine Configuration
    uint16_t engine_type;
    uint16_t cylinders;
    uint16_t displacement;
    uint16_t stroke;
    uint16_t compression_ratio;
    uint16_t injector_size;
    uint16_t firing_order;
    
    // Fuel Configuration
    uint16_t fuel_base_pulse;
    uint16_t fuel_deadtime_12v;
    uint16_t fuel_deadtime_14v;
    uint16_t fuel_pressure;
    uint16_t fuel_temp_coefficient;
    uint16_t fuel_inj_timing;
    
    // Ignition Configuration
    uint16_t spark_dwell_12v;
    uint16_t spark_dwell_14v;
    uint16_t spark_gap;
    uint16_t spark_advance_idle;
    uint16_t spark_advance_load;
    uint16_t spark_advance_map;
    
    // Sensor Configuration
    uint16_t map_sensor_type;
    uint16_t iat_sensor_type;
    uint16_t clt_sensor_type;
    uint16_t tps_sensor_type;
    uint16_t o2_sensor_type;
    uint16_t baro_sensor_type;
    
    // Safety Configuration
    uint16_t rpm_limit;
    uint16_t map_limit;
    uint16_t tps_limit;
    uint16_t coolant_temp_limit;
    uint16_t oil_temp_limit;
    uint16_t knock_limit;
    
    // TunerStudio Configuration
    uint8_t serial_speed;
    uint8_t serial_enabled;
    uint8_t can_enabled;
    uint8_t wideband_enabled;
    uint8_t debug_enabled;
    
    // Reserved for future use
    uint8_t reserved[CONFIG_PAGE_SIZE - 64];
} config_engine_t;

typedef struct {
    // VE Table (16x16)
    uint16_t ve_table[16][16];
    
    // VE Table Configuration
    uint16_t rpm_bins[16];
    uint16_t map_bins[16];
    uint16_t ve_table_rpm_min;
    uint16_t ve_table_rpm_max;
    uint16_t ve_table_map_min;
    uint16_t ve_table_map_max;
    
    // Reserved for future use
    uint8_t reserved[CONFIG_PAGE_SIZE - 516];
} config_ve_table_t;

typedef struct {
    // Spark Table (16x16)
    uint16_t spark_table[16][16];
    
    // Spark Table Configuration
    uint16_t rpm_bins[16];
    uint16_t map_bins[16];
    uint16_t spark_table_rpm_min;
    uint16_t spark_table_rpm_max;
    uint16_t spark_table_map_min;
    uint16_t spark_table_map_max;
    
    // Reserved for future use
    uint8_t reserved[CONFIG_PAGE_SIZE - 516];
} config_spark_table_t;

//=============================================================================
// Configuration Management Functions
//=============================================================================

// Initialization
void config_init(void);
void config_reset_to_defaults(void);

// Storage Operations
int config_read_page(uint16_t page, uint8_t* buffer);
int config_write_page(uint16_t page, const uint8_t* buffer);
int config_burn_page(uint16_t page);

// Configuration Access
config_engine_t* config_get_engine(void);
config_ve_table_t* config_get_ve_table(void);
config_spark_table_t* config_get_spark_table(void);

// Validation
int config_validate_page(uint16_t page, const uint8_t* buffer);
int config_validate_all(void);

// Backup and Restore
int config_backup_all(void);
int config_restore_all(void);

//=============================================================================
// External Variables
//=============================================================================

extern config_engine_t config_engine;
extern config_ve_table_t config_ve_table;
extern config_spark_table_t config_spark_table;

#ifdef __cplusplus
}
#endif

#endif // CONFIG_H
