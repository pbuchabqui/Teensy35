/**
 * @file tunerstudio.c
 * @brief TunerStudio Protocol Implementation for Teensy 3.5
 * @version 1.0.0
 * @date 2026-02-22
 *
 * Simplified TunerStudio protocol implementation based on rusEFI 2025
 * 
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#include "tunerstudio.h"
#include "../../hal/uart_k64.h"
#include <string.h>

//=============================================================================
// Global Variables
//=============================================================================

ts_channels_t ts_channels;
ts_counters_t ts_counters;
static uint8_t ts_buffer[TS_MAX_PACKET_SIZE];
static uint16_t ts_buffer_index = 0;
static uint8_t ts_packet_state = 0;
static uint8_t ts_expected_size = 0;
static uint32_t ts_last_timestamp = 0;

//=============================================================================
// CRC32 Implementation (simplified)
//=============================================================================

static uint32_t crc32_table[256];
static uint8_t crc32_table_initialized = 0;

static void init_crc32_table(void) {
    if (crc32_table_initialized) return;
    
    uint32_t polynomial = 0xEDB88320;
    for (int i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? polynomial : 0);
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = 1;
}

uint32_t tunerstudio_crc32(const uint8_t* data, uint32_t length) {
    init_crc32_table();
    
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

//=============================================================================
// UART Communication
//=============================================================================

static void uart_send_byte(uint8_t byte) {
    uart_putc(UART_0, byte);
}

static uint8_t uart_receive_byte(void) {
    return uart_getc(UART_0);
}

static uint8_t uart_bytes_available(void) {
    return uart_rx_ready(UART_0) ? 1 : 0;
}

//=============================================================================
// Packet Processing
//=============================================================================

static void process_query_command(void) {
    // Send query response
    uint8_t response[32];
    uint16_t offset = 0;
    
    // Signature
    response[offset++] = 'E';
    response[offset++] = 'C';
    response[offset++] = 'U';
    
    // Version
    response[offset++] = '2';
    response[offset++] = '.';
    response[offset++] = '2';
    response[offset++] = '0';
    
    // Build timestamp
    uint32_t timestamp = tunerstudio_get_timestamp();
    response[offset++] = (timestamp >> 24) & 0xFF;
    response[offset++] = (timestamp >> 16) & 0xFF;
    response[offset++] = (timestamp >> 8) & 0xFF;
    response[offset++] = timestamp & 0xFF;
    
    tunerstudio_send_response(TS_RESPONSE_OK, response, offset);
}

static void process_output_channels_command(void) {
    // Send channel data
    uint8_t response[TS_MAX_DATA_SIZE];
    uint16_t offset = 0;
    
    // Channel count
    response[offset++] = TS_CHANNEL_COUNT;
    
    // Channel values (4 bytes each)
    for (int i = 0; i < TS_CHANNEL_COUNT; i++) {
        float value = ts_channels.values[i];
        uint32_t value_int = *(uint32_t*)&value;
        response[offset++] = (value_int >> 24) & 0xFF;
        response[offset++] = (value_int >> 16) & 0xFF;
        response[offset++] = (value_int >> 8) & 0xFF;
        response[offset++] = value_int & 0xFF;
    }
    
    tunerstudio_send_response(TS_RESPONSE_OK, response, offset);
}

static void process_read_page_command(uint16_t page, uint16_t size) {
    uint8_t data[TS_MAX_DATA_SIZE];
    
    // Read page data
    tunerstudio_read_page(page, data, size);
    
    // Send response with data
    tunerstudio_send_response(TS_RESPONSE_OK, data, size);
}

static void process_write_chunk_command(uint16_t page, uint16_t offset, const uint8_t* data, uint8_t size) {
    // Write chunk data
    tunerstudio_write_chunk(page, offset, data, size);
    
    // Send acknowledgment
    tunerstudio_send_response(TS_RESPONSE_OK, NULL, 0);
}

static void process_burn_command(uint16_t page) {
    // Burn page to flash
    tunerstudio_burn_page(page);
    
    // Send acknowledgment
    tunerstudio_send_response(TS_RESPONSE_OK, NULL, 0);
}

static void process_packet(void) {
    if (ts_buffer_index < TS_PACKET_HEADER_SIZE) {
        return;  // Incomplete packet
    }
    
    // Parse packet header
    uint8_t command = ts_buffer[0];
    uint16_t offset = (ts_buffer[1] << 8) | ts_buffer[2];
    uint8_t data_size = ts_buffer_index - TS_PACKET_HEADER_SIZE;
    
    // Update counters
    ts_counters.totalCounter++;
    
    switch (command) {
        case TS_COMMAND_QUERY:
            ts_counters.queryCommandCounter++;
            process_query_command();
            break;
            
        case TS_COMMAND_OUTPUT_CHANNELS:
            ts_counters.outputChannelsCommandCounter++;
            process_output_channels_command();
            break;
            
        case TS_COMMAND_READ_PAGE:
            ts_counters.readPageCommandsCounter++;
            process_read_page_command(offset, data_size);
            break;
            
        case TS_COMMAND_WRITE_CHUNK:
            ts_counters.writeChunkCommandCounter++;
            tunerstudio_write_chunk(0, offset, &ts_buffer[TS_PACKET_HEADER_SIZE], data_size);
            break;
            
        case TS_COMMAND_BURN:
            ts_counters.burnCommandCounter++;
            process_burn_command(offset);
            break;
            
        case TS_COMMAND_CRC32_CHECK:
            ts_counters.crc32CheckCommandCounter++;
            // TODO: Implement CRC check
            tunerstudio_send_response(TS_RESPONSE_OK, NULL, 0);
            break;
            
        case TS_COMMAND_TEXT:
            ts_counters.textCommandCounter++;
            tunerstudio_debug("Text command received");
            tunerstudio_send_response(TS_RESPONSE_OK, NULL, 0);
            break;
            
        case TS_COMMAND_TEST:
            ts_counters.testCommandCounter++;
            tunerstudio_debug("Test command received");
            tunerstudio_send_response(TS_RESPONSE_OK, NULL, 0);
            break;
            
        default:
            ts_counters.errorUnrecognizedCommand++;
            tunerstudio_send_response(TS_RESPONSE_UNRECOGNIZED, NULL, 0);
            break;
    }
}

//=============================================================================
// Public Functions
//=============================================================================

void tunerstudio_init(void) {
    // Initialize channels
    memset(&ts_channels, 0, sizeof(ts_channels));
    ts_channels.timestamp = 0;
    ts_channels.active = 1;
    
    // Initialize counters
    memset(&ts_counters, 0, sizeof(ts_counters));
    
    // Initialize buffer
    ts_buffer_index = 0;
    ts_packet_state = 0;
    ts_expected_size = 0;
    
    tunerstudio_debug("TunerStudio initialized");
}

void tunerstudio_update(void) {
    // Process incoming bytes
    while (uart_bytes_available()) {
        uint8_t byte = uart_receive_byte();
        tunerstudio_process_byte(byte);
    }
    
    // Update channel values (simplified)
    tunerstudio_update_channels();
    
    // Update timestamp
    ts_last_timestamp++;
}

void tunerstudio_process_byte(uint8_t byte) {
    // Simple packet framing (simplified from rusEFI)
    if (ts_packet_state == 0) {
        // Wait for packet start
        if (byte == 0xAA) {  // Packet start byte (simplified)
            ts_packet_state = 1;
            ts_buffer_index = 0;
        }
    } else if (ts_packet_state == 1) {
        // Read packet size
        ts_expected_size = byte + TS_PACKET_HEADER_SIZE + TS_PACKET_TAIL_SIZE;
        ts_packet_state = 2;
    } else if (ts_packet_state == 2) {
        // Read packet data
        if (ts_buffer_index < ts_expected_size) {
            ts_buffer[ts_buffer_index++] = byte;
            
            if (ts_buffer_index >= ts_expected_size) {
                // Packet complete, process it
                process_packet();
                
                // Reset for next packet
                ts_buffer_index = 0;
                ts_packet_state = 0;
                ts_expected_size = 0;
            }
        }
    }
}

void tunerstudio_send_response(uint8_t response_code, const uint8_t* data, uint16_t size) {
    // Send response header
    uart_send_byte(response_code);
    uart_send_byte(size >> 8);
    uart_send_byte(size & 0xFF);
    
    // Send data if provided
    if (data != NULL && size > 0) {
        for (uint16_t i = 0; i < size; i++) {
            uart_send_byte(data[i]);
        }
    }
    
    // Send CRC32 (simplified - just send zeros for now)
    uart_send_byte(0x00);
    uart_send_byte(0x00);
    uart_send_byte(0x00);
    uart_send_byte(0x00);
}

void tunerstudio_set_channel(ts_channel_e channel, float value) {
    if (channel < TS_CHANNEL_COUNT) {
        ts_channels.values[channel] = value;
    }
}

float tunerstudio_get_channel(ts_channel_e channel) {
    if (channel < TS_CHANNEL_COUNT) {
        return ts_channels.values[channel];
    }
    return 0.0f;
}

void tunerstudio_update_channels(void) {
    // Update engine-related channels (simplified)
    static uint32_t counter = 0;
    
    // Simulate engine data (replace with real sensor data)
    ts_channels.values[TS_CHANNEL_ENGINE_RPM] = 800.0f + (counter % 2000);  // 800-2800 RPM
    ts_channels.values[TS_CHANNEL_MAP] = 100.0f;  // 100 kPa
    ts_channels.values[TS_CHANNEL_IAT] = 25.0f + (counter % 50);    // 25-75°C
    ts_channels.values[TS_CHANNEL_CLT] = 80.0f + (counter % 40);    // 80-120°C
    ts_channels.values[TS_CHANNEL_AFR] = 14.7f;  // Stoichiometric
    ts_channels.values[TS_CHANNEL_LAMBDA] = 1.0f;  // Stoichiometric
    
    counter++;
}

void tunerstudio_read_page(uint16_t page, uint8_t* data, uint16_t size) {
    // TODO: Implement page reading from flash/EEPROM
    memset(data, 0, size);
    
    tunerstudio_debug("Page read requested");
}

void tunerstudio_write_chunk(uint16_t page, uint16_t offset, const uint8_t* data, uint8_t size) {
    // TODO: Implement chunk writing to flash/EEPROM
    (void)page;
    (void)offset;
    (void)data;
    (void)size;
    
    tunerstudio_debug("Chunk write requested");
}

void tunerstudio_burn_page(uint16_t page) {
    // TODO: Implement page burning to flash
    (void)page;
    
    tunerstudio_debug("Page burn requested");
}

uint32_t tunerstudio_get_timestamp(void) {
    // TODO: Implement real timestamp
    return ts_last_timestamp;
}

void tunerstudio_debug(const char* message) {
    // Send debug message via UART
    uart_puts(UART_0, "[TS_DEBUG] ");
    uart_puts(UART_0, message);
    uart_puts(UART_0, "\r\n");
}

const ts_counters_t* tunerstudio_get_counters(void) {
    return &ts_counters;
}

void tunerstudio_reset_counters(void) {
    memset(&ts_counters, 0, sizeof(ts_counters));
}
