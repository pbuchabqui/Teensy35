/**
 * @file adc_k64.h
 * @brief ADC driver for Kinetis K64 (Teensy 3.5)
 * @version 1.0.0
 * @date 2026-02-10
 *
 * This file provides ADC (Analog-to-Digital Converter) functionality for
 * reading analog sensors. The MK64FX512 has two 13-bit ADCs.
 *
 * Features:
 * - 2 independent ADCs (ADC0, ADC1)
 * - 13-bit resolution (8192 levels)
 * - 27 analog input channels
 * - Hardware averaging support
 * - Calibration for accuracy
 * - Single-shot and continuous conversion modes
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#ifndef ADC_K64_H
#define ADC_K64_H

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// ADC Instances
//=============================================================================

typedef enum {
    ADC_0 = 0,
    ADC_1 = 1,
} adc_instance_t;

//=============================================================================
// ADC Channels
//=============================================================================

typedef enum {
    // ADC0 channels
    ADC0_DP0  = 0,   // A0 (Pin 14) - Differential pair 0 positive
    ADC0_DP1  = 1,   // A1 (Pin 15) - Differential pair 1 positive
    ADC0_DP2  = 2,   // A4 (Pin 18) - Differential pair 2 positive
    ADC0_DP3  = 3,   // A5 (Pin 19) - Differential pair 3 positive
    ADC0_DM0  = 4,   // A2 (Pin 16) - Differential pair 0 negative
    ADC0_DM1  = 5,   // A3 (Pin 17) - Differential pair 1 negative
    ADC0_SE4  = 6,   // Internal channel
    ADC0_SE5  = 7,   // Internal channel

    // ADC1 channels
    ADC1_DP0  = 0,   // A6 (Pin 20) - Differential pair 0 positive
    ADC1_DP1  = 1,   // A7 (Pin 21) - Differential pair 1 positive
    ADC1_DP3  = 3,   // Internal
    ADC1_DM0  = 4,   // A8 (Pin 22) - Differential pair 0 negative
    ADC1_DM1  = 5,   // A9 (Pin 23) - Differential pair 1 negative
    ADC1_SE16 = 16,  // A10 (Pin 24)

    // Internal channels (both ADCs)
    ADC_CHANNEL_TEMP    = 26,  // Temperature sensor
    ADC_CHANNEL_BANDGAP = 27,  // Bandgap reference (1.0V)
    ADC_CHANNEL_VREFSH  = 29,  // High voltage reference
    ADC_CHANNEL_VREFSL  = 30,  // Low voltage reference
} adc_channel_t;

//=============================================================================
// ADC Resolution
//=============================================================================

typedef enum {
    ADC_RES_8BIT  = 0,  // 8-bit resolution
    ADC_RES_10BIT = 1,  // 10-bit resolution
    ADC_RES_12BIT = 2,  // 12-bit resolution
    ADC_RES_13BIT = 3,  // 13-bit resolution (single-ended)
    ADC_RES_16BIT = 4,  // 16-bit resolution (differential)
} adc_resolution_t;

//=============================================================================
// ADC Averaging
//=============================================================================

typedef enum {
    ADC_AVG_DISABLED = 0,
    ADC_AVG_4_SAMPLES = 1,
    ADC_AVG_8_SAMPLES = 2,
    ADC_AVG_16_SAMPLES = 3,
    ADC_AVG_32_SAMPLES = 4,
} adc_averaging_t;

//=============================================================================
// ADC Configuration
//=============================================================================

typedef struct {
    adc_resolution_t resolution;  // ADC resolution (8, 10, 12, 13 bits)
    adc_averaging_t averaging;    // Hardware averaging
    bool enable_calibration;      // Run calibration on init
} adc_config_t;

//=============================================================================
// ADC Register Definitions
//=============================================================================

#define ADC0_BASE                   0x4003B000
#define ADC1_BASE                   0x400BB000

typedef struct {
    volatile uint32_t SC1[2];     // Status and Control Register 1
    volatile uint32_t CFG1;       // Configuration Register 1
    volatile uint32_t CFG2;       // Configuration Register 2
    volatile uint32_t R[2];       // Data Result Register
    volatile uint32_t CV1;        // Compare Value Register 1
    volatile uint32_t CV2;        // Compare Value Register 2
    volatile uint32_t SC2;        // Status and Control Register 2
    volatile uint32_t SC3;        // Status and Control Register 3
    volatile uint32_t OFS;        // Offset Correction Register
    volatile uint32_t PG;         // Plus-Side Gain Register
    volatile uint32_t MG;         // Minus-Side Gain Register
    volatile uint32_t CLPD;       // Plus-Side General Calibration Value
    volatile uint32_t CLPS;       // Plus-Side General Calibration Value
    volatile uint32_t CLP4;       // Plus-Side General Calibration Value
    volatile uint32_t CLP3;       // Plus-Side General Calibration Value
    volatile uint32_t CLP2;       // Plus-Side General Calibration Value
    volatile uint32_t CLP1;       // Plus-Side General Calibration Value
    volatile uint32_t CLP0;       // Plus-Side General Calibration Value
    volatile uint32_t RESERVED0;
    volatile uint32_t CLMD;       // Minus-Side General Calibration Value
    volatile uint32_t CLMS;       // Minus-Side General Calibration Value
    volatile uint32_t CLM4;       // Minus-Side General Calibration Value
    volatile uint32_t CLM3;       // Minus-Side General Calibration Value
    volatile uint32_t CLM2;       // Minus-Side General Calibration Value
    volatile uint32_t CLM1;       // Minus-Side General Calibration Value
    volatile uint32_t CLM0;       // Minus-Side General Calibration Value
} ADC_Type;

#define ADC0                        ((ADC_Type*)ADC0_BASE)
#define ADC1                        ((ADC_Type*)ADC1_BASE)

// ADC_SC1 bits
#define ADC_SC1_COCO                0x80000000  // Conversion Complete Flag
#define ADC_SC1_AIEN                0x40000000  // Interrupt Enable
#define ADC_SC1_DIFF                0x20000000  // Differential Mode Enable
#define ADC_SC1_ADCH_MASK           0x0000001F  // Input Channel Select
#define ADC_SC1_ADCH(x)             ((x) & 0x1F)

// ADC_CFG1 bits
#define ADC_CFG1_ADLPC              0x00000080  // Low-Power Configuration
#define ADC_CFG1_ADIV_MASK          0x00000060  // Clock Divide Select
#define ADC_CFG1_ADIV(x)            (((x) & 0x03) << 5)
#define ADC_CFG1_ADLSMP             0x00000010  // Long Sample Time
#define ADC_CFG1_MODE_MASK          0x0000000C  // Conversion Mode
#define ADC_CFG1_MODE(x)            (((x) & 0x03) << 2)
#define ADC_CFG1_ADICLK_MASK        0x00000003  // Input Clock Select
#define ADC_CFG1_ADICLK(x)          ((x) & 0x03)

// ADC_CFG2 bits
#define ADC_CFG2_MUXSEL             0x00000010  // ADC Mux Select
#define ADC_CFG2_ADACKEN            0x00000008  // Asynchronous Clock Enable
#define ADC_CFG2_ADHSC              0x00000004  // High-Speed Configuration
#define ADC_CFG2_ADLSTS_MASK        0x00000003  // Long Sample Time Select
#define ADC_CFG2_ADLSTS(x)          ((x) & 0x03)

// ADC_SC2 bits
#define ADC_SC2_ADACT               0x00000080  // Conversion Active
#define ADC_SC2_ADTRG               0x00000040  // Conversion Trigger Select
#define ADC_SC2_ACFE                0x00000020  // Compare Function Enable
#define ADC_SC2_ACFGT               0x00000010  // Compare Function Greater Than
#define ADC_SC2_ACREN               0x00000008  // Compare Function Range Enable
#define ADC_SC2_DMAEN               0x00000004  // DMA Enable
#define ADC_SC2_REFSEL_MASK         0x00000003  // Voltage Reference Selection
#define ADC_SC2_REFSEL(x)           ((x) & 0x03)

// ADC_SC3 bits
#define ADC_SC3_CAL                 0x00000080  // Calibration
#define ADC_SC3_CALF                0x00000040  // Calibration Failed Flag
#define ADC_SC3_ADCO                0x00000008  // Continuous Conversion Enable
#define ADC_SC3_AVGE                0x00000004  // Hardware Average Enable
#define ADC_SC3_AVGS_MASK           0x00000003  // Hardware Average Select
#define ADC_SC3_AVGS(x)             ((x) & 0x03)

//=============================================================================
// Function Prototypes
//=============================================================================

/**
 * @brief Initialize ADC with specified configuration
 *
 * @param instance ADC instance (ADC_0 or ADC_1)
 * @param config Pointer to ADC configuration structure
 * @return true if initialization successful, false otherwise
 */
