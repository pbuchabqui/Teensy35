/**
 * @file clock_k64.h
 * @brief Clock configuration for Kinetis K64 (Teensy 3.5)
 * @version 1.0.0
 * @date 2026-02-10
 *
 * This file provides clock initialization for the MK64FX512 to run at 120MHz.
 * It configures the PLL, dividers, and selects appropriate clock sources.
 *
 * Clock Tree:
 * - External Crystal: 16 MHz
 * - PLL Output: 120 MHz
 * - Core Clock: 120 MHz
 * - Bus Clock: 60 MHz
 * - FlexBus Clock: 40 MHz
 * - Flash Clock: 24 MHz
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#ifndef CLOCK_K64_H
#define CLOCK_K64_H

#include <stdint.h>

//=============================================================================
// Clock Configuration Constants
//=============================================================================

#define CPU_XTAL_CLK_HZ              16000000u  // 16 MHz external crystal
#define CPU_CORE_CLK_HZ              120000000u // 120 MHz core clock
#define CPU_BUS_CLK_HZ               60000000u  // 60 MHz bus clock
#define CPU_FLEXBUS_CLK_HZ           40000000u  // 40 MHz FlexBus clock
#define CPU_FLASH_CLK_HZ             24000000u  // 24 MHz flash clock

//=============================================================================
// MCG (Multipurpose Clock Generator) Register Definitions
//=============================================================================

#define MCG_BASE                     0x40064000
#define MCG                          ((MCG_Type*)MCG_BASE)

typedef struct {
    volatile uint8_t  C1;         // MCG Control 1 Register
    volatile uint8_t  C2;         // MCG Control 2 Register
    volatile uint8_t  C3;         // MCG Control 3 Register
    volatile uint8_t  C4;         // MCG Control 4 Register
    volatile uint8_t  C5;         // MCG Control 5 Register
    volatile uint8_t  C6;         // MCG Control 6 Register
    volatile uint8_t  S;          // MCG Status Register
    volatile uint8_t  RESERVED0;
    volatile uint8_t  SC;         // MCG Status and Control Register
    volatile uint8_t  RESERVED1;
    volatile uint8_t  ATCVH;      // MCG Auto Trim Compare Value High
    volatile uint8_t  ATCVL;      // MCG Auto Trim Compare Value Low
    volatile uint8_t  C7;         // MCG Control 7 Register
    volatile uint8_t  C8;         // MCG Control 8 Register
} MCG_Type;

// MCG_C1 bits
#define MCG_C1_CLKS_MASK            0xC0
#define MCG_C1_CLKS_FLL_PLL         0x00
#define MCG_C1_CLKS_INTERNAL        0x40
#define MCG_C1_CLKS_EXTERNAL        0x80
#define MCG_C1_FRDIV_MASK           0x38
#define MCG_C1_FRDIV(x)             (((x) & 0x07) << 3)
#define MCG_C1_IREFS               0x04
#define MCG_C1_IRCLKEN             0x02
#define MCG_C1_IREFSTEN            0x01

// MCG_C2 bits
#define MCG_C2_LOCRE0              0x80
#define MCG_C2_RANGE0_MASK         0x30
#define MCG_C2_RANGE0_LOW          0x00
#define MCG_C2_RANGE0_HIGH         0x10
#define MCG_C2_RANGE0_VERY_HIGH    0x20
#define MCG_C2_HGO0                0x08
#define MCG_C2_EREFS0              0x04
#define MCG_C2_LP                  0x02
#define MCG_C2_IRCS                0x01

// MCG_C5 bits
#define MCG_C5_PLLCLKEN0           0x40
#define MCG_C5_PLLSTEN0            0x20
#define MCG_C5_PRDIV0_MASK         0x1F
#define MCG_C5_PRDIV0(x)           ((x) & 0x1F)

// MCG_C6 bits
#define MCG_C6_LOLIE0              0x80
#define MCG_C6_PLLS                0x40
#define MCG_C6_CME0                0x20
#define MCG_C6_VDIV0_MASK          0x1F
#define MCG_C6_VDIV0(x)            ((x) & 0x1F)

// MCG_S bits
#define MCG_S_LOLS0                0x80
#define MCG_S_LOCK0                0x40
#define MCG_S_PLLST                0x20
#define MCG_S_IREFST               0x10
#define MCG_S_CLKST_MASK           0x0C
#define MCG_S_CLKST_FLL            0x00
#define MCG_S_CLKST_INTERNAL       0x04
#define MCG_S_CLKST_EXTERNAL       0x08
#define MCG_S_CLKST_PLL            0x0C
#define MCG_S_OSCINIT0             0x02
#define MCG_S_IRCST                0x01

//=============================================================================
// SIM (System Integration Module) Register Definitions
//=============================================================================

#define SIM_BASE                     0x40047000
#define SIM                          ((SIM_Type*)SIM_BASE)

typedef struct {
    volatile uint32_t SOPT1;      // System Options Register 1
    volatile uint32_t SOPT1CFG;   // SOPT1 Configuration Register
    volatile uint32_t RESERVED0[1023];
    volatile uint32_t SOPT2;      // System Options Register 2
    volatile uint32_t RESERVED1;
    volatile uint32_t SOPT4;      // System Options Register 4
    volatile uint32_t SOPT5;      // System Options Register 5
    volatile uint32_t RESERVED2;
    volatile uint32_t SOPT7;      // System Options Register 7
    volatile uint32_t RESERVED3[2];
    volatile uint32_t SDID;       // System Device Identification
    volatile uint32_t SCGC1;      // System Clock Gating Control 1
    volatile uint32_t SCGC2;      // System Clock Gating Control 2
    volatile uint32_t SCGC3;      // System Clock Gating Control 3
    volatile uint32_t SCGC4;      // System Clock Gating Control 4
    volatile uint32_t SCGC5;      // System Clock Gating Control 5
    volatile uint32_t SCGC6;      // System Clock Gating Control 6
    volatile uint32_t SCGC7;      // System Clock Gating Control 7
    volatile uint32_t CLKDIV1;    // System Clock Divider Register 1
    volatile uint32_t CLKDIV2;    // System Clock Divider Register 2
    volatile uint32_t FCFG1;      // Flash Configuration Register 1
    volatile uint32_t FCFG2;      // Flash Configuration Register 2
    volatile uint32_t UIDH;       // Unique Identification Register High
    volatile uint32_t UIDMH;      // Unique Identification Register Mid-High
    volatile uint32_t UIDML;      // Unique Identification Register Mid-Low
    volatile uint32_t UIDL;       // Unique Identification Register Low
} SIM_Type;

// SIM_CLKDIV1 bits (Clock dividers)
#define SIM_CLKDIV1_OUTDIV1_MASK    0xF0000000
#define SIM_CLKDIV1_OUTDIV1(x)      (((x) & 0x0F) << 28)
#define SIM_CLKDIV1_OUTDIV2_MASK    0x0F000000
#define SIM_CLKDIV1_OUTDIV2(x)      (((x) & 0x0F) << 24)
#define SIM_CLKDIV1_OUTDIV3_MASK    0x00F00000
#define SIM_CLKDIV1_OUTDIV3(x)      (((x) & 0x0F) << 20)
#define SIM_CLKDIV1_OUTDIV4_MASK    0x000F0000
#define SIM_CLKDIV1_OUTDIV4(x)      (((x) & 0x0F) << 16)

// SIM_SCGC5 bits (Port clocks)
#define SIM_SCGC5_PORTA             0x00000200
#define SIM_SCGC5_PORTB             0x00000400
#define SIM_SCGC5_PORTC             0x00000800
#define SIM_SCGC5_PORTD             0x00001000
#define SIM_SCGC5_PORTE             0x00002000

//=============================================================================
// OSC (Oscillator) Register Definitions
//=============================================================================

#define OSC_BASE                     0x40065000
#define OSC                          ((OSC_Type*)OSC_BASE)

typedef struct {
    volatile uint8_t  CR;         // OSC Control Register
    volatile uint8_t  RESERVED0[1];
    volatile uint8_t  DIV;        // OSC Clock Divider Register
} OSC_Type;

// OSC_CR bits
#define OSC_CR_ERCLKEN             0x80
#define OSC_CR_EREFSTEN            0x20
#define OSC_CR_SC2P                0x08
#define OSC_CR_SC4P                0x04
#define OSC_CR_SC8P                0x02
#define OSC_CR_SC16P               0x01

//=============================================================================
// Function Prototypes
//=============================================================================

/**
 * @brief Initialize system clocks to 120 MHz
 *
 * Configures the MCG to use the external 16 MHz crystal with PLL
 * to generate a 120 MHz core clock.
 *
 * Clock configuration:
 * - Core:    120 MHz (OUTDIV1 = /1)
 * - Bus:     60 MHz  (OUTDIV2 = /2)
 * - FlexBus: 40 MHz  (OUTDIV3 = /3)
 * - Flash:   24 MHz  (OUTDIV4 = /5)
 */
void clock_init(void);

/**
 * @brief Get current core clock frequency in Hz
 * @return Core clock frequency in Hz
 */
uint32_t clock_get_core_freq(void);

/**
 * @brief Get current bus clock frequency in Hz
 * @return Bus clock frequency in Hz
 */
uint32_t clock_get_bus_freq(void);

#endif // CLOCK_K64_H
