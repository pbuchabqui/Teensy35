/**
 * @file can_k64.h
 * @brief CAN bus driver for Kinetis K64 (Teensy 3.5)
 * @version 1.0.0
 * @date 2026-02-10
 *
 * This file provides FlexCAN functionality for vehicle network communication.
 * The MK64FX512 has one FlexCAN module with 16 message buffers.
 *
 * Features:
 * - Standard (11-bit) and extended (29-bit) identifiers
 * - 16 message buffers (TX/RX)
 * - Hardware message filtering
 * - Configurable baud rate (125k, 250k, 500k, 1M)
 * - Interrupt-driven reception
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#ifndef CAN_K64_H
#define CAN_K64_H

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// CAN Baud Rates
//=============================================================================

typedef enum {
    CAN_BAUD_125K  = 125000,
    CAN_BAUD_250K  = 250000,
    CAN_BAUD_500K  = 500000,
    CAN_BAUD_666K  = 666000,  // rusEFI 2025 update
    CAN_BAUD_1M    = 1000000,
} can_baud_t;

//=============================================================================
// CAN Message Structure
//=============================================================================

typedef struct {
    uint32_t id;                 // CAN identifier (11 or 29-bit)
    uint8_t data[8];            // Data bytes (0-8 bytes)
    uint8_t length;             // Data length (0-8)
    bool extended;              // Extended ID (29-bit) flag
    bool remote;                // Remote frame flag
} can_message_t;

//=============================================================================
// CAN Configuration
//=============================================================================

typedef struct {
    can_baud_t baud_rate;       // CAN baud rate
    bool enable_loopback;       // Loopback mode for testing
    bool enable_listen_only;    // Listen-only mode (no ACK)
} can_config_t;

//=============================================================================
// CAN Callback Function Type
//=============================================================================

typedef void (*can_rx_callback_t)(const can_message_t* msg);

//=============================================================================
// Function Prototypes
//=============================================================================

/**
 * @brief Initialize CAN bus
 *
 * @param config Pointer to CAN configuration
 * @return true if initialization successful
 */
bool can_init(const can_config_t* config);

/**
 * @brief Transmit CAN message
 *
 * @param msg Pointer to message to transmit
 * @return true if message queued successfully
 */
bool can_transmit(const can_message_t* msg);

/**
 * @brief Receive CAN message (polling)
 *
 * @param msg Pointer to buffer for received message
 * @param timeout_ms Timeout in milliseconds (0 = non-blocking)
 * @return true if message received
 */
bool can_receive(can_message_t* msg, uint32_t timeout_ms);

/**
 * @brief Register callback for received messages
 *
 * @param callback Function to call when message received
 */
void can_register_callback(can_rx_callback_t callback);

/**
 * @brief Check if CAN message is available
 *
 * @return true if message waiting
 */
bool can_available(void);

/**
 * @brief Get CAN bus error status
 *
 * @return Error count (0 = no errors)
 */
uint8_t can_get_error_count(void);

/**
 * @brief Check if CAN bus is operational
 *
 * @return true if bus OK
 */
bool can_is_bus_ok(void);

//=============================================================================
// High-Level CAN Functions
//=============================================================================

/**
 * @brief Send engine data over CAN
 *
 * Broadcasts RPM, TPS, MAP, CLT on standard CAN IDs
 *
 * @param rpm Engine RPM
 * @param tps Throttle position (%)
 * @param map Manifold pressure (kPa)
 * @param clt Coolant temperature (°C)
 */
void can_send_engine_data(uint16_t rpm, uint8_t tps, uint8_t map, int8_t clt);

/**
 * @brief Send wideband O2 data
 *
 * @param afr Air-fuel ratio
 * @param lambda Lambda value
 */
void can_send_wideband_data(float afr, float lambda);

#endif // CAN_K64_H
