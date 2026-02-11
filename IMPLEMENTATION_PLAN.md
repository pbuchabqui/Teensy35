# Russefi Firmware Implementation Plan for Teensy 3.5

## Executive Summary

This document outlines the comprehensive plan for implementing Russefi ECU firmware on the Teensy 3.5 development board. Russefi is an open-source GPL engine control unit firmware primarily designed for STM32F4/F7 microcontrollers. This implementation represents a significant porting effort to the Freescale/NXP Kinetis K64 platform (ARM Cortex-M4F).

**Current Status**: Initial planning phase - repository is empty
**Target Platform**: Teensy 3.5 (MK64FX512VMD12)
**Estimated Complexity**: High - requires extensive HAL porting and validation

---

## 1. Hardware Overview

### 1.1 Teensy 3.5 Specifications

| Component | Specification |
|-----------|---------------|
| **Processor** | MK64FX512VMD12 ARM Cortex-M4F @ 120MHz |
| **Flash Memory** | 512 KB |
| **RAM** | 256 KB |
| **EEPROM** | 4 KB |
| **FPU** | Single-precision floating point (Cortex-M4F) |
| **I/O Voltage** | 5V tolerant digital pins |
| **Digital I/O** | 58 pins (all interrupt-capable) |
| **Analog Input** | 27 pins, 2x 13-bit ADCs |
| **Analog Output** | 2 pins, 12-bit DAC |
| **PWM Outputs** | 20 pins |
| **CAN Bus** | 1 port (requires external transceiver) |
| **Serial Ports** | 6 UART, 3 SPI, 3 I2C |
| **DMA Channels** | 16 general-purpose |

### 1.2 Hardware Suitability for ECU Application

**Strengths:**
- Adequate processing power (120MHz Cortex-M4F with FPU)
- Sufficient memory (512KB Flash, 256KB RAM)
- Native CAN bus support
- Multiple high-resolution ADCs for sensor inputs
- Abundant PWM outputs for injector/ignition control
- 5V tolerant I/O compatible with automotive sensors

**Limitations:**
- **Discontinued Product**: Teensy 3.5 is no longer in production due to chip shortage
- Lower flash/RAM compared to modern STM32F7 boards
- Single CAN bus (many ECU applications need 2+)
- Requires external CAN transceiver (typically MCP2551 or SN65HVD230)
- Non-standard platform for Russefi (extensive porting required)

**Recommendation**: For new projects, consider STM32F4/F7-based Russefi boards (microRusEFI, Proteus, etc.) or Teensy 4.x. This Teensy 3.5 implementation should only proceed if you have existing hardware.

---

## 2. Russefi Architecture Analysis

### 2.1 Current Russefi Platform Support

**Primary Supported Platforms:**
- STM32F4 series (STM32F405, STM32F407, STM32F429)
- STM32F7 series (STM32F765, STM32F767)

**Experimental/Community Support:**
- Kinetis (limited, community-driven by @andreika)
- Cypress microcontrollers (limited)

### 2.2 Russefi Firmware Components

Based on the GitHub repository structure:

```
rusefi/
├── firmware/           # Core ECU control software
│   ├── controllers/    # Engine control algorithms
│   ├── hw_layer/       # Hardware abstraction layer (HAL)
│   ├── init/           # Initialization code
│   └── console/        # Serial console interface
├── hardware/           # KiCad PCB designs
├── java_console/       # PC tuning software
├── simulator/          # Desktop testing environment
└── unit_tests/         # Validation suite
```

**Key Areas Requiring Porting:**
1. **Hardware Abstraction Layer (hw_layer/)**: MCU-specific code
2. **Board Configuration**: Pin mappings, peripherals
3. **Build System**: Compiler flags, linker scripts
4. **Drivers**: GPIO, ADC, PWM, CAN, timers, interrupts

---

## 3. Implementation Strategy

### 3.1 Approach: Fork vs. Upstream Contribution

**Option A: Fork Russefi Repository**
- ✅ Faster initial development
- ✅ Full control over codebase
- ❌ Difficult to stay synchronized with upstream
- ❌ Duplicate maintenance effort

