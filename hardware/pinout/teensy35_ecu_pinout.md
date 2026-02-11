# Teensy 3.5 ECU Pin Mapping

**Board**: Teensy 3.5 (MK64FX512VMD12)
**Voltage**: 5V tolerant I/O, 3.3V logic
**Last Updated**: 2026-02-10

---

## Pin Assignment Overview

This document defines the default pin assignments for the rusEFI Teensy 3.5 ECU implementation. Pin assignments can be customized in `firmware/src/board/board_config.cpp`.

---

## Output Pins

### Fuel Injectors (PWM)

| Teensy Pin | Function | Peripheral | Notes |
|------------|----------|------------|-------|
| **3** | Injector 1 | FTM0_CH0 | Cylinder 1 fuel injector |
| **4** | Injector 2 | FTM0_CH1 | Cylinder 2 fuel injector |
| **5** | Injector 3 | FTM0_CH2 | Cylinder 3 fuel injector |
| **6** | Injector 4 | FTM0_CH3 | Cylinder 4 fuel injector |
| **9** | Injector 5 (optional) | FTM0_CH7 | Cylinder 5/6 for 6-cyl engines |
| **10** | Injector 6 (optional) | FTM0_CH7 | Cylinder 6 fuel injector |

**Electrical Requirements:**
- Logic-level PWM output (3.3V)
- Requires high-current driver (VNH2SP30, ULN2003, or dedicated injector IC)
- Typical frequency: 100-400 Hz
- Duty cycle: 0-100% (pulse width: 0-20ms typical)

**Recommended Driver**: VNH2SP30 half-bridge (can drive 6A inductive load)

---

### Ignition Coils (PWM)

| Teensy Pin | Function | Peripheral | Notes |
|------------|----------|------------|-------|
| **20** | Ignition 1 | FTM1_CH0 | Cylinder 1 ignition coil |
| **21** | Ignition 2 | FTM1_CH1 | Cylinder 2 ignition coil |
| **22** | Ignition 3 | FTM2_CH0 | Cylinder 3 ignition coil |
| **23** | Ignition 4 | FTM2_CH1 | Cylinder 4 ignition coil |
| **25** | Ignition 5 (optional) | FTM0_CH5 | Cylinder 5 ignition coil |
| **32** | Ignition 6 (optional) | FTM0_CH6 | Cylinder 6 ignition coil |

**Electrical Requirements:**
- Logic-level PWM output (3.3V/5V)
- Requires high-voltage IGBT driver (BIP373, VB921, or coil-on-plug module)
- Typical dwell time: 2-4ms
- Requires flyback diode or integrated protection

**Recommended Driver**: BIP373 ignition driver IC or automotive IGBT module

---

### Idle Air Control (IAC)

| Teensy Pin | Function | Peripheral | Notes |
|------------|----------|------------|-------|
| **7** | IAC Step A1 | GPIO | Stepper motor phase A+ |
| **8** | IAC Step A2 | GPIO | Stepper motor phase A- |
| **11** | IAC Step B1 | GPIO | Stepper motor phase B+ |
| **12** | IAC Step B2 | GPIO | Stepper motor phase B- |

**Electrical Requirements:**
- 4-wire stepper motor control
- Requires ULN2003A or L293D stepper driver IC
- Typical step rate: 100-500 steps/second

**Alternative**: Use PWM output (pin 24) for DC motor IAC valve

---

### Auxiliary Outputs

| Teensy Pin | Function | Peripheral | Notes |
|------------|----------|------------|-------|
| **13** | Status LED | GPIO (built-in) | On-board LED for diagnostics |
| **24** | Fuel Pump Relay | FTM0_CH4 | Main relay control (PWM capable) |
| **28** | Tachometer Output | FTM3_CH4 | Signal for aftermarket gauge |
| **29** | Cooling Fan Relay | GPIO | Low-side switch for radiator fan |
| **30** | Check Engine Light | GPIO | Malfunction indicator lamp (MIL) |
| **31** | Boost Control | FTM0_CH6 | PWM for electronic wastegate |

**Note**: All outputs require appropriate drivers (relays, transistors, or ICs)

---

## Input Pins

### Analog Sensors (ADC)

