/**
 * @file tunerstudio.h
 * @brief TunerStudio Protocol Implementation for Teensy 3.5
 * @version 1.0.0
 * @date 2026-02-22
 *
 * TunerStudio protocol implementation based on rusEFI 2025
 * Compatible with TunerStudio 3.x+ for ECU configuration and monitoring
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#ifndef TUNERSTUDIO_H
#define TUNERSTUDIO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// TunerStudio Protocol Constants
//=============================================================================

// Command identifiers (from rusEFI)
#define TS_COMMAND_QUERY               0x00
#define TS_COMMAND_OUTPUT_CHANNELS     0x01
#define TS_COMMAND_READ_PAGE           0x02
#define TS_COMMAND_WRITE_CHUNK         0x03
#define TS_COMMAND_BURN                0x04
#define TS_COMMAND_CRC32_CHECK        0x05
#define TS_COMMAND_TEXT                0x06
#define TS_COMMAND_TEST                0x07
#define TS_COMMAND_READ_SCATTER        0x08

// Page identifiers
#define TS_PAGE_SETTINGS              0x0000
#define TS_PAGE_SCATTER_OFFSETS        0x0100
#define TS_PAGE_LTFT_TRIMS             0x0200

// Packet structure
#define TS_PACKET_HEADER_SIZE          3
#define TS_PACKET_TAIL_SIZE            4
#define TS_MAX_PACKET_SIZE             256
#define TS_MAX_DATA_SIZE              (TS_MAX_PACKET_SIZE - TS_PACKET_HEADER_SIZE - TS_PACKET_TAIL_SIZE)

// Response codes
#define TS_RESPONSE_OK                 0x00
#define TS_RESPONSE_ERROR              0x01
#define TS_RESPONSE_CRC_ERROR          0x02
#define TS_RESPONSE_OUT_OF_RANGE       0x03
#define TS_RESPONSE_UNRECOGNIZED       0x04

//=============================================================================
// TunerStudio Channel Types
//=============================================================================

typedef enum {
    TS_CHANNEL_ENGINE_LOAD = 0,
    TS_CHANNEL_ENGINE_RPM,
    TS_CHANNEL_MAP,
    TS_CHANNEL_IAT,
    TS_CHANNEL_CLT,
    TS_CHANNEL_TPS,
    TS_CHANNEL_AFR,
    TS_CHANNEL_LAMBDA,
    TS_CHANNEL_INJ_TIMING,
    TS_CHANNEL_IGN_TIMING,
    TS_CHANNEL_VVT,
    TS_CHANNEL_FUEL_BASE,
    TS_CHANNEL_FUEL_TRIM,
    TS_CHANNEL_IGN_TRIM,
    TS_CHANNEL_DWELL,
    TS_CHANNEL_COIL_CHARGE,
    TS_CHANNEL_KNOCK_COUNT,
    TS_CHANNEL_WBO_LAMBDA,
    TS_CHANNEL_WBO_AFR,
    TS_CHANNEL_WBO_HEATER,
    TS_CHANNEL_WBO_TEMP,
    TS_CHANNEL_WBO_CURRENT,
    TS_CHANNEL_DEBUG_INT1,
    TS_CHANNEL_DEBUG_INT2,
    TS_CHANNEL_DEBUG_INT3,
    TS_CHANNEL_DEBUG_INT4,
    TS_CHANNEL_COUNT
} ts_channel_e;

//=============================================================================
// Data Structures
//=============================================================================

typedef struct {
    uint8_t command;
    uint16_t offset;
    uint8_t data[TS_MAX_DATA_SIZE];
    uint8_t data_size;
} ts_packet_t;

typedef struct {
    uint32_t queryCommandCounter;
    uint32_t outputChannelsCommandCounter;
    uint32_t readPageCommandsCounter;
    uint32_t readScatterCommandsCounter;
    uint32_t burnCommandCounter;
    uint32_t crc32CheckCommandCounter;
    uint32_t writeChunkCommandCounter;
    uint32_t totalCounter;
    uint32_t textCommandCounter;
    uint32_t testCommandCounter;
    uint32_t errorCounter;
    uint32_t errorUnderrunCounter;
    uint32_t errorOverrunCounter;
    uint32_t errorCrcCounter;
    uint32_t errorUnrecognizedCommand;
    uint32_t errorOutOfRange;
    uint32_t errorOther;
} ts_counters_t;

typedef struct {
    float values[TS_CHANNEL_COUNT];
    uint32_t timestamp;
    uint8_t active;
} ts_channels_t;

//=============================================================================
// Function Prototypes
//=============================================================================

// Core protocol functions
void tunerstudio_init(void);
void tunerstudio_update(void);
void tunerstudio_process_byte(uint8_t byte);
void tunerstudio_send_response(uint8_t response_code, const uint8_t* data, uint16_t size);

// Channel management
void tunerstudio_set_channel(ts_channel_e channel, float value);
float tunerstudio_get_channel(ts_channel_e channel);
void tunerstudio_update_channels(void);

// Configuration functions
void tunerstudio_read_page(uint16_t page, uint8_t* data, uint16_t size);
void tunerstudio_write_chunk(uint16_t page, uint16_t offset, const uint8_t* data, uint8_t size);
void tunerstudio_burn_page(uint16_t page);

// Utility functions
uint32_t tunerstudio_crc32(const uint8_t* data, uint32_t length);
uint32_t tunerstudio_get_timestamp(void);
void tunerstudio_debug(const char* message);

// Statistics
const ts_counters_t* tunerstudio_get_counters(void);
void tunerstudio_reset_counters(void);

//=============================================================================
// External Variables
//=============================================================================

extern ts_channels_t ts_channels;
extern ts_counters_t ts_counters;

#ifdef __cplusplus
}
#endif

#endif // TUNERSTUDIO_H
