/**
 * @file board_pins.h
 * @brief Teensy 3.5 ECU Pin Definitions
 * @version 0.1.0
 * @date 2026-02-10
 *
 * This header file defines all pin assignments for the Teensy 3.5 ECU.
 * Pin mappings can be customized based on your specific wiring configuration.
 *
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see hardware/pinout/teensy35_ecu_pinout.md for complete documentation
 */

#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include <stdint.h>

//=============================================================================
// Status LED
//=============================================================================

#define PIN_LED_STATUS          13      // Onboard LED (built-in)

//=============================================================================
// Fuel Injector Outputs (PWM)
//=============================================================================

#define PIN_INJECTOR_1          3       // Cylinder 1 injector
#define PIN_INJECTOR_2          4       // Cylinder 2 injector
#define PIN_INJECTOR_3          5       // Cylinder 3 injector
#define PIN_INJECTOR_4          6       // Cylinder 4 injector
#define PIN_INJECTOR_5          9       // Cylinder 5 injector (optional)
#define PIN_INJECTOR_6          10      // Cylinder 6 injector (optional)

//=============================================================================
// Ignition Coil Outputs (PWM)
//=============================================================================

#define PIN_IGNITION_1          20      // Cylinder 1 ignition coil
#define PIN_IGNITION_2          21      // Cylinder 2 ignition coil
#define PIN_IGNITION_3          22      // Cylinder 3 ignition coil
#define PIN_IGNITION_4          23      // Cylinder 4 ignition coil
#define PIN_IGNITION_5          25      // Cylinder 5 ignition coil (optional)
#define PIN_IGNITION_6          32      // Cylinder 6 ignition coil (optional)

//=============================================================================
// Idle Air Control (IAC) Stepper Motor
//=============================================================================

#define PIN_IAC_STEP_A1         7       // Stepper phase A+
#define PIN_IAC_STEP_A2         8       // Stepper phase A-
#define PIN_IAC_STEP_B1         11      // Stepper phase B+
#define PIN_IAC_STEP_B2         12      // Stepper phase B-

//=============================================================================
// Auxiliary Outputs
//=============================================================================

#define PIN_FUEL_PUMP           24      // Fuel pump relay
#define PIN_TACHOMETER          28      // Tachometer output signal
#define PIN_COOLING_FAN         29      // Radiator fan relay
#define PIN_CHECK_ENGINE        30      // Malfunction indicator lamp (MIL)
#define PIN_BOOST_CONTROL       31      // Electronic wastegate (PWM)

//=============================================================================
// Analog Sensor Inputs (ADC)
//=============================================================================

#define PIN_TPS                 A0      // Throttle position sensor (0-5V)
#define PIN_MAP                 A1      // Manifold absolute pressure (0-5V)
#define PIN_CLT                 A2      // Coolant temperature (thermistor)
#define PIN_IAT                 A3      // Intake air temperature (thermistor)
#define PIN_O2_SENSOR           A4      // Oxygen sensor narrowband (0-1V)
#define PIN_BATTERY_VOLTAGE     A5      // Battery voltage (12V via divider)
#define PIN_WIDEBAND_O2         A6      // Wideband O2 controller (0-5V)
#define PIN_FUEL_PRESSURE       A7      // Fuel pressure sensor (0-5V)
#define PIN_OIL_PRESSURE        A8      // Oil pressure sensor (0-5V)
#define PIN_BOOST_PRESSURE      A9      // Boost/MAP sensor (0-5V)
#define PIN_FLEX_FUEL           A10     // Flex fuel ethanol sensor (0-5V)
#define PIN_USER_ANALOG_1       A11     // User-defined analog input 1
#define PIN_USER_ANALOG_2       A12     // User-defined analog input 2

//=============================================================================
// Digital Inputs (Frequency/Position Sensors)
//=============================================================================

