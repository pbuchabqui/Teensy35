/**
 * @file adc_k64.c
 * @brief ADC driver implementation for Kinetis K64
 * @version 1.0.0
 * @date 2026-02-10
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#include "adc_k64.h"
#include "clock_k64.h"

//=============================================================================
// SIM Register Access (for clock gating)
//=============================================================================

#define SIM_SCGC6_ADC0              0x08000000
#define SIM_SCGC3_ADC1              0x08000000

//=============================================================================
// Private Constants
//=============================================================================

#define ADC_VREF                    3.3f  // Reference voltage (3.3V)
#define ADC_CALIBRATION_TIMEOUT     1000  // Calibration timeout in iterations

//=============================================================================
// Private Helper Functions
//=============================================================================

/**
 * @brief Get ADC register pointer for a given instance
 *
 * @param instance ADC instance
 * @return Pointer to ADC registers
 */
static ADC_Type* adc_get_regs(adc_instance_t instance) {
    switch (instance) {
        case ADC_0: return ADC0;
        case ADC_1: return ADC1;
        default: return NULL;
    }
}

/**
 * @brief Enable ADC clock
 *
 * @param instance ADC instance
 */
static void adc_enable_clock(adc_instance_t instance) {
    switch (instance) {
        case ADC_0:
            SIM->SCGC6 |= SIM_SCGC6_ADC0;
            break;
        case ADC_1:
            SIM->SCGC3 |= SIM_SCGC3_ADC1;
            break;
    }
}

//=============================================================================
// Public Functions
//=============================================================================

bool adc_init(adc_instance_t instance, const adc_config_t* config) {
    ADC_Type* adc = adc_get_regs(instance);
    if (adc == NULL || config == NULL) {
        return false;
    }

    // Enable ADC clock
    adc_enable_clock(instance);

    // Configure ADC_CFG1
    // - Bus clock (60 MHz)
    // - Divide by 4 = 15 MHz ADC clock
    // - Long sample time for better accuracy
    uint32_t cfg1 = ADC_CFG1_ADICLK(0) |     // Bus clock
                    ADC_CFG1_ADIV(2) |        // Divide by 4
                    ADC_CFG1_ADLSMP;          // Long sample time

    // Set resolution
    switch (config->resolution) {
        case ADC_RES_8BIT:
            cfg1 |= ADC_CFG1_MODE(0);
            break;
        case ADC_RES_10BIT:
            cfg1 |= ADC_CFG1_MODE(2);
            break;
        case ADC_RES_12BIT:
            cfg1 |= ADC_CFG1_MODE(1);
            break;
        case ADC_RES_13BIT:
        case ADC_RES_16BIT:
            cfg1 |= ADC_CFG1_MODE(3);
            break;
    }

    adc->CFG1 = cfg1;

    // Configure ADC_CFG2
    // - Default MUX (ADxxb channels)
    // - High-speed conversion
    adc->CFG2 = ADC_CFG2_ADHSC;

    // Configure ADC_SC2
    // - Software trigger
    // - Default voltage reference (VREFH/VREFL)
    adc->SC2 = ADC_SC2_REFSEL(0);

    // Configure ADC_SC3
    uint32_t sc3 = 0;

    // Enable hardware averaging if requested
    if (config->averaging != ADC_AVG_DISABLED) {
        sc3 |= ADC_SC3_AVGE | ADC_SC3_AVGS(config->averaging - 1);
    }

    adc->SC3 = sc3;

    // Perform calibration if requested
    if (config->enable_calibration) {
        if (!adc_calibrate(instance)) {
            return false;
        }
    }

    return true;
}

bool adc_calibrate(adc_instance_t instance) {
    ADC_Type* adc = adc_get_regs(instance);
    if (adc == NULL) {
        return false;
    }

    // Start calibration
    adc->SC3 |= ADC_SC3_CAL;

    // Wait for calibration to complete
    uint32_t timeout = ADC_CALIBRATION_TIMEOUT;
    while ((adc->SC3 & ADC_SC3_CAL) && (timeout > 0)) {
        timeout--;
    }

    // Check if calibration failed
    if (adc->SC3 & ADC_SC3_CALF) {
        return false;  // Calibration failed
    }

    if (timeout == 0) {
        return false;  // Calibration timeout
    }

    // Calculate plus-side calibration value
    uint16_t cal_var = adc->CLP0 + adc->CLP1 + adc->CLP2 +
                       adc->CLP3 + adc->CLP4 + adc->CLPS;
    cal_var = cal_var / 2;
    cal_var |= 0x8000;  // Set MSB
    adc->PG = cal_var;

    // Calculate minus-side calibration value
    cal_var = adc->CLM0 + adc->CLM1 + adc->CLM2 +
              adc->CLM3 + adc->CLM4 + adc->CLMS;
    cal_var = cal_var / 2;
    cal_var |= 0x8000;  // Set MSB
    adc->MG = cal_var;

    return true;
}

uint16_t adc_read(adc_instance_t instance, adc_channel_t channel) {
    ADC_Type* adc = adc_get_regs(instance);
    if (adc == NULL) {
        return 0;
    }

    // Start conversion on selected channel
    adc->SC1[0] = ADC_SC1_ADCH(channel);

    // Wait for conversion to complete
    while (!(adc->SC1[0] & ADC_SC1_COCO)) {
        // Wait
    }

    // Read and return result
    return (uint16_t)(adc->R[0] & 0xFFFF);
}

float adc_read_voltage(adc_instance_t instance, adc_channel_t channel) {
    uint16_t raw_value = adc_read(instance, channel);

    // Get ADC configuration to determine resolution
    ADC_Type* adc = adc_get_regs(instance);
    if (adc == NULL) {
        return 0.0f;
    }

    // Determine maximum value based on resolution
    uint32_t mode = (adc->CFG1 & ADC_CFG1_MODE_MASK) >> 2;
    uint16_t max_value;

    switch (mode) {
        case 0: max_value = 255;    break;  // 8-bit
        case 1: max_value = 4095;   break;  // 12-bit
        case 2: max_value = 1023;   break;  // 10-bit
        case 3: max_value = 8191;   break;  // 13-bit (or 16-bit differential)
        default: max_value = 8191;  break;
    }

    // Convert to voltage
    return ((float)raw_value / (float)max_value) * ADC_VREF;
}

bool adc_conversion_complete(adc_instance_t instance) {
    ADC_Type* adc = adc_get_regs(instance);
    if (adc == NULL) {
        return false;
    }

    return (adc->SC1[0] & ADC_SC1_COCO) != 0;
}

uint16_t adc_get_max_value(adc_resolution_t resolution) {
    switch (resolution) {
        case ADC_RES_8BIT:  return 255;
        case ADC_RES_10BIT: return 1023;
        case ADC_RES_12BIT: return 4095;
        case ADC_RES_13BIT: return 8191;
        case ADC_RES_16BIT: return 65535;
        default: return 8191;
    }
}
