# Getting Started with Russefi Teensy 3.5

**Quick reference guide for developers starting work on this project**

---

## 📋 Prerequisites

### Required Hardware
- Teensy 3.5 development board
- USB cable (micro-B)
- Computer running Linux, macOS, or Windows

### Required Software

1. **ARM GCC Toolchain**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install gcc-arm-none-eabi

   # macOS (Homebrew)
   brew install gcc-arm-embedded

   # Windows
   # Download from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
   ```

2. **CMake** (3.15 or later)
   ```bash
   # Ubuntu/Debian
   sudo apt-get install cmake

   # macOS
   brew install cmake

   # Windows
   # Download from: https://cmake.org/download/
   ```

3. **Teensy Loader CLI**
   ```bash
   # Linux
   wget https://www.pjrc.com/teensy/teensy_loader_cli_linux.tar.gz
   tar -xzf teensy_loader_cli_linux.tar.gz
   sudo cp teensy_loader_cli /usr/local/bin/

   # macOS/Windows
   # Download from: https://www.pjrc.com/teensy/loader_cli.html
   ```

4. **Git** (for version control)
   ```bash
   sudo apt-get install git  # Linux
   brew install git          # macOS
   ```

---

## 🚀 Quick Start (5 Minutes)

### 1. Clone the Repository

```bash
git clone https://github.com/pbuchabqui/Teensy35.git
cd Teensy35
```

### 2. Read the Implementation Plan

```bash
# Review the comprehensive planning document
cat IMPLEMENTATION_PLAN.md

# Or view the hardware pinout
cat hardware/pinout/teensy35_ecu_pinout.md
```

### 3. Set Up Development Environment

```bash
# Verify ARM GCC is installed
arm-none-eabi-gcc --version
# Expected: gcc version 10.3 or later

# Verify CMake is installed
cmake --version
# Expected: cmake version 3.15 or later

# Verify Teensy Loader is installed
teensy_loader_cli --help
```

### 4. Build the Firmware (When Ready)

**Note**: The current repository is in the planning phase. The following steps will work once Phase 1 development begins.

```bash
# Create build directory
mkdir build
cd build

# Configure build system
cmake ../firmware

# Compile firmware
make

# View memory usage
arm-none-eabi-size russefi_teensy35.elf
```

### 5. Flash to Teensy 3.5

```bash
# From the build/ directory
make flash

# Or manually with teensy_loader_cli
teensy_loader_cli -mmcu=mk64fx512 -w russefi_teensy35.hex -v
```

### 6. Verify Firmware is Running

```bash
# Open serial monitor (115200 baud)
# Linux/macOS:
screen /dev/ttyACM0 115200

# Or use Arduino Serial Monitor
# Tools → Serial Monitor (set to 115200 baud)
```

**Expected Output**:
```
========================================
   Russefi Teensy 3.5 ECU Firmware
========================================
Version: 0.1.0 (Planning Phase)
Platform: Teensy 3.5 (MK64FX512)
CPU Speed: 120 MHz
License: GPL v3
========================================

System Information:
------------------
Processor: MK64FX512VMD12 (Kinetis K64)
Architecture: ARM Cortex-M4F @ 120 MHz
...
```

---

## 📚 Project Structure Overview

```
Teensy35/
├── IMPLEMENTATION_PLAN.md       # Master development roadmap
├── README.md                     # Project overview
├── LICENSE                       # GPL v3 license
│
├── firmware/                     # ECU firmware source
│   ├── CMakeLists.txt           # Build configuration
│   ├── src/
│   │   ├── main.cpp             # Main program entry
│   │   ├── hal/                 # Hardware abstraction layer
│   │   ├── board/               # Teensy 3.5 config
│   │   └── controllers/         # Engine control logic
│   └── include/                 # Header files
│
├── hardware/                     # Schematics and wiring
│   ├── schematics/              # KiCad PCB designs
│   ├── pinout/                  # Pin mapping docs
│   │   └── teensy35_ecu_pinout.md
│   └── datasheets/              # Component datasheets
│
├── docs/                         # Documentation
│   ├── GETTING_STARTED.md       # This file
│   ├── BUILDING.md              # (Coming soon)
│   └── WIRING.md                # (Coming soon)
│
├── tools/                        # Build scripts
│   └── flash_teensy.sh          # (Coming soon)
│
└── tests/                        # Unit tests
    └── unit_tests/              # (Coming soon)