**Option B: Contribute to Upstream Russefi**
- ✅ Community support and review
- ✅ Synchronized with latest features
- ✅ Benefits all Kinetis/Teensy users
- ❌ Slower approval process
- ❌ Must follow project standards

**Recommended**: Start with a fork for rapid prototyping, then upstream key components once stable.

### 3.2 Phased Development Plan

#### Phase 1: Foundation (Weeks 1-3)
**Goal**: Minimal bootable firmware

1. **Build System Setup**
   - Configure CMake/Make for Kinetis toolchain
   - Set up ARM GCC for Cortex-M4F
   - Create linker script for MK64FX512 memory map
   - Integrate with Teensyduino libraries (optional)

2. **Basic Hardware Abstraction**
   - Clock initialization (120MHz PLL configuration)
   - GPIO driver (digital I/O)
   - UART driver (serial console for debugging)
   - System tick timer

3. **Validation**
   - Blink LED test
   - Serial "Hello World" output
   - Verify clock frequency

**Deliverables**: Bootable firmware that can communicate via USB serial

---

#### Phase 2: Core Peripherals (Weeks 4-7)
**Goal**: Essential sensor/actuator interfaces

1. **ADC Driver**
   - Configure 13-bit ADC for sensor inputs
   - Implement DMA-based conversion for efficiency
   - Calibration and voltage reference setup
   - Multi-channel scanning mode

2. **PWM Driver**
   - FlexTimer (FTM) configuration for injectors
   - High-resolution PWM for ignition coils
   - Complementary PWM with dead-time insertion
   - Frequency and duty cycle control

3. **CAN Bus Interface**
   - FlexCAN module initialization
   - Message transmission/reception
   - Filter configuration
   - Error handling

4. **Timing/Scheduling**
   - Periodic Interrupt Timer (PIT) for task scheduling
   - Input capture for crankshaft/camshaft position sensors
   - Microsecond-precision timing

**Deliverables**: Firmware can read analog sensors, drive PWM outputs, and communicate over CAN

---

#### Phase 3: Engine Control Logic (Weeks 8-12)
**Goal**: Integrate Russefi control algorithms

1. **Port Core Controllers**
   - Fuel injection calculation
   - Ignition timing control
   - Idle air control (IAC)
   - Electronic throttle control (if applicable)

2. **Sensor Processing**
   - Throttle position sensor (TPS)
   - Manifold absolute pressure (MAP)
   - Coolant/intake air temperature (CLT/IAT)
   - Oxygen sensor (wideband/narrowband)
   - Crankshaft/camshaft position

3. **Board Configuration**
   - Create `board_configuration.cpp` for Teensy 3.5
   - Define default pin mappings
   - Set ADC channel assignments
   - Configure PWM output assignments

**Deliverables**: Functional ECU firmware with basic engine control

---

#### Phase 4: Tuning & Validation (Weeks 13-16)
**Goal**: Real-world testing and refinement

1. **TunerStudio Integration**
   - Configure serial protocol (UART/USB)
   - Test parameter tuning interface
   - Verify data logging functionality

2. **Bench Testing**
   - Injector pulse width accuracy
   - Ignition timing precision
   - CAN message integrity
   - Sensor reading validation

3. **Engine Testing** (if applicable)
   - Cold start behavior
   - Idle stability
   - Acceleration response
   - Closed-loop fuel control

4. **Documentation**
   - Pin mapping diagrams
   - Wiring instructions
   - Calibration procedures
   - Known limitations

**Deliverables**: Validated ECU ready for real-world deployment

---

## 4. Technical Implementation Details

### 4.1 Directory Structure

Proposed organization for this repository:

```
Teensy35/
├── firmware/
│   ├── src/
│   │   ├── hal/              # Kinetis hardware abstraction
│   │   │   ├── adc_k64.cpp
│   │   │   ├── can_k64.cpp
│   │   │   ├── gpio_k64.cpp
│   │   │   ├── pwm_k64.cpp
│   │   │   └── uart_k64.cpp
│   │   ├── board/            # Teensy 3.5 specific config
│   │   │   ├── board_pins.h
│   │   │   └── board_config.cpp
│   │   ├── controllers/      # Engine control (from Russefi)
│   │   └── main.cpp
│   ├── include/
│   └── CMakeLists.txt
├── hardware/
│   ├── schematics/           # Interface board designs
│   │   ├── can_transceiver.kicad_pro
│   │   └── input_conditioning.kicad_pro
│   ├── pinout/
│   │   └── teensy35_ecu_pinout.md
│   └── datasheets/
├── tools/
│   ├── flash_teensy.sh       # Upload script
│   └── serial_console.py     # Debug interface
├── docs/
│   ├── BUILDING.md
│   ├── WIRING.md
│   ├── CALIBRATION.md
│   └── TROUBLESHOOTING.md
├── tests/
│   └── unit_tests/
└── IMPLEMENTATION_PLAN.md    # This file
```

### 4.2 Hardware Abstraction Layer Design

The HAL must bridge Russefi's API to Kinetis peripherals:

**Example: ADC Interface**

```cpp
// Russefi expects this API:
float readAnalogVoltage(adc_channel_e channel);

// Teensy 3.5 implementation:
float readAnalogVoltage(adc_channel_e channel) {
    // Map logical channel to physical ADC pin
    uint8_t pin = adcChannelMap[channel];

    // Use Kinetis ADC with DMA
    uint16_t rawValue = adc_read(ADC0, pin);

    // Convert to voltage (13-bit = 8192 levels, 3.3V reference)
    return (rawValue / 8192.0f) * 3.3f;
}
```

**Example: PWM Interface**

```cpp
// Russefi expects this API:
void setInjectorDutyCycle(uint8_t channel, float dutyCycle);

// Teensy 3.5 implementation using FlexTimer:
void setInjectorDutyCycle(uint8_t channel, float dutyCycle) {
    uint8_t ftmPin = injectorPinMap[channel];

    // Configure FTM for high-resolution PWM
    FTM_SetChannelDutyCycle(FTM0, ftmPin, dutyCycle);
}
```

### 4.3 Build System Configuration

**CMakeLists.txt** (recommended over Make for cross-platform support):

```cmake
cmake_minimum_required(VERSION 3.15)
project(teensy35_russefi C CXX ASM)

# Teensy 3.5 toolchain
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/teensy35.cmake)

# Compiler flags
set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")
set(CMAKE_C_FLAGS "${MCU_FLAGS} -O2 -Wall -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -fno-rtti")

# Linker script
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/mk64fx512.ld")
set(CMAKE_EXE_LINKER_FLAGS "-T${LINKER_SCRIPT} -Wl,--gc-sections")

# Source files
add_executable(russefi_teensy35
    src/main.cpp
    src/hal/adc_k64.cpp
    src/hal/can_k64.cpp
    # ... additional sources
)

target_link_libraries(russefi_teensy35 m)  # Math library for FPU
```

### 4.4 Pin Mapping Strategy

Teensy 3.5 default ECU pin assignments:

| Function | Pin(s) | Peripheral | Notes |
|----------|--------|------------|-------|
| **Injector 1-4** | 3, 4, 5, 6 | FTM0_CH0-3 | High-current drivers required |
| **Ignition 1-4** | 20, 21, 22, 23 | FTM1_CH0-3 | 5V logic, use ignition module |
| **TPS** | A0 (14) | ADC0_DP0 | 0-5V analog input |
| **MAP** | A1 (15) | ADC0_DP1 | GM 3-bar sensor (0-5V) |
| **CLT** | A2 (16) | ADC0_DM0 | Thermistor with pullup |
| **IAT** | A3 (17) | ADC0_DM1 | Thermistor with pullup |
| **O2 Sensor** | A4 (18) | ADC0_DP2 | 0-1V narrowband |
| **Battery Voltage** | A5 (19) | ADC0_DP3 | 1/5 voltage divider |
| **Crank Position** | 7 | FTM0_CH4 IC | VR sensor or Hall effect |
| **Cam Position** | 8 | FTM0_CH5 IC | Hall effect |
| **CAN High/Low** | 3, 4 | CAN0_TX/RX | Requires MCP2551 transceiver |
| **USB Serial** | - | USB0 | Built-in USB for tuning |
| **Idle Stepper** | 9-12 | GPIO | 4-wire stepper driver |