| Teensy Pin | Function | ADC Channel | Voltage Range | Notes |
|------------|----------|-------------|---------------|-------|
| **A0 (14)** | Throttle Position (TPS) | ADC0_DP0 | 0-5V | Potentiometric sensor |
| **A1 (15)** | Manifold Pressure (MAP) | ADC0_DP1 | 0-5V | 3-bar GM sensor |
| **A2 (16)** | Coolant Temp (CLT) | ADC0_DM0 | 0-3.3V | NTC thermistor + pullup |
| **A3 (17)** | Intake Air Temp (IAT) | ADC0_DM1 | 0-3.3V | NTC thermistor + pullup |
| **A4 (18)** | O2 Sensor (narrowband) | ADC0_DP2 | 0-1V | Zirconia exhaust sensor |
| **A5 (19)** | Battery Voltage | ADC0_DP3 | 0-3.3V | 12V via 1:5 divider |
| **A6 (20)** | Wideband O2 | ADC1_DP0 | 0-5V | AFR/lambda controller |
| **A7 (21)** | Fuel Pressure | ADC1_DP1 | 0-5V | 0-150 PSI sensor |
| **A8 (22)** | Oil Pressure | ADC1_DM0 | 0-5V | 0-100 PSI sensor |
| **A9 (23)** | Boost Pressure | ADC1_DM1 | 0-5V | -15 to +30 PSI MAP |
| **A10 (24)** | Flex Fuel Sensor | ADC0_DP2 | Variable | Ethanol content (0-5V) |
| **A11 (25)** | User Analog 1 | ADC1_SE18 | 0-3.3V | Custom sensor input |
| **A12 (26)** | User Analog 2 | ADC1_SE19 | 0-3.3V | Custom sensor input |

**ADC Specifications:**
- Resolution: 13-bit usable (8192 levels)
- Reference voltage: 3.3V (internal)
- Maximum input: 5V (with 5V tolerant pins)
- Sampling rate: Up to 100 kSPS per channel

**⚠️ Warning**: Pins marked 0-3.3V require voltage divider for 5V sensors

---

### Digital Inputs (Frequency/Capture)

| Teensy Pin | Function | Peripheral | Notes |
|------------|----------|------------|-------|
| **33** | Crankshaft Position | FTM0_CH4 (IC) | VR sensor or Hall effect |
| **34** | Camshaft Position | FTM0_CH5 (IC) | Hall effect sensor |
| **35** | Vehicle Speed Sensor | GPIO (INT) | Pulse counter (VSS) |
| **36** | Knock Sensor 1 | ADC1_DP3 | Requires analog conditioning |
| **37** | Knock Sensor 2 | ADC1_DM3 | Optional for V-engines |
| **38** | Clutch Switch | GPIO (INT) | Launch control / rev limiter |
| **39** | Brake Switch | GPIO (INT) | Safety interlock |

**Crank/Cam Sensor Notes:**
- VR (variable reluctance) sensors require external conditioning (LM1815, MAX9926)
- Hall effect sensors can connect directly (0-5V or 0-12V with pullup)
- Input capture mode measures pulse width and frequency
- Typical crank frequency: 100-500 Hz @ idle, 1-2 kHz @ redline

---

## Communication Interfaces

### CAN Bus

| Teensy Pin | Function | Peripheral | Notes |
|------------|----------|------------|-------|
| **CAN0_TX (3)** | CAN Transmit | FlexCAN0 TX | Requires external transceiver |
| **CAN0_RX (4)** | CAN Receive | FlexCAN0 RX | MCP2551 or SN65HVD230 IC |

**⚠️ Pin Conflict**: CAN pins overlap with Injector 1/2. Choose one function per pin.

**Alternative Pin Mapping**:
- Use Injectors 3-6 only (pins 5, 6, 9, 10)
- Use CAN for communication (pins 3, 4)

**CAN Transceiver Circuit**:
```
Teensy Pin 3 (TX) → MCP2551 Pin 1 (TXD)
Teensy Pin 4 (RX) ← MCP2551 Pin 4 (RXD)
MCP2551 Pin 6 (CANH) → CAN Bus High
MCP2551 Pin 7 (CANL) → CAN Bus Low
Add 120Ω termination resistor between CANH and CANL
```

**Typical Baud Rates**: 250 kbps, 500 kbps, 1 Mbps

---

### USB Serial (TunerStudio)

| Interface | Function | Notes |
|-----------|----------|-------|
| **USB** | Serial Console | Built-in USB-to-serial converter |
| - | TunerStudio Communication | 115200 baud default |
| - | Data Logging | Up to 10 Hz parameter updates |

**No external pins required** - uses Teensy's onboard USB controller

---

### Additional Serial Ports

| Teensy Pin | Function | Peripheral | Baud Rate | Notes |
|------------|----------|------------|-----------|-------|
| **0 (RX1)** | Serial1 RX | UART0 | 9600-115200 | GPS module input |
| **1 (TX1)** | Serial1 TX | UART0 | 9600-115200 | GPS module output |
| **9 (RX2)** | Serial2 RX | UART1 | Configurable | Wideband controller |
| **10 (TX2)** | Serial2 TX | UART1 | Configurable | LC-2 / AEM UEGO |
| **7 (RX3)** | Serial3 RX | UART2 | Configurable | Bluetooth module |
| **8 (TX3)** | Serial3 TX | UART2 | Configurable | HC-05 / HC-06 |

---

## Power Connections

| Connection | Voltage | Current | Notes |
|------------|---------|---------|-------|
| **VIN** | 5-12V | 250mA max | Main power input (regulated to 3.3V) |
| **3.3V** | 3.3V | 250mA max | Logic power output (do NOT exceed) |
| **GND** | 0V | - | Multiple ground pins available |
| **VUSB** | 5V | 500mA max | USB power (when connected to PC) |

