/**
 * @file gpio_k64.c
 * @brief GPIO driver implementation for Kinetis K64
 * @version 1.0.0
 * @date 2026-02-10
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 */

#include "gpio_k64.h"
#include "clock_k64.h"

//=============================================================================
// Private Helper Functions
//=============================================================================

/**
 * @brief Get PORT register pointer for a given port
 *
 * @param port GPIO port
 * @return Pointer to PORT registers
 */
static PORT_Type* gpio_get_port_regs(gpio_port_t port) {
    switch (port) {
        case GPIO_PORT_A: return PORTA;
        case GPIO_PORT_B: return PORTB;
        case GPIO_PORT_C: return PORTC;
        case GPIO_PORT_D: return PORTD;
        case GPIO_PORT_E: return PORTE;
        default: return NULL;
    }
}

/**
 * @brief Get GPIO register pointer for a given port
 *
 * @param port GPIO port
 * @return Pointer to GPIO registers
 */
static GPIO_Type* gpio_get_gpio_regs(gpio_port_t port) {
    switch (port) {
        case GPIO_PORT_A: return GPIOA;
        case GPIO_PORT_B: return GPIOB;
        case GPIO_PORT_C: return GPIOC;
        case GPIO_PORT_D: return GPIOD;
        case GPIO_PORT_E: return GPIOE;
        default: return NULL;
    }
}

//=============================================================================
// Public Functions
//=============================================================================

void gpio_init(void) {
    // Enable clock for all GPIO ports in SIM
    SIM->SCGC5 |= SIM_SCGC5_PORTA |
                  SIM_SCGC5_PORTB |
                  SIM_SCGC5_PORTC |
                  SIM_SCGC5_PORTD |
                  SIM_SCGC5_PORTE;
}

void gpio_config(gpio_port_t port, gpio_pin_t pin, gpio_dir_t dir) {
    PORT_Type* port_regs = gpio_get_port_regs(port);
    GPIO_Type* gpio_regs = gpio_get_gpio_regs(port);

    if (port_regs == NULL || gpio_regs == NULL) {
        return; // Invalid port
    }

    // Configure pin mux for GPIO function
    port_regs->PCR[pin] = PORT_PCR_MUX(PORT_MUX_GPIO);

    // Set pin direction
    if (dir == GPIO_DIR_OUTPUT) {
        gpio_regs->PDDR |= (1U << pin);
    } else {
        gpio_regs->PDDR &= ~(1U << pin);
    }
}

void gpio_set(gpio_port_t port, gpio_pin_t pin) {
    GPIO_Type* gpio_regs = gpio_get_gpio_regs(port);

    if (gpio_regs != NULL) {
        gpio_regs->PSOR = (1U << pin);
    }
}

void gpio_clear(gpio_port_t port, gpio_pin_t pin) {
    GPIO_Type* gpio_regs = gpio_get_gpio_regs(port);

    if (gpio_regs != NULL) {
        gpio_regs->PCOR = (1U << pin);
    }
}

void gpio_toggle(gpio_port_t port, gpio_pin_t pin) {
    GPIO_Type* gpio_regs = gpio_get_gpio_regs(port);

    if (gpio_regs != NULL) {
        gpio_regs->PTOR = (1U << pin);
    }
}

void gpio_write(gpio_port_t port, gpio_pin_t pin, gpio_state_t state) {
    if (state == GPIO_STATE_HIGH) {
        gpio_set(port, pin);
    } else {
        gpio_clear(port, pin);
    }
}

gpio_state_t gpio_read(gpio_port_t port, gpio_pin_t pin) {
    GPIO_Type* gpio_regs = gpio_get_gpio_regs(port);

    if (gpio_regs != NULL) {
        return (gpio_regs->PDIR & (1U << pin)) ? GPIO_STATE_HIGH : GPIO_STATE_LOW;
    }

    return GPIO_STATE_LOW;
}
