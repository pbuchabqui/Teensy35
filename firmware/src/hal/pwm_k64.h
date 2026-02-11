/**
 * @file pwm_k64.h
 * @brief PWM driver for Kinetis K64 (Teensy 3.5)
 * @version 1.0.0
 * @date 2026-02-10
 *
 * This file provides PWM (Pulse Width Modulation) functionality using
 * FlexTimer (FTM) modules for controlling injectors, ignition coils, and
 * other actuators.
 *
 * Features:
 * - 4 FlexTimer modules (FTM0-FTM3)
 * - Edge-aligned and center-aligned PWM
 * - Complementary PWM with dead-time insertion
 * - Frequency range: 1 Hz - 60 MHz
 * - 16-bit resolution
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#ifndef PWM_K64_H
#define PWM_K64_H

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// PWM Module (FlexTimer) Selection
//=============================================================================

typedef enum {
    PWM_FTM0 = 0,
    PWM_FTM1 = 1,
    PWM_FTM2 = 2,
    PWM_FTM3 = 3,
} pwm_ftm_t;

//=============================================================================
// PWM Channel Selection
//=============================================================================

typedef enum {
    PWM_CHANNEL_0 = 0,
    PWM_CHANNEL_1 = 1,
    PWM_CHANNEL_2 = 2,
    PWM_CHANNEL_3 = 3,
    PWM_CHANNEL_4 = 4,
    PWM_CHANNEL_5 = 5,
    PWM_CHANNEL_6 = 6,
    PWM_CHANNEL_7 = 7,
} pwm_channel_t;

//=============================================================================
// PWM Alignment Mode
//=============================================================================

typedef enum {
    PWM_EDGE_ALIGNED = 0,    // Edge-aligned PWM (default)
    PWM_CENTER_ALIGNED = 1,  // Center-aligned PWM
} pwm_alignment_t;

//=============================================================================
// PWM Polarity
//=============================================================================

typedef enum {
    PWM_POLARITY_HIGH = 0,   // High-true pulses (default)
    PWM_POLARITY_LOW = 1,    // Low-true pulses (inverted)
} pwm_polarity_t;

//=============================================================================
// PWM Configuration
//=============================================================================

typedef struct {
    uint32_t frequency_hz;       // PWM frequency in Hz (1 - 1000000)
    pwm_alignment_t alignment;   // Edge or center aligned
    bool enable_prescaler_auto;  // Automatically calculate prescaler
} pwm_config_t;

//=============================================================================
// PWM Channel Configuration
//=============================================================================

typedef struct {
    pwm_polarity_t polarity;     // Output polarity
    uint16_t duty_cycle_percent; // Initial duty cycle (0-100%)
    bool enable_output;          // Enable output immediately
} pwm_channel_config_t;

//=============================================================================
// FlexTimer Register Definitions
//=============================================================================

#define FTM0_BASE                   0x40038000
#define FTM1_BASE                   0x40039000
#define FTM2_BASE                   0x4003A000
#define FTM3_BASE                   0x400B9000

typedef struct {
    volatile uint32_t SC;         // Status and Control
    volatile uint32_t CNT;        // Counter
    volatile uint32_t MOD;        // Modulo
    struct {
        volatile uint32_t CnSC;   // Channel Status and Control
        volatile uint32_t CnV;    // Channel Value
    } CONTROLS[8];                // 8 channels
    volatile uint32_t CNTIN;      // Counter Initial Value
    volatile uint32_t STATUS;     // Capture and Compare Status
    volatile uint32_t MODE;       // Features Mode Selection
    volatile uint32_t SYNC;       // Synchronization
    volatile uint32_t OUTINIT;    // Initial State for Channels Output
    volatile uint32_t OUTMASK;    // Output Mask
    volatile uint32_t COMBINE;    // Function for Linked Channels
    volatile uint32_t DEADTIME;   // Deadtime Insertion Control
    volatile uint32_t EXTTRIG;    // FTM External Trigger
    volatile uint32_t POL;        // Channels Polarity
    volatile uint32_t FMS;        // Fault Mode Status
    volatile uint32_t FILTER;     // Input Capture Filter Control
    volatile uint32_t FLTCTRL;    // Fault Control
    volatile uint32_t QDCTRL;     // Quadrature Decoder Control and Status
    volatile uint32_t CONF;       // Configuration
    volatile uint32_t FLTPOL;     // FTM Fault Input Polarity
    volatile uint32_t SYNCONF;    // Synchronization Configuration
    volatile uint32_t INVCTRL;    // FTM Inverting Control
    volatile uint32_t SWOCTRL;    // FTM Software Output Control
    volatile uint32_t PWMLOAD;    // FTM PWM Load
} FTM_Type;

#define FTM0                        ((FTM_Type*)FTM0_BASE)
#define FTM1                        ((FTM_Type*)FTM1_BASE)
#define FTM2                        ((FTM_Type*)FTM2_BASE)
#define FTM3                        ((FTM_Type*)FTM3_BASE)

// FTM_SC bits
#define FTM_SC_TOF                  0x00000080  // Timer Overflow Flag
#define FTM_SC_TOIE                 0x00000040  // Timer Overflow Interrupt Enable
#define FTM_SC_CPWMS                0x00000020  // Center-Aligned PWM Select
#define FTM_SC_CLKS_MASK            0x00000018  // Clock Source Selection
#define FTM_SC_CLKS(x)              (((x) & 0x03) << 3)
#define FTM_SC_PS_MASK              0x00000007  // Prescale Factor Selection
#define FTM_SC_PS(x)                ((x) & 0x07)

// FTM_CnSC bits
#define FTM_CnSC_CHF                0x00000080  // Channel Flag
#define FTM_CnSC_CHIE               0x00000040  // Channel Interrupt Enable
#define FTM_CnSC_MSB                0x00000020  // Channel Mode Select B
#define FTM_CnSC_MSA                0x00000010  // Channel Mode Select A
#define FTM_CnSC_ELSB               0x00000008  // Edge or Level Select B
#define FTM_CnSC_ELSA               0x00000004  // Edge or Level Select A
#define FTM_CnSC_DMA                0x00000001  // DMA Enable

// PWM mode: MSB:MSA = 1:0 (edge-aligned) or 1:1 (center-aligned)
// High-true: ELSB:ELSA = 1:0
// Low-true:  ELSB:ELSA = 0:1
#define FTM_CnSC_PWM_HIGH           (FTM_CnSC_MSB | FTM_CnSC_ELSB)
#define FTM_CnSC_PWM_LOW            (FTM_CnSC_MSB | FTM_CnSC_ELSA)

//=============================================================================
// Function Prototypes
//=============================================================================

/**
 * @brief Initialize PWM module (FlexTimer)
 *
 * @param ftm FlexTimer module (FTM0-FTM3)
 * @param config Pointer to PWM configuration structure
 * @return true if initialization successful
 */