---

## 5. Critical Dependencies & Requirements

### 5.1 Software Dependencies

| Component | Version | Purpose |
|-----------|---------|---------|
| **ARM GCC Toolchain** | 10.3+ | Cross-compiler for ARM Cortex-M4 |
| **CMake** | 3.15+ | Build system generator |
| **Teensy Loader CLI** | Latest | Firmware upload utility |
| **OpenOCD** (optional) | 0.11+ | JTAG debugging |
| **Kinetis SDK** (optional) | 2.x | NXP peripheral drivers |

### 5.2 Hardware Dependencies

**Minimum Setup (Development):**
- Teensy 3.5 board
- USB cable (micro-B)
- Bench power supply (5V or 12V automotive)

**Full ECU Setup (Engine Testing):**
- MCP2551 or SN65HVD230 CAN transceiver
- High-current injector drivers (e.g., VNH2SP30 or dedicated IC)
- Ignition coil drivers (e.g., BIP373 or IGBT modules)
- Input signal conditioning (voltage dividers, RC filters)
- Automotive-grade connectors and wiring
- Enclosure with thermal management

### 5.3 Russefi Components to Integrate

**From Russefi Repository (https://github.com/rusefi/rusefi):**
- Engine control algorithms (`firmware/controllers/`)
- Math utilities and lookup tables
- Configuration structures
- TunerStudio protocol implementation
- Sensor processing logic

**Adapt/Rewrite for Kinetis:**
- Low-level drivers (`firmware/hw_layer/`)
- Board-specific configuration
- Build system files
- Startup code and linker scripts

---

## 6. Risk Assessment & Mitigation

### 6.1 Technical Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| **Insufficient Flash/RAM** | Medium | High | Profile memory usage, optimize code size |
| **Timing Precision Issues** | Medium | Critical | Validate interrupt latency, use DMA where possible |
| **CAN Bus Compatibility** | Low | Medium | Thorough testing with standard CAN tools |
| **Russefi API Changes** | High | Medium | Pin to stable Russefi release, not master branch |
| **Hardware Damage** | Low | High | Implement current limiting, overvoltage protection |

### 6.2 Project Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| **Teensy 3.5 Unavailability** | High | High | Stockpile boards, plan migration to Teensy 4.x |
| **Incomplete Kinetis Documentation** | Medium | Medium | Use PJRC forums, NXP application notes |
| **Lack of Community Support** | High | Medium | Document thoroughly, engage Russefi developers |
| **Scope Creep** | High | Medium | Strict phase gating, focus on MVP first |

---

## 7. Testing & Validation Strategy

### 7.1 Unit Testing

**Framework**: Google Test (gtest) with hardware mocking

**Test Cases:**
- ADC conversion accuracy (±1% of reference voltage)
- PWM frequency and duty cycle (±0.5% tolerance)
- CAN message encoding/decoding
- Timing calculations (fuel/ignition)
- Sensor linearization (temperature curves, MAP transfer function)

### 7.2 Integration Testing

**Bench Test Setup:**
- Signal generator for simulated crank/cam signals
- Oscilloscope for PWM output verification
- CAN bus analyzer (e.g., CANable)
- Precision voltage source for sensor simulation

**Test Scenarios:**
- Engine cranking simulation (no fuel/spark)
- Idle conditions (600-1000 RPM)
- WOT acceleration (redline sweep)
- Deceleration fuel cut-off
- Error handling (sensor faults, CAN timeout)

### 7.3 Engine Testing

**Safety Prerequisites:**
- Kill switch wired to ignition enable
- Fuel pressure safety shutdown
- Coolant temperature monitoring
- Backup ECU for emergency failover

**Test Matrix:**
- [ ] Cold start (-10°C to 25°C)
- [ ] Hot start (80°C+)
- [ ] Idle stability (load variations)
- [ ] Part-throttle cruise
- [ ] Wide-open throttle (dynamometer recommended)
- [ ] Transient response (tip-in/tip-out)
- [ ] Closed-loop lambda control
- [ ] Extended durability run (1+ hour)

---

## 8. Documentation Deliverables

### 8.1 Technical Documentation

1. **BUILDING.md**: Step-by-step compilation instructions
2. **FLASHING.md**: Firmware upload procedures
3. **WIRING.md**: Complete pin mapping and schematics
4. **CALIBRATION.md**: Initial tuning guide
5. **TROUBLESHOOTING.md**: Common issues and solutions
6. **API_REFERENCE.md**: HAL function documentation

### 8.2 User Documentation

1. **Quick Start Guide**: 0 to running engine in 30 minutes
2. **Safety Guidelines**: Electrical and mechanical hazards
3. **Tuning Tutorial**: TunerStudio configuration walkthrough
4. **Known Limitations**: What this ECU cannot do

---

## 9. Success Criteria

### 9.1 Minimum Viable Product (MVP)

A successful implementation must demonstrate:

- ✅ Firmware boots and initializes all peripherals
- ✅ USB serial console functional for debugging
- ✅ Reads 8+ analog sensors with <2% error
- ✅ Outputs 4+ PWM channels with <1% duty cycle error
- ✅ CAN bus communication verified with external tool
- ✅ TunerStudio can connect and modify parameters
- ✅ Simulated engine cranking produces injector pulses

### 9.2 Production-Ready Target

For real-world deployment:

- ✅ Starts and idles a real engine (any configuration)
- ✅ Closed-loop fuel control functional (O2 sensor feedback)
- ✅ Ignition timing accuracy ±1° crank angle
- ✅ Stable idle (<50 RPM deviation)
- ✅ Smooth acceleration with no hesitation
- ✅ Data logging captures 50+ parameters at 10Hz+
- ✅ Error handling recovers from sensor faults
- ✅ 100+ hour durability test with no crashes

---

## 10. Timeline & Milestones

### Estimated Timeline: 16 Weeks (4 Months)

| Phase | Duration | Key Milestone | Target Date |
|-------|----------|---------------|-------------|
| **Phase 1** | 3 weeks | Bootable firmware with serial console | Week 3 |
| **Phase 2** | 4 weeks | All peripherals functional (ADC/PWM/CAN) | Week 7 |
| **Phase 3** | 4 weeks | Engine control logic integrated | Week 11 |
| **Phase 4** | 5 weeks | Bench + engine testing complete | Week 16 |

**Critical Path Items:**
1. Week 1: Build system operational
2. Week 4: ADC reading accurate sensors
3. Week 7: PWM driving real injectors
4. Week 12: First engine start attempt
5. Week 16: Tuning refinement complete

---

## 11. Alternative Approaches Considered

### 11.1 Speeduino Firmware

**Speeduino** is another open-source ECU firmware with existing Teensy 3.5 support.

**Comparison:**

| Aspect | Russefi | Speeduino |
|--------|---------|-----------|
| **Teensy Support** | Requires porting | Native support |
| **Feature Set** | More advanced (sequential, VVT, etc.) | Simpler (batch injection) |
| **Tuning Software** | TunerStudio (advanced) | TunerStudio (simplified) |
| **Community** | Smaller, STM32-focused | Larger Arduino community |
| **Code Quality** | Professional-grade C++ | Arduino-style C++ |

**Recommendation**: If you need quick results, use Speeduino. If you need advanced features (sequential injection, flex fuel, etc.), proceed with Russefi porting.

### 11.2 Teensy 4.x Migration

Teensy 4.0/4.1 uses NXP i.MX RT1062 (ARM Cortex-M7 @ 600MHz) with significantly more resources.

**Advantages:**
- Actively manufactured (not discontinued)
- 5x faster processor
- More RAM/Flash
- Modern peripherals

**Disadvantages:**
- Different architecture (requires new HAL)
- No native CAN (requires external MCP2515)
- 3.3V I/O (not 5V tolerant)

**Recommendation**: Consider Teensy 4.1 for new designs, but proceed with 3.5 if you have existing hardware.

---

## 12. Next Steps (Immediate Actions)

### 12.1 Repository Setup

1. ✅ Create directory structure (firmware/, hardware/, docs/, tools/)
2. ✅ Add `.gitignore` for build artifacts
3. ✅ Create initial `README.md` with project overview
4. ✅ Add `LICENSE` file (GPL to match Russefi)

### 12.2 Development Environment

1. Install ARM GCC toolchain: `sudo apt install gcc-arm-none-eabi`
2. Install Teensy Loader: Download from PJRC.com
3. Clone Russefi repository: `git clone https://github.com/rusefi/rusefi.git`
4. Set up CMake build system

### 12.3 Initial Prototype

1. Create `main.cpp` with basic LED blink
2. Write minimal linker script for MK64FX512
3. Compile and flash to Teensy 3.5
4. Verify UART output at 115200 baud

---

## 13. References & Resources

### 13.1 Datasheets & Manuals

- [MK64FX512 Reference Manual (PDF)](https://www.pjrc.com/teensy/K64P144M120SF5RM.pdf) - Complete peripheral documentation
- [MK64FX512 Datasheet (PDF)](https://www.pjrc.com/teensy/K64P144M120SF5.pdf) - Electrical specifications
- [Teensy 3.5 Schematic](https://www.pjrc.com/teensy/schematic35.png) - Official board design

### 13.2 Russefi Resources

- [Russefi GitHub Repository](https://github.com/rusefi/rusefi) - Main source code
- [Russefi Wiki](https://github.com/rusefi/rusefi/wiki/) - Documentation hub
- [Russefi Forums](https://rusefi.com/forum/) - Community support
- [Custom Firmware Guide](https://github.com/rusefi/rusefi/wiki/Custom-Firmware) - Porting instructions

### 13.3 Teensy Resources

- [PJRC Teensy 3.5 Page](https://www.pjrc.com/store/teensy35.html) - Official product page
- [Teensy Forum](https://forum.pjrc.com/) - Community discussions
- [Teensyduino](https://www.pjrc.com/teensy/td_download.html) - Arduino IDE integration
- [PlatformIO Teensy 3.5](https://docs.platformio.org/en/latest/boards/teensy/teensy35.html) - Alternative build system

### 13.4 Community Examples

- [Speeduino Teensy 3.5 ECU](https://github.com/dvjcodec/ECU_Teen_3.5_rev.B) - Working Teensy ECU design
- [FlasherX](https://github.com/joepasquariello/FlasherX) - OTA firmware updates for Teensy

---

## 14. Conclusion

Porting Russefi to Teensy 3.5 is a **challenging but feasible project** requiring:

- **4 months of dedicated development** (16 weeks)
- **Strong embedded C++ skills** (HAL programming, real-time systems)
- **Automotive domain knowledge** (engine control theory, CAN protocols)
- **Hardware interfacing experience** (oscilloscope debugging, signal conditioning)

**Key Success Factors:**
1. Start with minimal bootable firmware (Phase 1)
2. Validate each peripheral individually (Phase 2)
3. Integrate Russefi incrementally (Phase 3)
4. Extensive bench testing before engine testing (Phase 4)

**Critical Warnings:**
⚠️ Teensy 3.5 is discontinued - consider Teensy 4.x or STM32-based boards for production
⚠️ Russefi does NOT officially support Kinetis - expect limited community help
⚠️ Engine testing is dangerous - implement safety interlocks and have backup ECU

**Alternatives to Consider:**
- **Speeduino**: Native Teensy 3.5 support, simpler feature set
- **Russefi on STM32**: Official platform with extensive support
- **Teensy 4.1 + MegaSquirt**: Modern hardware with proven firmware

If you proceed, this plan provides the roadmap. Good luck! 🚗⚡

---

**Document Version**: 1.0
**Last Updated**: 2026-02-10
**Author**: Claude (AI Assistant)
**Repository**: https://github.com/pbuchabqui/Teensy35