#define PIN_CRANK_POSITION      33      // Crankshaft position sensor
#define PIN_CAM_POSITION        34      // Camshaft position sensor
#define PIN_VEHICLE_SPEED       35      // Vehicle speed sensor (VSS)
#define PIN_KNOCK_SENSOR_1      36      // Knock sensor 1 (analog input)
#define PIN_KNOCK_SENSOR_2      37      // Knock sensor 2 (analog input)
#define PIN_CLUTCH_SWITCH       38      // Clutch pedal switch
#define PIN_BRAKE_SWITCH        39      // Brake pedal switch

//=============================================================================
// CAN Bus Interface
//=============================================================================

// NOTE: CAN pins overlap with Injector 1/2 (pins 3, 4)
// Choose either CAN bus OR use injectors 1-2, not both

#define PIN_CAN_TX              3       // CAN bus transmit (FlexCAN0)
#define PIN_CAN_RX              4       // CAN bus receive (FlexCAN0)

//=============================================================================
// Serial Communication (UART)
//=============================================================================

// USB Serial (built-in, no pins needed)
// Used for TunerStudio communication

// Serial1 (pins 0, 1) - GPS module
#define PIN_SERIAL1_RX          0
#define PIN_SERIAL1_TX          1

// Serial2 (pins 9, 10) - Wideband O2 controller
// NOTE: Overlaps with Injector 5/6
#define PIN_SERIAL2_RX          9
#define PIN_SERIAL2_TX          10

// Serial3 (pins 7, 8) - Bluetooth module
// NOTE: Overlaps with IAC stepper
#define PIN_SERIAL3_RX          7
#define PIN_SERIAL3_TX          8

//=============================================================================
// ADC Channel Mapping (for direct peripheral access)
//=============================================================================

typedef enum {
    ADC_CHANNEL_TPS = 0,
    ADC_CHANNEL_MAP,
    ADC_CHANNEL_CLT,
    ADC_CHANNEL_IAT,
    ADC_CHANNEL_O2,
    ADC_CHANNEL_BATTERY,
    ADC_CHANNEL_WIDEBAND,
    ADC_CHANNEL_FUEL_PRESSURE,
    ADC_CHANNEL_OIL_PRESSURE,
    ADC_CHANNEL_BOOST_PRESSURE,
    ADC_CHANNEL_FLEX_FUEL,
    ADC_CHANNEL_USER_1,
    ADC_CHANNEL_USER_2,
    ADC_CHANNEL_COUNT
} adc_channel_t;

//=============================================================================
// PWM Channel Mapping
//=============================================================================

typedef enum {
    PWM_CHANNEL_INJ_1 = 0,
    PWM_CHANNEL_INJ_2,
    PWM_CHANNEL_INJ_3,
    PWM_CHANNEL_INJ_4,
    PWM_CHANNEL_INJ_5,
    PWM_CHANNEL_INJ_6,
    PWM_CHANNEL_IGN_1,
    PWM_CHANNEL_IGN_2,
    PWM_CHANNEL_IGN_3,
    PWM_CHANNEL_IGN_4,
    PWM_CHANNEL_IGN_5,
    PWM_CHANNEL_IGN_6,
    PWM_CHANNEL_COUNT
} pwm_channel_t;

//=============================================================================
// Pin Validation Macros
//=============================================================================

/**
 * @brief Check if a pin number is valid for Teensy 3.5
 * @param pin Pin number to validate
 * @return true if valid, false otherwise
 */
#define IS_VALID_PIN(pin) ((pin) >= 0 && (pin) <= 63)

/**
 * @brief Check if a pin supports analog input
 * @param pin Pin number to check
 * @return true if analog-capable, false otherwise
 */
#define IS_ANALOG_PIN(pin) ((pin) >= A0 && (pin) <= A25)

/**
 * @brief Check if a pin supports PWM output
 * @param pin Pin number to check
 * @return true if PWM-capable, false otherwise
 */
#define IS_PWM_PIN(pin) (/* TODO: implement based on FTM mapping */)

#endif // BOARD_PINS_H
