/**
 * @file input_capture_k64_simple.c
 * @brief Simplified Input Capture driver for Kinetis K64
 * @version 1.0.0
 * @date 2026-02-22
 *
 * Simplified version for compilation testing
 * 
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#include <stdint.h>
#include <stddef.h>
#include "input_capture_k64.h"
#include "clock_k64.h"

//=============================================================================
// Private Variables
//=============================================================================

// Callback storage for each FTM channel
static ic_callback_t ic_callbacks[4][8] = {{NULL}};

//=============================================================================
// Private Helper Functions
//=============================================================================

/**
 * @brief Get FTM register pointer (from pwm_k64.c)
 */
extern FTM_Type* pwm_get_regs(pwm_ftm_t ftm);

//=============================================================================
// Interrupt Handlers
//=============================================================================

void FTM0_IRQHandler(void) {
    FTM_Type* ftm = pwm_get_regs(PWM_FTM0);
    
    // Check all channel interrupt flags
    for (int ch = 0; ch < 8; ch++) {
        if (ftm->CONTROLS[ch].CnSC & FTM_CnSC_CHF) {
            // Clear interrupt flag
            ftm->CONTROLS[ch].CnSC &= ~FTM_CnSC_CHF;
            
            // Call registered callback
            if (ic_callbacks[0][ch] != NULL) {
                uint32_t capture = ftm->CONTROLS[ch].CnV;
                ic_callbacks[0][ch](capture);
            }
        }
    }
}

void FTM1_IRQHandler(void) {
    FTM_Type* ftm = pwm_get_regs(PWM_FTM1);
    
    for (int ch = 0; ch < 8; ch++) {
        if (ftm->CONTROLS[ch].CnSC & FTM_CnSC_CHF) {
            ftm->CONTROLS[ch].CnSC &= ~FTM_CnSC_CHF;
            
            if (ic_callbacks[1][ch] != NULL) {
                uint32_t capture = ftm->CONTROLS[ch].CnV;
                ic_callbacks[1][ch](capture);
            }
        }
    }
}

void FTM2_IRQHandler(void) {
    FTM_Type* ftm = pwm_get_regs(PWM_FTM2);
    
    for (int ch = 0; ch < 8; ch++) {
        if (ftm->CONTROLS[ch].CnSC & FTM_CnSC_CHF) {
            ftm->CONTROLS[ch].CnSC &= ~FTM_CnSC_CHF;
            
            if (ic_callbacks[2][ch] != NULL) {
                uint32_t capture = ftm->CONTROLS[ch].CnV;
                ic_callbacks[2][ch](capture);
            }
        }
    }
}

void FTM3_IRQHandler(void) {
    FTM_Type* ftm = pwm_get_regs(PWM_FTM3);
    
    for (int ch = 0; ch < 8; ch++) {
        if (ftm->CONTROLS[ch].CnSC & FTM_CnSC_CHF) {
            ftm->CONTROLS[ch].CnSC &= ~FTM_CnSC_CHF;
            
            if (ic_callbacks[3][ch] != NULL) {
                uint32_t capture = ftm->CONTROLS[ch].CnV;
                ic_callbacks[3][ch](capture);
            }
        }
    }
}

//=============================================================================
// Public Functions
//=============================================================================

void input_capture_init(void) {
    // Clock is enabled by PWM driver
    
    // Clear all callbacks
    for (int ftm = 0; ftm < 4; ftm++) {
        for (int ch = 0; ch < 8; ch++) {
            ic_callbacks[ftm][ch] = NULL;
        }
    }
}

void input_capture_config(pwm_ftm_t ftm, pwm_channel_t channel, 
                         ic_edge_t edge, uint8_t filter) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return;
    }
    
    // Configure channel for input capture
    ftm_regs->CONTROLS[channel].CnSC = 
        (edge == IC_EDGE_RISING ? 0x04 : 0x02) |  // ELS bits
        (filter ? 0x04 : 0x00);  // CHIE bit
}

void input_capture_set_callback(pwm_ftm_t ftm, pwm_channel_t channel, 
                               ic_callback_t callback) {
    if (ftm < 4 && channel < PWM_CHANNEL_7) {
        ic_callbacks[ftm][channel] = callback;
    }
}

void input_capture_enable(pwm_ftm_t ftm, pwm_channel_t channel) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return;
    }
    
    // Enable channel interrupt
    ftm_regs->CONTROLS[channel].CnSC |= FTM_CnSC_CHIE;
}

void input_capture_disable(pwm_ftm_t ftm, pwm_channel_t channel) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return;
    }
    
    // Disable channel interrupt
    ftm_regs->CONTROLS[channel].CnSC &= ~FTM_CnSC_CHIE;
}

uint32_t input_capture_get_value(pwm_ftm_t ftm, pwm_channel_t channel) {
    FTM_Type* ftm_regs = pwm_get_regs(ftm);
    
    if (ftm_regs == NULL || channel > PWM_CHANNEL_7) {
        return 0;
    }
    
    return ftm_regs->CONTROLS[channel].CnV;
}

// Simplified stub functions for compatibility
void crank_sensor_init(uint16_t teeth_per_rev, uint16_t missing_teeth, sensor_type_t sensor_type) {
    (void)teeth_per_rev;
    (void)missing_teeth;
    (void)sensor_type;
    // TODO: Implement proper crank sensor initialization
}

void cam_sensor_init(uint16_t teeth_per_rev, sensor_type_t sensor_type) {
    (void)teeth_per_rev;
    (void)sensor_type;
    // TODO: Implement proper cam sensor initialization
}
