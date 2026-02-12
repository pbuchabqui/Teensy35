/**
 * @file trigger_decoder_k64.h
 * @brief rusEFI-compatible Trigger Decoder for Teensy 3.5 (MK64FX512)
 *
 * Implements missing tooth detection and synchronization using rusEFI's
 * TriggerDecoderBase algorithm.
 *
 * @version 2.3.0
 * @date 2026-02-11
 *
 * Based on rusEFI:
 * - firmware/controllers/trigger/trigger_decoder.cpp
 * - firmware/controllers/trigger/trigger_decoder.h
 * - Class: TriggerDecoderBase
 *
 * References:
 * - https://rusefi.com/docs/html/class_trigger_decoder_base.html
 * - https://wiki.rusefi.com/Trigger/
 * - https://github.com/rusefi/rusefi/wiki/All-Supported-Triggers
 */

#ifndef TRIGGER_DECODER_K64_H
#define TRIGGER_DECODER_K64_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Trigger decoder structure (rusEFI-compatible)
 *
 * Implements missing tooth detection for crank position synchronization.
 * Compatible with rusEFI TriggerDecoderBase algorithm.
 */
typedef struct {
    // Tooth configuration
    uint8_t total_teeth;              ///< Total teeth per revolution (e.g., 36 for 36-1)
    uint8_t missing_teeth;            ///< Number of missing teeth (e.g., 1 for 36-1)
    uint8_t tooth_count;              ///< Current tooth index (0 to total_teeth-1)

    // Timing measurements
    uint32_t prev_tooth_time;         ///< Previous tooth timestamp (µs)
    uint32_t prev_tooth_period;       ///< Previous tooth period (µs)
    uint32_t current_tooth_period;    ///< Current tooth period (µs)

    // Synchronization parameters (rusEFI algorithm)
    float sync_ratio_from;            ///< Min ratio for sync gap (default: 1.5)
    float sync_ratio_to;              ///< Max ratio for sync gap (default: 3.0)

    // Synchronization state
    bool sync_locked;                 ///< Synchronization status
    uint8_t sync_point_tooth;         ///< Tooth index at sync point (0 for 36-1)
    uint32_t sync_count;              ///< Number of successful syncs
    uint32_t tooth_event_counter;     ///< Total tooth events received

    // Error tracking
    uint32_t sync_loss_count;         ///< Number of times sync was lost
    uint32_t last_sync_time;          ///< Timestamp of last successful sync

    // Callbacks (rusEFI pattern)
    void (*on_sync_callback)(void);           ///< Called when sync is achieved
    void (*on_tooth_callback)(uint8_t tooth); ///< Called on each valid tooth

} trigger_decoder_t;

/**
 * @brief Initialize trigger decoder
 *
 * Sets up the decoder for a specific trigger wheel pattern.
 *
 * @param decoder Pointer to decoder structure
 * @param teeth Total number of teeth (e.g., 36 for 36-1)
 * @param missing Number of missing teeth (e.g., 1 for 36-1)
 *
 * Example:
 *   trigger_decoder_t decoder;
 *   trigger_decoder_init(&decoder, 36, 1);  // 36-1 wheel
 */
void trigger_decoder_init(trigger_decoder_t* decoder,
                         uint8_t teeth,
                         uint8_t missing);

/**
 * @brief Process a tooth event (rusEFI algorithm)
 *
 * Called on each rising edge of the crank sensor signal.
 * Implements rusEFI's missing tooth detection algorithm:
 *
 *   ratio = current_period / previous_period
 *   if (ratio >= syncRatioFrom && ratio <= syncRatioTo):
 *       SYNC FOUND! Reset tooth_count to known position
 *
 * @param decoder Pointer to decoder structure
 * @param timestamp Current timestamp in microseconds
 */
void trigger_decoder_process_tooth(trigger_decoder_t* decoder,
                                  uint32_t timestamp);

/**
 * @brief Check if decoder is synchronized
 *
 * @param decoder Pointer to decoder structure
 * @return true if synchronized, false otherwise
 */
bool trigger_decoder_is_synced(const trigger_decoder_t* decoder);

/**
 * @brief Get current tooth index
 *
 * @param decoder Pointer to decoder structure
 * @return Current tooth index (0 to total_teeth-1), or 0 if not synced
 */
uint8_t trigger_decoder_get_tooth_index(const trigger_decoder_t* decoder);

/**
 * @brief Get current tooth period
 *
 * @param decoder Pointer to decoder structure
 * @return Current tooth period in microseconds
 */
uint32_t trigger_decoder_get_tooth_period(const trigger_decoder_t* decoder);

/**
 * @brief Set synchronization ratio range
 *
 * Configures the ratio range for missing tooth detection.
 * Default values (rusEFI-compatible):
 *   - sync_ratio_from: 1.5 (gap must be at least 1.5× normal tooth)
 *   - sync_ratio_to: 3.0 (gap must be at most 3.0× normal tooth)
 *
 * @param decoder Pointer to decoder structure
 * @param ratio_from Minimum ratio for sync gap
 * @param ratio_to Maximum ratio for sync gap
 */
void trigger_decoder_set_sync_ratio(trigger_decoder_t* decoder,
                                   float ratio_from,
                                   float ratio_to);

/**
 * @brief Set sync point tooth index
 *
 * Configures which tooth index the sync gap represents.
 * For 36-1 wheel, this is typically 0 (gap after tooth 35).
 *
 * @param decoder Pointer to decoder structure
 * @param tooth_index Tooth index at sync point
 */
void trigger_decoder_set_sync_point(trigger_decoder_t* decoder,
                                   uint8_t tooth_index);

/**
 * @brief Reset decoder state
 *
 * Clears synchronization and resets all state.
 * Use this when crank signal is lost or RPM drops to zero.
 *
 * @param decoder Pointer to decoder structure
 */
void trigger_decoder_reset(trigger_decoder_t* decoder);

/**
 * @brief Set callback for synchronization events
 *
 * @param decoder Pointer to decoder structure
 * @param callback Function to call when sync is achieved (can be NULL)
 */
void trigger_decoder_set_sync_callback(trigger_decoder_t* decoder,
                                      void (*callback)(void));

/**
 * @brief Set callback for tooth events
 *
 * @param decoder Pointer to decoder structure
 * @param callback Function to call on each tooth (can be NULL)
 */
void trigger_decoder_set_tooth_callback(trigger_decoder_t* decoder,
                                       void (*callback)(uint8_t tooth));

/**
 * @brief Get decoder statistics
 *
 * Returns diagnostic information about decoder performance.
 *
 * @param decoder Pointer to decoder structure
 * @param sync_count Output: number of successful syncs
 * @param sync_loss_count Output: number of times sync was lost
 * @param tooth_count Output: total teeth processed
 */
void trigger_decoder_get_stats(const trigger_decoder_t* decoder,
                              uint32_t* sync_count,
                              uint32_t* sync_loss_count,
                              uint32_t* tooth_count);

#ifdef __cplusplus
}
#endif

#endif // TRIGGER_DECODER_K64_H
