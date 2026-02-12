/**
 * @file trigger_decoder_k64.c
 * @brief rusEFI-compatible Trigger Decoder Implementation
 *
 * Implements missing tooth detection and synchronization using rusEFI's
 * TriggerDecoderBase algorithm.
 *
 * @version 2.3.0
 * @date 2026-02-11
 */

#include "trigger_decoder_k64.h"
#include <string.h>

// Default synchronization ratios (rusEFI-compatible)
#define DEFAULT_SYNC_RATIO_FROM  1.5f   ///< Min ratio: gap >= 1.5× normal tooth
#define DEFAULT_SYNC_RATIO_TO    3.0f   ///< Max ratio: gap <= 3.0× normal tooth

// Minimum period for valid tooth (µs) - prevents noise at very high RPM
#define MIN_TOOTH_PERIOD_US      100    ///< ~10 teeth at 300,000 RPM

/**
 * @brief Initialize trigger decoder
 */
void trigger_decoder_init(trigger_decoder_t* decoder,
                         uint8_t teeth,
                         uint8_t missing)
{
    if (decoder == NULL) {
        return;
    }

    memset(decoder, 0, sizeof(trigger_decoder_t));

    decoder->total_teeth = teeth;
    decoder->missing_teeth = missing;
    decoder->tooth_count = 0;

    // Set default sync ratios (rusEFI-compatible)
    decoder->sync_ratio_from = DEFAULT_SYNC_RATIO_FROM;
    decoder->sync_ratio_to = DEFAULT_SYNC_RATIO_TO;

    // For 36-1 wheel, sync point is at tooth 0 (after the gap)
    decoder->sync_point_tooth = 0;

    decoder->sync_locked = false;
    decoder->on_sync_callback = NULL;
    decoder->on_tooth_callback = NULL;
}

/**
 * @brief Process a tooth event (rusEFI algorithm)
 *
 * Core algorithm from rusEFI TriggerDecoderBase:
 *
 * 1. Calculate tooth period: delta = current_time - prev_time
 * 2. Calculate ratio: ratio = current_period / previous_period
 * 3. Detect missing tooth gap:
 *    if (ratio >= syncRatioFrom && ratio <= syncRatioTo):
 *        - SYNC FOUND!
 *        - Reset tooth_count to sync_point_tooth
 *        - Set sync_locked = true
 * 4. Increment tooth_count for next tooth
 * 5. Validate tooth count doesn't exceed total_teeth
 */
void trigger_decoder_process_tooth(trigger_decoder_t* decoder,
                                  uint32_t timestamp)
{
    if (decoder == NULL) {
        return;
    }

    decoder->tooth_event_counter++;

    // First tooth ever - just record timestamp
    if (decoder->prev_tooth_time == 0) {
        decoder->prev_tooth_time = timestamp;
        return;
    }

    // Calculate current tooth period
    uint32_t tooth_period = timestamp - decoder->prev_tooth_time;

    // Noise rejection: ignore very short periods
    if (tooth_period < MIN_TOOTH_PERIOD_US) {
        return;
    }

    // Store current period for next iteration
    decoder->current_tooth_period = tooth_period;

    // rusEFI algorithm: detect missing tooth gap
    if (decoder->prev_tooth_period > 0) {
        // Calculate ratio of current period to previous period
        float ratio = (float)tooth_period / (float)decoder->prev_tooth_period;

        // Check if this is the sync gap (missing tooth)
        if (ratio >= decoder->sync_ratio_from && ratio <= decoder->sync_ratio_to) {
            // SYNC FOUND!
            decoder->tooth_count = decoder->sync_point_tooth;
            decoder->last_sync_time = timestamp;

            // Mark as synced if we weren't already
            if (!decoder->sync_locked) {
                decoder->sync_locked = true;
                decoder->sync_count++;

                // Call sync callback if registered
                if (decoder->on_sync_callback != NULL) {
                    decoder->on_sync_callback();
                }
            } else {
                // Already synced - this is periodic sync confirmation
                decoder->sync_count++;
            }
        }
        // Check if ratio is impossibly small (might indicate missed teeth)
        else if (decoder->sync_locked && ratio < 0.5f) {
            // Tooth period too short - possible sync loss
            // Don't immediately lose sync, but track the anomaly
            // rusEFI uses tolerance here to handle RPM acceleration
        }
    }

    // Validate tooth count (lose sync if exceeded)
    if (decoder->sync_locked) {
        if (decoder->tooth_count >= decoder->total_teeth) {
            // Lost synchronization!
            decoder->sync_locked = false;
            decoder->sync_loss_count++;
            decoder->tooth_count = 0;
        } else {
            // Call tooth callback if registered and synced
            if (decoder->on_tooth_callback != NULL) {
                decoder->on_tooth_callback(decoder->tooth_count);
            }

            // Increment tooth counter for next tooth
            decoder->tooth_count++;
        }
    }

    // Update history for next iteration
    decoder->prev_tooth_period = tooth_period;
    decoder->prev_tooth_time = timestamp;
}