bool pwm_init(pwm_ftm_t ftm, const pwm_config_t* config);

/**
 * @brief Configure PWM channel
 *
 * @param ftm FlexTimer module
 * @param channel PWM channel (0-7)
 * @param config Pointer to channel configuration structure
 * @return true if configuration successful
 */
bool pwm_channel_init(pwm_ftm_t ftm, pwm_channel_t channel,
                      const pwm_channel_config_t* config);

/**
 * @brief Set PWM duty cycle (0-100%)
 *
 * @param ftm FlexTimer module
 * @param channel PWM channel
 * @param duty_percent Duty cycle percentage (0-100)
 */
void pwm_set_duty_cycle(pwm_ftm_t ftm, pwm_channel_t channel,
                        uint16_t duty_percent);

/**
 * @brief Set PWM duty cycle (absolute value)
 *
 * @param ftm FlexTimer module
 * @param channel PWM channel
 * @param duty_value Duty cycle value (0-MOD)
 */
void pwm_set_duty_value(pwm_ftm_t ftm, pwm_channel_t channel,
                        uint16_t duty_value);

/**
 * @brief Set PWM frequency
 *
 * Changing frequency will affect all channels on this FTM module
 *
 * @param ftm FlexTimer module
 * @param frequency_hz Frequency in Hz
 * @return true if frequency set successfully
 */
bool pwm_set_frequency(pwm_ftm_t ftm, uint32_t frequency_hz);

/**
 * @brief Enable PWM channel output
 *
 * @param ftm FlexTimer module
 * @param channel PWM channel
 */
void pwm_enable(pwm_ftm_t ftm, pwm_channel_t channel);

/**
 * @brief Disable PWM channel output
 *
 * @param ftm FlexTimer module
 * @param channel PWM channel
 */
void pwm_disable(pwm_ftm_t ftm, pwm_channel_t channel);

/**
 * @brief Get current PWM modulo value
 *
 * @param ftm FlexTimer module
 * @return Modulo value (period)
 */
uint16_t pwm_get_modulo(pwm_ftm_t ftm);

/**
 * @brief Set PWM pulse width in microseconds
 *
 * Useful for injector control where pulse width is specified in ms/µs
 *
 * @param ftm FlexTimer module
 * @param channel PWM channel
 * @param pulse_us Pulse width in microseconds
 */
void pwm_set_pulse_width_us(pwm_ftm_t ftm, pwm_channel_t channel,
                            uint32_t pulse_us);

#endif // PWM_K64_H
