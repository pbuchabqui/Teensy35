/**
 * @file clock_k64.c
 * @brief Clock configuration implementation for Kinetis K64
 * @version 1.0.0
 * @date 2026-02-10
 *
 * This file implements the clock initialization for 120MHz operation
 * using the external 16 MHz crystal and PLL.
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#include "clock_k64.h"

//=============================================================================
// Private Functions
//=============================================================================

/**
 * @brief Transition MCG from FEI mode to FBE mode
 *
 * FEI (FLL Engaged Internal) -> FBE (FLL Bypassed External)
 * This switches from the internal oscillator to the external crystal.
 */
static void mcg_fei_to_fbe(void) {
    // Enable external oscillator (OSC0)
    // Select external reference clock for MCG
    MCG->C2 = MCG_C2_RANGE0_VERY_HIGH |  // 8-32 MHz crystal range
              MCG_C2_HGO0 |               // High gain oscillator
              MCG_C2_EREFS0;              // External reference (crystal)

    // Wait for oscillator to initialize
    while (!(MCG->S & MCG_S_OSCINIT0)) {
        // Wait for crystal oscillator to stabilize
    }

    // Switch to external reference clock
    MCG->C1 = MCG_C1_CLKS_EXTERNAL |     // Select external reference
              MCG_C1_FRDIV(4);            // Divide 16MHz by 512 = 31.25kHz

    // Wait for clock source to switch
    while ((MCG->S & MCG_S_IREFST) != 0) {
        // Wait for external reference
    }

    while ((MCG->S & MCG_S_CLKST_MASK) != MCG_S_CLKST_EXTERNAL) {
        // Wait for external clock selected
    }
}

/**
 * @brief Transition MCG from FBE mode to PBE mode
 *
 * FBE (FLL Bypassed External) -> PBE (PLL Bypassed External)
 * This enables the PLL but doesn't switch to it yet.
 */
static void mcg_fbe_to_pbe(void) {
    // Configure PLL
    // PRDIV = 7 (divide by 8): 16 MHz / 8 = 2 MHz
    MCG->C5 = MCG_C5_PRDIV0(7);

    // VDIV = 24 (multiply by 30): 2 MHz * 60 = 120 MHz
    MCG->C6 = MCG_C6_VDIV0(36) |         // VCO = 2 MHz * 60 = 120 MHz
              MCG_C6_PLLS;                // Select PLL

    // Wait for PLL to lock
    while (!(MCG->S & MCG_S_PLLST)) {
        // Wait for PLL selected
    }

    while (!(MCG->S & MCG_S_LOCK0)) {
        // Wait for PLL locked
    }
}

/**
 * @brief Transition MCG from PBE mode to PEE mode
 *
 * PBE (PLL Bypassed External) -> PEE (PLL Engaged External)
 * This switches the system clock to use the PLL output.
 */
static void mcg_pbe_to_pee(void) {
    // Switch to PLL as clock source
    MCG->C1 &= ~MCG_C1_CLKS_MASK;
    MCG->C1 |= MCG_C1_CLKS_FLL_PLL;

    // Wait for PLL clock to be selected
    while ((MCG->S & MCG_S_CLKST_MASK) != MCG_S_CLKST_PLL) {
        // Wait for PLL output selected
    }
}

//=============================================================================
// Public Functions
//=============================================================================

void clock_init(void) {
    // Enable oscillator in SIM
    OSC->CR = OSC_CR_ERCLKEN |           // Enable external reference clock
              OSC_CR_EREFSTEN;           // Enable in stop mode

    // Configure clock dividers BEFORE switching to high-speed clock
    // This ensures all peripherals stay within their maximum frequencies
    SIM->CLKDIV1 = SIM_CLKDIV1_OUTDIV1(0) |  // Core:    /1 = 120 MHz
                   SIM_CLKDIV1_OUTDIV2(1) |  // Bus:     /2 = 60 MHz
                   SIM_CLKDIV1_OUTDIV3(2) |  // FlexBus: /3 = 40 MHz
                   SIM_CLKDIV1_OUTDIV4(4);   // Flash:   /5 = 24 MHz

    // Transition through MCG modes to reach PEE (PLL Engaged External)
    mcg_fei_to_fbe();  // Switch to external crystal
    mcg_fbe_to_pbe();  // Enable PLL
    mcg_pbe_to_pee();  // Switch to PLL output

    // System is now running at 120 MHz from PLL
}

uint32_t clock_get_core_freq(void) {
    return CPU_CORE_CLK_HZ;
}

uint32_t clock_get_bus_freq(void) {
    return CPU_BUS_CLK_HZ;
}