/**
 * @brief Check if decoder is synchronized
 */
bool trigger_decoder_is_synced(const trigger_decoder_t* decoder)
{
    if (decoder == NULL) {
        return false;
    }
    return decoder->sync_locked;
}

/**
 * @brief Get current tooth index
 */
uint8_t trigger_decoder_get_tooth_index(const trigger_decoder_t* decoder)
{
    if (decoder == NULL || !decoder->sync_locked) {
        return 0;
    }
    return decoder->tooth_count;
}

/**
 * @brief Get current tooth period
 */
uint32_t trigger_decoder_get_tooth_period(const trigger_decoder_t* decoder)
{
    if (decoder == NULL) {
        return 0;
    }
    return decoder->current_tooth_period;
}

/**
 * @brief Set synchronization ratio range
 */
void trigger_decoder_set_sync_ratio(trigger_decoder_t* decoder,
                                   float ratio_from,
                                   float ratio_to)
{
    if (decoder == NULL) {
        return;
    }

    decoder->sync_ratio_from = ratio_from;
    decoder->sync_ratio_to = ratio_to;
}

/**
 * @brief Set sync point tooth index
 */
void trigger_decoder_set_sync_point(trigger_decoder_t* decoder,
                                   uint8_t tooth_index)
{
    if (decoder == NULL) {
        return;
    }

    decoder->sync_point_tooth = tooth_index;
}

/**
 * @brief Reset decoder state
 */
void trigger_decoder_reset(trigger_decoder_t* decoder)
{
    if (decoder == NULL) {
        return;
    }

    decoder->sync_locked = false;
    decoder->tooth_count = 0;
    decoder->prev_tooth_time = 0;
    decoder->prev_tooth_period = 0;
    decoder->current_tooth_period = 0;
}

/**
 * @brief Set callback for synchronization events
 */
void trigger_decoder_set_sync_callback(trigger_decoder_t* decoder,
                                      void (*callback)(void))
{
    if (decoder == NULL) {
        return;
    }

    decoder->on_sync_callback = callback;
}

/**
 * @brief Set callback for tooth events
 */
void trigger_decoder_set_tooth_callback(trigger_decoder_t* decoder,
                                       void (*callback)(uint8_t tooth))
{
    if (decoder == NULL) {
        return;
    }

    decoder->on_tooth_callback = callback;
}

/**
 * @brief Get decoder statistics
 */
void trigger_decoder_get_stats(const trigger_decoder_t* decoder,
                              uint32_t* sync_count,
                              uint32_t* sync_loss_count,
                              uint32_t* tooth_count)
{
    if (decoder == NULL) {
        return;
    }

    if (sync_count != NULL) {
        *sync_count = decoder->sync_count;
    }

    if (sync_loss_count != NULL) {
        *sync_loss_count = decoder->sync_loss_count;
    }

    if (tooth_count != NULL) {
        *tooth_count = decoder->tooth_event_counter;
    }
}
