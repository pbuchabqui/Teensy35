/**
 * @file startup_mk64fx512.c
 * @brief Startup code for Teensy 3.5 (MK64FX512VMD12)
 * @version 1.0.0
 * @date 2026-02-10
 *
 * This file contains the startup code for the Kinetis MK64FX512 microcontroller.
 * It defines the interrupt vector table and implements the Reset_Handler that
 * initializes the .data and .bss sections before calling main().
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#include <stdint.h>
#include <string.h>

//=============================================================================
// External symbols from linker script
//=============================================================================

extern uint32_t _sstack;     // Initial stack pointer
extern uint32_t _sidata;     // Start of .data in Flash
extern uint32_t _sdata;      // Start of .data in RAM
extern uint32_t _edata;      // End of .data in RAM
extern uint32_t _sbss;       // Start of .bss in RAM
extern uint32_t _ebss;       // End of .bss in RAM

//=============================================================================
// Function prototypes
//=============================================================================

extern int main(void);
void Reset_Handler(void);
void Default_Handler(void);

// Cortex-M4 core handlers
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

// Kinetis MK64FX512 peripheral handlers
void DMA0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA7_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA8_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA9_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA10_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA11_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA12_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA13_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA14_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA15_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_Error_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void MCM_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FTFE_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void Read_Collision_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void LVD_LVW_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void LLW_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void Watchdog_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RNG_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2S0_Tx_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2S0_Rx_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART0_RX_TX_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART0_ERR_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART1_RX_TX_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART1_ERR_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART2_RX_TX_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART2_ERR_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART3_RX_TX_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART3_ERR_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ADC0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CMP0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CMP1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FTM0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FTM1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FTM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CMT_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_Seconds_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PIT0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PIT1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PIT2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PIT3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PDB0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USB0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USBDCD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void Reserved71_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DAC0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void MCG_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void LPTimer_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PORTA_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PORTB_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PORTC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PORTD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PORTE_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWI_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART4_RX_TX_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART4_ERR_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART5_RX_TX_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART5_ERR_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CMP2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FTM3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DAC1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ADC1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN0_ORed_Message_buffer_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN0_Bus_Off_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN0_Error_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN0_Tx_Warning_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN0_Rx_Warning_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CAN0_Wake_Up_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SDHC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ENET_1588_Timer_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ENET_Transmit_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ENET_Receive_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ENET_Error_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

//=============================================================================
// Interrupt Vector Table
//=============================================================================

__attribute__((section(".vectors"), used))
void (*const g_pfnVectors[])(void) = {
    // Core Level - CM4
    (void (*)(void))&_sstack,           // 0:  Initial Stack Pointer
    Reset_Handler,                       // 1:  Reset Handler
    NMI_Handler,                         // 2:  NMI Handler
    HardFault_Handler,                   // 3:  Hard Fault Handler
    MemManage_Handler,                   // 4:  MPU Fault Handler
    BusFault_Handler,                    // 5:  Bus Fault Handler
    UsageFault_Handler,                  // 6:  Usage Fault Handler
    0,                                   // 7:  Reserved
    0,                                   // 8:  Reserved
    0,                                   // 9:  Reserved
    0,                                   // 10: Reserved
    SVC_Handler,                         // 11: SVCall Handler
    DebugMon_Handler,                    // 12: Debug Monitor Handler
    0,                                   // 13: Reserved
    PendSV_Handler,                      // 14: PendSV Handler
    SysTick_Handler,                     // 15: SysTick Handler

    // Chip Level - MK64FX512
    DMA0_IRQHandler,                     // 16: DMA channel 0 transfer complete
    DMA1_IRQHandler,                     // 17: DMA channel 1 transfer complete
    DMA2_IRQHandler,                     // 18: DMA channel 2 transfer complete
    DMA3_IRQHandler,                     // 19: DMA channel 3 transfer complete
    DMA4_IRQHandler,                     // 20: DMA channel 4 transfer complete
    DMA5_IRQHandler,                     // 21: DMA channel 5 transfer complete
    DMA6_IRQHandler,                     // 22: DMA channel 6 transfer complete
    DMA7_IRQHandler,                     // 23: DMA channel 7 transfer complete
    DMA8_IRQHandler,                     // 24: DMA channel 8 transfer complete
    DMA9_IRQHandler,                     // 25: DMA channel 9 transfer complete
    DMA10_IRQHandler,                    // 26: DMA channel 10 transfer complete
    DMA11_IRQHandler,                    // 27: DMA channel 11 transfer complete
    DMA12_IRQHandler,                    // 28: DMA channel 12 transfer complete
    DMA13_IRQHandler,                    // 29: DMA channel 13 transfer complete
    DMA14_IRQHandler,                    // 30: DMA channel 14 transfer complete
    DMA15_IRQHandler,                    // 31: DMA channel 15 transfer complete
    DMA_Error_IRQHandler,                // 32: DMA error interrupt channels 0-15
    MCM_IRQHandler,                      // 33: MCM Normal interrupt
    FTFE_IRQHandler,                     // 34: FTFE Command complete
    Read_Collision_IRQHandler,           // 35: FTFE Read collision
    LVD_LVW_IRQHandler,                  // 36: PMC Low-voltage detect, low-voltage warning
    LLW_IRQHandler,                      // 37: Low Leakage Wakeup
    Watchdog_IRQHandler,                 // 38: WDOG interrupt
    RNG_IRQHandler,                      // 39: Random Number Generator
    I2C0_IRQHandler,                     // 40: I2C0
    I2C1_IRQHandler,                     // 41: I2C1
    SPI0_IRQHandler,                     // 42: SPI0
    SPI1_IRQHandler,                     // 43: SPI1
    I2S0_Tx_IRQHandler,                  // 44: I2S0 transmit
    I2S0_Rx_IRQHandler,                  // 45: I2S0 receive
    UART0_RX_TX_IRQHandler,              // 46: UART0 status
    UART0_ERR_IRQHandler,                // 47: UART0 error
    UART1_RX_TX_IRQHandler,              // 48: UART1 status
    UART1_ERR_IRQHandler,                // 49: UART1 error
    UART2_RX_TX_IRQHandler,              // 50: UART2 status
    UART2_ERR_IRQHandler,                // 51: UART2 error
    UART3_RX_TX_IRQHandler,              // 52: UART3 status
    UART3_ERR_IRQHandler,                // 53: UART3 error
    ADC0_IRQHandler,                     // 54: ADC0
    CMP0_IRQHandler,                     // 55: CMP0
    CMP1_IRQHandler,                     // 56: CMP1
    FTM0_IRQHandler,                     // 57: FTM0
    FTM1_IRQHandler,                     // 58: FTM1
    FTM2_IRQHandler,                     // 59: FTM2
    CMT_IRQHandler,                      // 60: CMT
    RTC_IRQHandler,                      // 61: RTC alarm interrupt
    RTC_Seconds_IRQHandler,              // 62: RTC seconds interrupt
    PIT0_IRQHandler,                     // 63: PIT channel 0
    PIT1_IRQHandler,                     // 64: PIT channel 1
    PIT2_IRQHandler,                     // 65: PIT channel 2
    PIT3_IRQHandler,                     // 66: PIT channel 3
    PDB0_IRQHandler,                     // 67: PDB0
    USB0_IRQHandler,                     // 68: USB0
    USBDCD_IRQHandler,                   // 69: USB DCD
    Reserved71_IRQHandler,               // 70: Reserved
    DAC0_IRQHandler,                     // 71: DAC0
    MCG_IRQHandler,                      // 72: MCG
    LPTimer_IRQHandler,                  // 73: Low Power Timer
    PORTA_IRQHandler,                    // 74: Port A interrupt
    PORTB_IRQHandler,                    // 75: Port B interrupt
    PORTC_IRQHandler,                    // 76: Port C interrupt
    PORTD_IRQHandler,                    // 77: Port D interrupt
    PORTE_IRQHandler,                    // 78: Port E interrupt
    SWI_IRQHandler,                      // 79: Software interrupt
    SPI2_IRQHandler,                     // 80: SPI2
    UART4_RX_TX_IRQHandler,              // 81: UART4 status
    UART4_ERR_IRQHandler,                // 82: UART4 error
    UART5_RX_TX_IRQHandler,              // 83: UART5 status
    UART5_ERR_IRQHandler,                // 84: UART5 error
    CMP2_IRQHandler,                     // 85: CMP2
    FTM3_IRQHandler,                     // 86: FTM3
    DAC1_IRQHandler,                     // 87: DAC1
    ADC1_IRQHandler,                     // 88: ADC1
    I2C2_IRQHandler,                     // 89: I2C2
    CAN0_ORed_Message_buffer_IRQHandler, // 90: CAN0 OR'd message buffer (0-15)
    CAN0_Bus_Off_IRQHandler,             // 91: CAN0 bus off
    CAN0_Error_IRQHandler,               // 92: CAN0 error
    CAN0_Tx_Warning_IRQHandler,          // 93: CAN0 Tx warning
    CAN0_Rx_Warning_IRQHandler,          // 94: CAN0 Rx warning
    CAN0_Wake_Up_IRQHandler,             // 95: CAN0 wake up
    SDHC_IRQHandler,                     // 96: SDHC
    ENET_1588_Timer_IRQHandler,          // 97: Ethernet MAC IEEE 1588 timer interrupt
    ENET_Transmit_IRQHandler,            // 98: Ethernet MAC transmit interrupt
    ENET_Receive_IRQHandler,             // 99: Ethernet MAC receive interrupt
    ENET_Error_IRQHandler,               // 100: Ethernet MAC error interrupt
};

//=============================================================================
// Reset Handler - Called on startup/reset
//=============================================================================

void Reset_Handler(void) {
    uint32_t *src, *dest;

    // Copy .data section from Flash to RAM
    src = &_sidata;
    dest = &_sdata;
    while (dest < &_edata) {
        *dest++ = *src++;
    }

    // Zero-initialize .bss section
    dest = &_sbss;
    while (dest < &_ebss) {
        *dest++ = 0;
    }

    // Call global constructors (C++)
    extern void (*__init_array_start[])(void);
    extern void (*__init_array_end[])(void);
    int count = __init_array_end - __init_array_start;
    for (int i = 0; i < count; i++) {
        __init_array_start[i]();
    }

    // Call main()
    main();

    // If main() returns, enter infinite loop
    while (1) {
        __asm volatile("wfi"); // Wait for interrupt
    }
}

//=============================================================================
// Default Handler - Catch unhandled interrupts
//=============================================================================

void Default_Handler(void) {
    // Trap unhandled interrupts
    while (1) {
        __asm volatile("bkpt #0"); // Breakpoint for debugger
    }
}