bool adc_init(adc_instance_t instance, const adc_config_t* config);

/**
 * @brief Read analog value from specified channel (blocking)
 *
 * @param instance ADC instance
 * @param channel ADC channel to read
 * @return Raw ADC value (0-8191 for 13-bit)
 */
uint16_t adc_read(adc_instance_t instance, adc_channel_t channel);

/**
 * @brief Read analog voltage from specified channel
 *
 * Converts raw ADC value to voltage based on reference (3.3V)
 *
 * @param instance ADC instance
 * @param channel ADC channel to read
 * @return Voltage in volts (0.0 - 3.3V)
 */
float adc_read_voltage(adc_instance_t instance, adc_channel_t channel);

/**
 * @brief Check if ADC conversion is complete
 *
 * @param instance ADC instance
 * @return true if conversion complete
 */
bool adc_conversion_complete(adc_instance_t instance);

/**
 * @brief Calibrate ADC for improved accuracy
 *
 * Should be called during initialization for best accuracy.
 * Takes approximately 5ms to complete.
 *
 * @param instance ADC instance
 * @return true if calibration successful
 */
bool adc_calibrate(adc_instance_t instance);

/**
 * @brief Get maximum ADC value for current resolution
 *
 * @param resolution ADC resolution setting
 * @return Maximum value (e.g., 8191 for 13-bit)
 */
uint16_t adc_get_max_value(adc_resolution_t resolution);

#endif // ADC_K64_H