**⚠️ Power Supply Requirements:**
- Teensy 3.5 draws ~150mA @ 3.3V typical
- Total ECU current depends on connected peripherals
- Use automotive 5V or 12V regulator with noise filtering
- Protect with TVS diode for load dump protection (36V minimum)
- Add 100µF capacitor near VIN pin

**Recommended Power Supply**:
- LM2940 (1A low-dropout 5V regulator) or
- Automotive-grade switching regulator (e.g., TI LM2576HV)

---

## Wiring Harness Pinout Table

### Connector A: Engine Sensors (20-pin)

| Pin | Signal | Wire Color | Destination |
|-----|--------|------------|-------------|
| 1 | TPS Signal | Yellow | Pin A0 |
| 2 | TPS 5V Supply | Red | External 5V regulator |
| 3 | TPS Ground | Black | Ground plane |
| 4 | MAP Signal | Green | Pin A1 |
| 5 | MAP 5V Supply | Red | External 5V regulator |
| 6 | MAP Ground | Black | Ground plane |
| 7 | CLT Signal | Blue | Pin A2 |
| 8 | CLT Ground | Black | Ground plane |
| 9 | IAT Signal | Purple | Pin A3 |
| 10 | IAT Ground | Black | Ground plane |
| ... | ... | ... | ... |

*(Full harness diagram coming soon)*

---

## Alternative Pin Configurations

### Configuration A: 4-Cylinder Sequential + CAN

```
Injectors: Pins 5, 6, 9, 10 (no pins 3, 4)
Ignition: Pins 20, 21, 22, 23
CAN Bus: Pins 3, 4
```

### Configuration B: 6-Cylinder Wasted Spark (No CAN)

```
Injectors: Pins 3, 4, 5, 6, 9, 10
Ignition: Pins 20, 21, 22 (wasted spark pairs)
CAN Bus: Disabled
```

### Configuration C: 4-Cylinder + USB Tuning Only

```
Injectors: Pins 3, 4, 5, 6
Ignition: Pins 20, 21, 22, 23
CAN Bus: Disabled (use USB serial)
```

---

## Peripheral Summary

| Peripheral | Quantity | Notes |
|------------|----------|-------|
| **PWM Channels** | 20 | FlexTimer (FTM0-FTM3) |
| **ADC Inputs** | 27 | Two 13-bit ADCs |
| **Digital I/O** | 58 | All interrupt-capable |
| **CAN Bus** | 1 | FlexCAN with external transceiver |
| **Serial Ports** | 6 | UART0-UART5 |
| **SPI** | 3 | SPI0-SPI2 |
| **I2C** | 3 | I2C0-I2C2 |
| **DMA Channels** | 16 | For high-speed ADC/PWM |

---

## Pin Conflict Matrix

⚠️ **Warning**: Some pins share peripherals. Consult the MK64FX512 datasheet for muxing details.

| Pin | Primary Function | Alternate Function 1 | Alternate Function 2 |
|-----|------------------|---------------------|---------------------|
| 3 | Injector 1 (PWM) | CAN0_TX | UART2_TX |
| 4 | Injector 2 (PWM) | CAN0_RX | UART2_RX |
| 9 | Injector 5 (PWM) | Serial2_RX (UART1) | - |
| 10 | Injector 6 (PWM) | Serial2_TX (UART1) | - |

**Recommendation**: Plan your pin usage carefully based on engine configuration and communication needs.

---

## External Components Required

### Minimum Setup (Bench Testing)
- [ ] Teensy 3.5 board
- [ ] USB cable (micro-B)
- [ ] 5V or 12V power supply (automotive regulator recommended)

### Full ECU Setup (Engine Testing)
- [ ] MCP2551 or SN65HVD230 CAN transceiver
- [ ] VNH2SP30 or similar injector drivers (x4-6)
- [ ] BIP373 or IGBT ignition drivers (x4-6)
- [ ] ULN2003A stepper driver (for IAC)
- [ ] Input signal conditioning circuits (VR sensor interface, voltage dividers)
- [ ] TVS diodes for overvoltage protection
- [ ] Automotive-grade connectors (Deutsch, AMP Superseal, etc.)
- [ ] Shielded twisted-pair wiring for sensors
- [ ] Enclosure with adequate cooling/mounting

---

## References

- [Teensy 3.5 Pinout Card (PDF)](https://www.pjrc.com/teensy/card7a_rev2.pdf)
- [MK64FX512 Reference Manual](https://www.pjrc.com/teensy/K64P144M120SF5RM.pdf)
- [rusEFI Hardware Wiki](https://github.com/rusefi/rusefi/wiki/Hardware)

---

**Document Version**: 1.0
**Last Updated**: 2026-02-10
**Maintained By**: Teensy35 Russefi Port Team
