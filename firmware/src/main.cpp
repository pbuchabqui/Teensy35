/**
 * @file main.cpp
 * @brief Russefi ECU Firmware - Teensy 3.5 Port
 * @version 0.1.0 (Phase 1)
 * @date 2026-02-10
 *
 * This file contains the main entry point for the rusEFI firmware port
 * to the Teensy 3.5 (Freescale/NXP Kinetis MK64FX512) platform.
 *
 * Phase 1: Minimal bootable firmware with LED blink and serial console
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#include <stdint.h>
#include <string.h>
extern "C" {
#include "hal/clock_k64.h"
#include "hal/gpio_k64.h"
#include "hal/uart_k64.h"
#include "communication/tunerstudio/tunerstudio.h"
#include "config/config.h"
}
#include "fatfs/fatfs_wrapper.h"

//=============================================================================
// Hardware Configuration
//=============================================================================

// LED on Teensy 3.5 is on Port C, Pin 5
#define LED_PORT    GPIO_PORT_C
#define LED_PIN     GPIO_PIN_5

// Debug UART configuration
#define DEBUG_UART  UART_0
#define UART_BAUD   115200

//=============================================================================
// Global Variables
//=============================================================================

static volatile uint32_t systick_count = 0;

//=============================================================================
// SysTick Handler - Called every 1ms
//=============================================================================

extern "C" void SysTick_Handler(void) {
    systick_count++;
}

//=============================================================================
// Utility Functions
//=============================================================================

/**
 * @brief Simple delay in milliseconds
 * @param ms Delay time in milliseconds
 */
void delay_ms(uint32_t ms) {
    uint32_t start = systick_count;
    while ((systick_count - start) < ms) {
        __asm volatile("wfi"); // Wait for interrupt (low power)
    }
}

/**
 * @brief Get system uptime in milliseconds
 * @return Uptime in milliseconds
 */
uint32_t millis(void) {
    return systick_count;
}

/**
 * @brief Print string to debug UART
 * @param str Null-terminated string
 */
void print(const char* str) {
    uart_puts(DEBUG_UART, str);
}

/**
 * @brief Print string with newline
 * @param str Null-terminated string
 */
void println(const char* str) {
    uart_puts(DEBUG_UART, str);
    uart_puts(DEBUG_UART, "\r\n");
}

/**
 * @brief Print unsigned 32-bit integer
 * @param value Value to print
 */
void print_uint(uint32_t value) {
    char buffer[12];
    int i = 0;

    if (value == 0) {
        uart_putc(DEBUG_UART, '0');
        return;
    }

    // Convert to string (reverse order)
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    // Print in correct order
    while (i > 0) {
        uart_putc(DEBUG_UART, buffer[--i]);
    }
}

//=============================================================================
// System Initialization
//=============================================================================

/**
 * @brief Initialize SysTick timer for 1ms interrupts
 */
void systick_init(void) {
    // SysTick reload value for 1ms tick at 120MHz
    uint32_t reload_value = (CPU_CORE_CLK_HZ / 1000) - 1;

    // Configure SysTick
    *((volatile uint32_t*)0xE000E014) = reload_value;  // SYST_RVR
    *((volatile uint32_t*)0xE000E018) = 0;             // SYST_CVR (clear)
    *((volatile uint32_t*)0xE000E010) = 0x00000007;    // SYST_CSR (enable, interrupt, core clock)
}

/**
 * @brief Print startup banner
 */
void print_banner(void) {
    println("========================================");
    println("   Russefi Teensy 3.5 ECU Firmware");
    println("========================================");
    println("Version: 0.1.0 (Phase 1)");
    println("Platform: Teensy 3.5 (MK64FX512)");
    println("CPU Speed: 120 MHz");
    println("License: GPL v3");
    println("========================================");
    println("");
}

/**
 * @brief Print system information
 */
void print_system_info(void) {
    println("System Information:");
    println("------------------");
    println("Processor: MK64FX512VMD12 (Kinetis K64)");
    println("Architecture: ARM Cortex-M4F @ 120 MHz");
    println("FPU: Single-precision (32-bit float)");
    println("Flash Memory: 512 KB");
    println("RAM: 256 KB");
    println("EEPROM: 4 KB");
    println("Analog Inputs: 27 channels (13-bit ADC)");
    println("PWM Outputs: 20 channels");
    println("Digital I/O: 58 pins (5V tolerant)");
    println("CAN Bus: 1x FlexCAN");
    println("Serial Ports: 6x UART");
    println("");
}

//=============================================================================
// Main Function
//=============================================================================

int main(void) {
    // Initialize system clocks (120 MHz)
    clock_init();

    // Initialize GPIO subsystem
    gpio_init();

    // Configure LED pin as output
    gpio_config(LED_PORT, LED_PIN, GPIO_DIR_OUTPUT);
    gpio_clear(LED_PORT, LED_PIN); // LED off initially

    // Initialize UART for debug output
    uart_config_t uart_cfg = {
        .baud_rate = UART_BAUD,
        .enable_tx = true,
        .enable_rx = false,
    };
    uart_init(DEBUG_UART, &uart_cfg);

    // Initialize SysTick for millisecond timing
    systick_init();

    // Small delay to allow UART to stabilize
    delay_ms(100);

    // Print startup information
    print_banner();
    print_system_info();

    println("Initialization complete.");
    println("LED will blink at 1 Hz");
    println("");

    println("rusEFI Teensy 3.5 v2.2.0 - Basic functionality test");
    println("FatFS and Wideband updates implemented (see documentation)");
    
    // Initialize TunerStudio communication
    tunerstudio_init();
    println("TunerStudio communication initialized");
    
    // Initialize configuration system
    config_init();
    println("Configuration system initialized");

    // Main loop
    uint32_t last_blink = 0;
    uint32_t last_heartbeat = 0;
    bool led_state = false;

    while (1) {
        uint32_t now = millis();

        // Blink LED every 500ms (1 Hz = 500ms on, 500ms off)
        if ((now - last_blink) >= 500) {
            last_blink = now;
            led_state = !led_state;

            if (led_state) {
                gpio_set(LED_PORT, LED_PIN);
            } else {
                gpio_clear(LED_PORT, LED_PIN);
            }
        }

        // Print heartbeat every 1 second
        if ((now - last_heartbeat) >= 1000) {
            last_heartbeat = now;

            print("Heartbeat: ");
            print_uint(now / 1000);
            println(" seconds uptime");
        }

        // Sleep until next interrupt (low power)
        __asm volatile("wfi");

        // Handle TunerStudio communication
        tunerstudio_update();
        
        // TODO: Add ECU logic here
        // - Read sensors (MAP, IAT, CLT, TPS, O2, etc.)
        // - Calculate fuel injection timing
        // - Calculate ignition timing
        // - Update PWM outputs
        // - Process CAN messages
        // - Handle TunerStudio communication
    }

    // Should never reach here
    return 0;
}
