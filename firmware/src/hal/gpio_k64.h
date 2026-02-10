/**
 * @file gpio_k64.h
 * @brief GPIO driver for Kinetis K64 (Teensy 3.5)
 * @version 1.0.0
 * @date 2026-02-10
 *
 * This file provides GPIO (General Purpose Input/Output) control for
 * the MK64FX512 microcontroller.
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#ifndef GPIO_K64_H
#define GPIO_K64_H

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// GPIO Ports
//=============================================================================

typedef enum {
    GPIO_PORT_A = 0,
    GPIO_PORT_B = 1,
    GPIO_PORT_C = 2,
    GPIO_PORT_D = 3,
    GPIO_PORT_E = 4,
} gpio_port_t;

//=============================================================================
// GPIO Pin Numbers
//=============================================================================

typedef enum {
    GPIO_PIN_0  = 0,
    GPIO_PIN_1  = 1,
    GPIO_PIN_2  = 2,
    GPIO_PIN_3  = 3,
    GPIO_PIN_4  = 4,
    GPIO_PIN_5  = 5,
    GPIO_PIN_6  = 6,
    GPIO_PIN_7  = 7,
    GPIO_PIN_8  = 8,
    GPIO_PIN_9  = 9,
    GPIO_PIN_10 = 10,
    GPIO_PIN_11 = 11,
    GPIO_PIN_12 = 12,
    GPIO_PIN_13 = 13,
    GPIO_PIN_14 = 14,
    GPIO_PIN_15 = 15,
    GPIO_PIN_16 = 16,
    GPIO_PIN_17 = 17,
    GPIO_PIN_18 = 18,
    GPIO_PIN_19 = 19,
    GPIO_PIN_20 = 20,
    GPIO_PIN_21 = 21,
    GPIO_PIN_22 = 22,
    GPIO_PIN_23 = 23,
    GPIO_PIN_24 = 24,
    GPIO_PIN_25 = 25,
    GPIO_PIN_26 = 26,
    GPIO_PIN_27 = 27,
    GPIO_PIN_28 = 28,
    GPIO_PIN_29 = 29,
    GPIO_PIN_30 = 30,
    GPIO_PIN_31 = 31,
} gpio_pin_t;

//=============================================================================
// GPIO Direction
//=============================================================================

typedef enum {
    GPIO_DIR_INPUT  = 0,
    GPIO_DIR_OUTPUT = 1,
} gpio_dir_t;

//=============================================================================
// GPIO Pin State
//=============================================================================

typedef enum {
    GPIO_STATE_LOW  = 0,
    GPIO_STATE_HIGH = 1,
} gpio_state_t;

//=============================================================================
// PORT Register Definitions
//=============================================================================

#define PORTA_BASE                  0x40049000
#define PORTB_BASE                  0x4004A000
#define PORTC_BASE                  0x4004B000
#define PORTD_BASE                  0x4004C000
#define PORTE_BASE                  0x4004D000

typedef struct {
    volatile uint32_t PCR[32];    // Pin Control Registers
    volatile uint32_t GPCLR;      // Global Pin Control Low Register
    volatile uint32_t GPCHR;      // Global Pin Control High Register
    volatile uint32_t RESERVED[6];
    volatile uint32_t ISFR;       // Interrupt Status Flag Register
} PORT_Type;

#define PORTA                       ((PORT_Type*)PORTA_BASE)
#define PORTB                       ((PORT_Type*)PORTB_BASE)
#define PORTC                       ((PORT_Type*)PORTC_BASE)
#define PORTD                       ((PORT_Type*)PORTD_BASE)
#define PORTE                       ((PORT_Type*)PORTE_BASE)

// PORT_PCR bits
#define PORT_PCR_ISF                0x01000000  // Interrupt Status Flag
#define PORT_PCR_IRQC_MASK          0x000F0000  // Interrupt Configuration
#define PORT_PCR_MUX_MASK           0x00000700  // Pin Mux Control
#define PORT_PCR_MUX(x)             (((x) & 0x07) << 8)
#define PORT_PCR_DSE                0x00000040  // Drive Strength Enable
#define PORT_PCR_PFE                0x00000010  // Passive Filter Enable
#define PORT_PCR_SRE                0x00000004  // Slew Rate Enable
#define PORT_PCR_PE                 0x00000002  // Pull Enable
#define PORT_PCR_PS                 0x00000001  // Pull Select

// Pin mux values
#define PORT_MUX_DISABLED           0
#define PORT_MUX_GPIO               1
#define PORT_MUX_ALT2               2
#define PORT_MUX_ALT3               3
#define PORT_MUX_ALT4               4
#define PORT_MUX_ALT5               5
#define PORT_MUX_ALT6               6
#define PORT_MUX_ALT7               7

//=============================================================================
// GPIO Register Definitions
//=============================================================================

#define GPIOA_BASE                  0x400FF000
#define GPIOB_BASE                  0x400FF040
#define GPIOC_BASE                  0x400FF080
#define GPIOD_BASE                  0x400FF0C0
#define GPIOE_BASE                  0x400FF100

typedef struct {
    volatile uint32_t PDOR;       // Port Data Output Register
    volatile uint32_t PSOR;       // Port Set Output Register
    volatile uint32_t PCOR;       // Port Clear Output Register
    volatile uint32_t PTOR;       // Port Toggle Output Register
    volatile uint32_t PDIR;       // Port Data Input Register
    volatile uint32_t PDDR;       // Port Data Direction Register
} GPIO_Type;

#define GPIOA                       ((GPIO_Type*)GPIOA_BASE)
#define GPIOB                       ((GPIO_Type*)GPIOB_BASE)
#define GPIOC                       ((GPIO_Type*)GPIOC_BASE)
#define GPIOD                       ((GPIO_Type*)GPIOD_BASE)
#define GPIOE                       ((GPIO_Type*)GPIOE_BASE)

//=============================================================================
// Function Prototypes
//=============================================================================

/**
 * @brief Initialize GPIO subsystem
 *
 * Enables clocks for all GPIO ports
 */
void gpio_init(void);

/**
 * @brief Configure a GPIO pin
 *
 * @param port GPIO port (A, B, C, D, or E)
 * @param pin Pin number (0-31)
 * @param dir Direction (input or output)
 */
void gpio_config(gpio_port_t port, gpio_pin_t pin, gpio_dir_t dir);

/**
 * @brief Set GPIO pin high
 *
 * @param port GPIO port
 * @param pin Pin number
 */
void gpio_set(gpio_port_t port, gpio_pin_t pin);

/**
 * @brief Set GPIO pin low
 *
 * @param port GPIO port
 * @param pin Pin number
 */
void gpio_clear(gpio_port_t port, gpio_pin_t pin);

/**
 * @brief Toggle GPIO pin state
 *
 * @param port GPIO port
 * @param pin Pin number
 */
void gpio_toggle(gpio_port_t port, gpio_pin_t pin);

/**
 * @brief Write GPIO pin state
 *
 * @param port GPIO port
 * @param pin Pin number
 * @param state Pin state (HIGH or LOW)
 */
void gpio_write(gpio_port_t port, gpio_pin_t pin, gpio_state_t state);

/**
 * @brief Read GPIO pin state
 *
 * @param port GPIO port
 * @param pin Pin number
 * @return Pin state (HIGH or LOW)
 */
gpio_state_t gpio_read(gpio_port_t port, gpio_pin_t pin);

#endif // GPIO_K64_H