```

---

## 🔧 Development Workflow

### Understanding the Phases

The project is divided into 4 phases (see `IMPLEMENTATION_PLAN.md`):

- **Phase 1** (Weeks 1-3): Basic bootable firmware
- **Phase 2** (Weeks 4-7): Peripheral drivers (ADC, PWM, CAN)
- **Phase 3** (Weeks 8-12): Engine control integration
- **Phase 4** (Weeks 13-16): Testing and validation

### Current Status: Planning Phase

We are currently in the **planning and architecture design** stage. Key activities:

1. ✅ Research rusEFI firmware architecture
2. ✅ Analyze Teensy 3.5 hardware capabilities
3. ✅ Create comprehensive implementation plan
4. ✅ Define pin mappings and interfaces
5. ⏳ Set up build system (in progress)
6. ⏳ Create basic bootable firmware (next step)

### Contributing to Development

1. **Pick a Task**: Review open issues on GitHub or consult `IMPLEMENTATION_PLAN.md`

2. **Create a Branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make Changes**: Edit source files, test locally

4. **Commit with Meaningful Messages**:
   ```bash
   git add .
   git commit -m "Add ADC driver for Kinetis K64"
   ```

5. **Push and Create Pull Request**:
   ```bash
   git push origin feature/your-feature-name
   # Then create PR on GitHub
   ```

---

## 🐛 Troubleshooting

### Build Errors

**Problem**: `arm-none-eabi-gcc: command not found`
**Solution**: Install ARM GCC toolchain (see Prerequisites above)

**Problem**: `CMake Error: The source directory does not appear to contain CMakeLists.txt`
**Solution**: Make sure you're running `cmake ../firmware` from the `build/` directory

**Problem**: `/usr/bin/ld: cannot find -lc`
**Solution**: Install newlib for ARM:
```bash
sudo apt-get install libnewlib-arm-none-eabi
```

### Flashing Errors

**Problem**: `Teensy did not respond to request`
**Solution**:
1. Press the physical button on Teensy 3.5 board
2. Verify USB cable is connected
3. Try a different USB port

**Problem**: `Permission denied: /dev/ttyACM0`
**Solution**: Add your user to the `dialout` group:
```bash
sudo usermod -a -G dialout $USER
# Log out and log back in
```

### Serial Monitor Issues

**Problem**: No output on serial monitor
**Solution**:
1. Verify baud rate is set to 115200
2. Wait 3 seconds after reset (USB enumeration delay)
3. Try pressing the reset button on Teensy

---

## 🔗 Useful Resources

### Teensy 3.5 Documentation
- [Official Product Page](https://www.pjrc.com/store/teensy35.html)
- [Pinout Card (PDF)](https://www.pjrc.com/teensy/card7a_rev2.pdf)
- [MK64FX512 Reference Manual (14MB PDF)](https://www.pjrc.com/teensy/K64P144M120SF5RM.pdf)
- [Teensy Forum](https://forum.pjrc.com/)

### rusEFI Documentation
- [GitHub Repository](https://github.com/rusefi/rusefi)
- [Wiki Home](https://github.com/rusefi/rusefi/wiki/)
- [Custom Firmware Guide](https://github.com/rusefi/rusefi/wiki/Custom-Firmware)
- [Community Forum](https://rusefi.com/forum/)

### Development Tools
- [ARM GCC Documentation](https://gcc.gnu.org/onlinedocs/)
- [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/)
- [Git Cheat Sheet](https://education.github.com/git-cheat-sheet-education.pdf)

### ECU Theory
- [Engine Management: Advanced Tuning (Book)](https://www.amazon.com/Engine-Management-Advanced-Tuning-Hartman/dp/0977961311)
- [TunerStudio Manual](https://www.tunerstudio.com/index.php/manuals)
- [MegaSquirt Documentation](http://www.msextra.com/doc/)

---

## 📞 Getting Help

### Documentation First
1. Read `IMPLEMENTATION_PLAN.md` thoroughly
2. Check hardware pinout: `hardware/pinout/teensy35_ecu_pinout.md`
3. Review existing code comments

### Ask Questions
- **GitHub Issues**: For bugs or feature requests
- **rusEFI Forum**: For general ECU questions
- **Teensy Forum**: For Teensy-specific hardware questions

### Provide Context When Asking
- What you're trying to accomplish
- What you've already tried
- Error messages (full output)
- Hardware setup (schematic if applicable)

---

## ⚠️ Safety Reminders

Before working with real engine hardware:

- ✅ Implement safety interlocks (kill switch, pressure limits)
- ✅ Test on bench equipment first
- ✅ Have fire extinguisher available
- ✅ Never test on public roads
- ✅ Use proper automotive wiring and connectors
- ✅ Protect against overvoltage (TVS diodes, fuses)
- ✅ Ensure proper grounding to prevent electrical noise

**This is experimental software. Always have a backup ECU available.**

---

## 🎯 Next Steps

Ready to contribute? Here's what to work on:

### Phase 1 Tasks (Current Priority)
1. Set up Teensy core library integration
2. Create linker script for MK64FX512 memory map
3. Implement basic GPIO driver (LED blink test)
4. Implement UART driver (serial console)
5. Verify system clock configuration (120 MHz)

### Learning Resources
- Read Chapter 11-13 of MK64FX512 Reference Manual (ADC)
- Study rusEFI HAL structure in `firmware/hw_layer/`
- Review Teensy 3.5 example projects on PJRC forums

---

**Document Version**: 1.0
**Last Updated**: 2026-02-10
**Maintained By**: Teensy35 Russefi Port Team

Happy coding! 🚗⚡
