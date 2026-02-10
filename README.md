# Russefi on Teensy 3.5

**Open-source ECU firmware port for Teensy 3.5 development board**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Platform](https://img.shields.io/badge/Platform-Teensy%203.5-orange)](https://www.pjrc.com/store/teensy35.html)
[![Status](https://img.shields.io/badge/Status-Planning-yellow)]()

---

## ⚠️ Project Status: Initial Planning Phase

This repository contains the planning and development work for porting [rusEFI](https://github.com/rusefi/rusefi) engine control unit firmware to the Teensy 3.5 platform (Freescale/NXP Kinetis MK64FX512).

**Current Phase**: Architecture planning and feasibility analysis
**Target Platform**: Teensy 3.5 (ARM Cortex-M4F @ 120MHz)
**Base Firmware**: rusEFI (STM32-based open-source ECU)

---

## 🎯 Project Goals

1. Port rusEFI firmware from STM32F4/F7 to Kinetis K64 architecture
2. Implement hardware abstraction layer (HAL) for Teensy 3.5 peripherals
3. Validate ECU functionality on bench testing equipment
4. Enable TunerStudio integration for configuration and tuning
5. Document comprehensive setup and calibration procedures

---

## 🚨 Important Warnings

- **Discontinued Hardware**: Teensy 3.5 is no longer manufactured due to chip shortages. Consider Teensy 4.x or STM32-based boards for new projects.
- **Unofficial Port**: rusEFI does NOT officially support Kinetis. This is an experimental community effort.
- **Safety Critical**: Engine control systems can cause vehicle damage or injury if improperly implemented. Always have backup safety systems.
- **No Warranty**: This software is provided AS-IS with no guarantees of fitness for any purpose.

---

## 📋 Key Features (Planned)

### Hardware Capabilities
- **Processor**: MK64FX512 ARM Cortex-M4F @ 120MHz with hardware FPU
- **Memory**: 512KB Flash, 256KB RAM, 4KB EEPROM
- **Analog Inputs**: 27 channels, 2x 13-bit ADCs
- **PWM Outputs**: 20 channels for injectors/ignition
- **CAN Bus**: 1x FlexCAN (requires external transceiver)
- **I/O**: 5V tolerant digital pins, USB serial for tuning

### ECU Features (from rusEFI)
- Sequential fuel injection
- Wasted spark or sequential ignition
- Variable valve timing (VVT) control
- Electronic throttle control
- Closed-loop lambda control
- Launch control and traction control
- CAN communication for sensors/gauges
- Data logging and real-time tuning

---

## 📁 Repository Structure

```
Teensy35/
├── IMPLEMENTATION_PLAN.md    # Comprehensive development roadmap
├── firmware/                 # ECU firmware source code
│   ├── src/
│   │   ├── hal/             # Kinetis hardware abstraction layer
│   │   ├── board/           # Teensy 3.5 board configuration
│   │   ├── controllers/     # Engine control algorithms
│   │   └── main.cpp
│   ├── include/
│   └── CMakeLists.txt       # Build configuration
├── hardware/                 # Schematics and wiring
│   ├── schematics/          # KiCad interface board designs
│   ├── pinout/              # Pin mapping documentation
│   └── datasheets/          # Component datasheets
├── docs/                     # User and developer documentation
├── tools/                    # Build and debug utilities
└── tests/                    # Unit and integration tests
```

---

## 🚀 Quick Start

**Prerequisites:**
- Teensy 3.5 development board
- ARM GCC toolchain (`gcc-arm-none-eabi`)
- CMake 3.15+
- Teensy Loader CLI

**Clone and Build** (once implementation begins):
```bash
git clone https://github.com/pbuchabqui/Teensy35.git
cd Teensy35
mkdir build && cd build
cmake ../firmware
make
```

**Flash Firmware**:
```bash
teensy_loader_cli -mmcu=mk64fx512 -w russefi_teensy35.hex
```

---

## 📖 Documentation

- [**IMPLEMENTATION_PLAN.md**](IMPLEMENTATION_PLAN.md) - Complete development roadmap and technical architecture
- [**Hardware Pinout**](hardware/pinout/teensy35_ecu_pinout.md) - ECU pin assignments and wiring
- [**Building**](docs/BUILDING.md) - Compilation instructions (coming soon)
- [**Wiring Guide**](docs/WIRING.md) - Interface circuit designs (coming soon)
- [**Calibration**](docs/CALIBRATION.md) - Tuning procedures (coming soon)

---

## 🛠️ Development Roadmap

### Phase 1: Foundation (Weeks 1-3) - 🟡 In Progress
- [x] Architecture research and planning
- [ ] Build system setup (CMake + ARM GCC)
- [ ] Basic HAL (GPIO, UART, timers)
- [ ] Bootable firmware with LED blink

### Phase 2: Core Peripherals (Weeks 4-7) - ⚪ Not Started
- [ ] ADC driver for sensor inputs
- [ ] PWM driver for injectors/ignition
- [ ] CAN bus interface
- [ ] Input capture for crank/cam sensors

### Phase 3: Engine Control (Weeks 8-12) - ⚪ Not Started
- [ ] Port rusEFI control algorithms
- [ ] Sensor processing and calibration
- [ ] TunerStudio protocol integration
- [ ] Board configuration file

### Phase 4: Validation (Weeks 13-16) - ⚪ Not Started
- [ ] Bench testing (injector/ignition accuracy)
- [ ] CAN communication validation
- [ ] Engine testing (if applicable)
- [ ] Documentation and tuning guides

---

## 🔗 Related Projects

### rusEFI Ecosystem
- [rusEFI Firmware](https://github.com/rusefi/rusefi) - Main ECU firmware (STM32-based)
- [rusEFI Hardware](https://github.com/rusefi/rusefi/wiki/Hardware) - Official ECU boards
- [TunerStudio](https://www.tunerstudio.com/) - Tuning software

### Teensy Resources
- [PJRC Teensy 3.5](https://www.pjrc.com/store/teensy35.html) - Official product page
- [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html) - Arduino integration
- [Teensy Forum](https://forum.pjrc.com/) - Community support

### Alternative Firmware
- [Speeduino](https://speeduino.com/) - Arduino-based ECU with native Teensy 3.5 support
- [MegaSquirt](https://www.msextra.com/) - Commercial/DIY ECU firmware

---

## 🤝 Contributing

This project is in early planning stages. Contributions welcome!

**How to Help:**
1. Review the [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)
2. Open issues for architecture discussions
3. Submit PRs for HAL driver implementations
4. Test on real hardware and report results

**Coding Standards:**
- C++11 minimum (avoid exceptions/RTTI for embedded)
- Follow rusEFI naming conventions where applicable
- Document all public APIs with Doxygen comments
- Write unit tests for critical functions

---

## 📜 License

This project is licensed under **GPL v3** to maintain compatibility with rusEFI.

- See [LICENSE](LICENSE) file for details
- Derived from [rusEFI](https://github.com/rusefi/rusefi) (GPL v3)
- NO WARRANTY - Use at your own risk

---

## ⚠️ Safety Disclaimer

**THIS SOFTWARE IS INTENDED FOR EXPERIMENTAL USE ONLY**

- Engine control systems are safety-critical applications
- Improper calibration can cause engine damage or vehicle fires
- Always implement hardware safety interlocks (kill switches, pressure limits)
- Never test on public roads
- Have a backup ECU and fire extinguisher available
- Consult a professional before deploying on a real vehicle

**The authors and contributors assume NO LIABILITY for any damages.**

---

## 📞 Support & Community

- **GitHub Issues**: Bug reports and feature requests
- **rusEFI Forum**: [rusefi.com/forum](https://rusefi.com/forum/) (for general ECU questions)
- **Teensy Forum**: [forum.pjrc.com](https://forum.pjrc.com/) (for hardware questions)

---

## 🙏 Acknowledgments

- **rusEFI Team** - For the excellent open-source ECU firmware
- **PJRC** - For the Teensy platform and comprehensive documentation
- **@andreika** - For pioneering Kinetis support in rusEFI
- **Speeduino Community** - For proving Teensy viability in ECU applications

---

**Last Updated**: 2026-02-10
**Project Maintainer**: [@pbuchabqui](https://github.com/pbuchabqui)
**Branch**: `claude/plan-russefi-teensy-kGimY`