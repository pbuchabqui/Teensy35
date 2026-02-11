# Russefi Teensy 3.5 Firmware

## Phase 1: Foundation (Bootable Firmware)

This directory contains the bare-metal firmware implementation for the Teensy 3.5 ECU.

### Current Status

✅ **Phase 1 Complete** - Minimal bootable firmware with:
- Clock initialization (120 MHz PLL)
- GPIO driver (LED control)
- UART driver (serial console output)
- SysTick timer (millisecond timing)
- Startup code and linker script

### Building the Firmware

#### Prerequisites

Install the ARM GCC toolchain:

```bash
# Ubuntu/Debian
sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi

# macOS
brew install gcc-arm-embedded

# Verify installation
arm-none-eabi-gcc --version
```

#### Compile

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build firmware
make

# View memory usage
arm-none-eabi-size russefi_teensy35.elf
```

#### Expected Output

```
Scanning dependencies of target russefi_teensy35.elf
[ 16%] Building C object CMakeFiles/russefi_teensy35.elf.dir/src/startup_mk64fx512.c.obj
[ 33%] Building C object CMakeFiles/russefi_teensy35.elf.dir/src/hal/clock_k64.c.obj
[ 50%] Building C object CMakeFiles/russefi_teensy35.elf.dir/src/hal/gpio_k64.c.obj
[ 66%] Building CXX object CMakeFiles/russefi_teensy35.elf.dir/src/hal/uart_k64.c.obj
[ 83%] Building CXX object CMakeFiles/russefi_teensy35.elf.dir/src/main.cpp.obj
[100%] Linking CXX executable russefi_teensy35.elf
   text    data     bss     dec     hex filename
   4532      24    8256   12812    320c russefi_teensy35.elf
```

### Flashing to Teensy 3.5

#### Using Teensy Loader CLI

```bash
# From build directory
teensy_loader_cli -mmcu=mk64fx512 -w russefi_teensy35.hex -v
```

#### Using Teensy Loader GUI

1. Open Teensy Loader application
2. Drag and drop `russefi_teensy35.hex` onto the window
3. Press the button on the Teensy 3.5 board
4. Firmware will be flashed automatically

### Testing the Firmware

#### LED Test

The onboard LED (Pin C5) should blink at 1 Hz (on for 500ms, off for 500ms).

#### Serial Console Test

Connect to the Teensy via USB and open a serial terminal:

```bash
# Linux
screen /dev/ttyACM0 115200

# macOS
screen /dev/tty.usbmodem* 115200

# Or use Arduino Serial Monitor (set to 115200 baud)
```

**Expected Output:**

```
========================================
   Russefi Teensy 3.5 ECU Firmware
========================================
Version: 0.1.0 (Phase 1)
Platform: Teensy 3.5 (MK64FX512)
CPU Speed: 120 MHz
License: GPL v3
========================================

System Information:
------------------
Processor: MK64FX512VMD12 (Kinetis K64)
Architecture: ARM Cortex-M4F @ 120 MHz
FPU: Single-precision (32-bit float)
Flash Memory: 512 KB
RAM: 256 KB
...

Initialization complete.
LED will blink at 1 Hz

Heartbeat: 1 seconds uptime
Heartbeat: 2 seconds uptime
Heartbeat: 3 seconds uptime
...
```

### Directory Structure

```
firmware/
├── CMakeLists.txt           # Build configuration
├── mk64fx512.ld             # Linker script for memory layout
├── README.md                # This file
├── src/
│   ├── startup_mk64fx512.c  # Startup code and vector table
│   ├── main.cpp             # Main application entry point
│   ├── hal/                 # Hardware Abstraction Layer
│   │   ├── clock_k64.c      # Clock initialization (120 MHz)
│   │   ├── clock_k64.h
│   │   ├── gpio_k64.c       # GPIO driver
│   │   ├── gpio_k64.h
│   │   ├── uart_k64.c       # UART serial driver
│   │   └── uart_k64.h
│   └── board/               # Board-specific configuration
│       └── board_pins.h     # Pin definitions
└── include/                 # Public headers (future use)
```

### Key Components

#### Startup Code (`startup_mk64fx512.c`)

- Defines interrupt vector table (101 interrupt vectors)
- Implements Reset_Handler that:
  - Copies `.data` from Flash to RAM
  - Zero-initializes `.bss` section
  - Calls C++ constructors
  - Jumps to `main()`

#### Linker Script (`mk64fx512.ld`)

- Defines memory regions:
  - Flash: 0x00000000 - 0x0007FFFF (512 KB)
  - RAM: 0x1FFF0000 - 0x2002FFFF (256 KB)
- Organizes sections: `.vectors`, `.text`, `.data`, `.bss`
- Allocates 16 KB stack, 8 KB heap

#### Clock Driver (`hal/clock_k64.c`)

- Configures MCG (Multipurpose Clock Generator) to:
  - Use 16 MHz external crystal
  - Enable PLL with 2 MHz input
  - Generate 120 MHz VCO output
  - Divide clocks for peripherals:
    - Core: 120 MHz
    - Bus: 60 MHz
    - FlexBus: 40 MHz
    - Flash: 24 MHz

#### GPIO Driver (`hal/gpio_k64.c`)

- Initializes GPIO ports (A-E)
- Configures pin direction (input/output)
- Provides functions for:
  - Set, clear, toggle pins
  - Read pin state

#### UART Driver (`hal/uart_k64.c`)

- Initializes UART0-5 for serial communication
- Calculates baud rate divisor based on bus clock
- Provides functions for:
  - Transmit byte/string
  - Receive byte
  - Check TX/RX ready status

### Memory Usage

Current firmware uses approximately:
- **Flash**: ~4.5 KB (< 1% of 512 KB)
- **RAM**: ~8 KB (< 4% of 256 KB)

Plenty of space for rusEFI control algorithms!

### Troubleshooting

#### Build Errors

**Error**: `arm-none-eabi-gcc: command not found`
- Install ARM GCC toolchain (see Prerequisites)

**Error**: `cannot find -lc`
- Install newlib: `sudo apt-get install libnewlib-arm-none-eabi`

#### Flashing Errors

**Error**: `Teensy did not respond`
- Press the physical button on Teensy 3.5 board
- Check USB cable connection

#### Runtime Errors

**LED not blinking**:
- Check that firmware flashed successfully
- Verify LED pin definition (Port C, Pin 5)
- Use oscilloscope/multimeter to check pin output

**No serial output**:
- Verify baud rate is 115200
- Wait 100ms after reset for UART to stabilize
- Check UART pin configuration (PTB16/PTB17 for UART0)

### Next Steps (Phase 2)

Upcoming features:
- [ ] ADC driver for analog sensor inputs
- [ ] PWM driver for injector/ignition outputs
- [ ] CAN bus interface
- [ ] Input capture for crank/cam sensors
- [ ] PIT timer for precise scheduling

### Contributing

When adding new HAL drivers:
1. Create header file in `src/hal/` (e.g., `adc_k64.h`)
2. Create implementation in `src/hal/` (e.g., `adc_k64.c`)
3. Add to `CMakeLists.txt` SOURCES list
4. Test thoroughly before committing

### References

- [MK64FX512 Reference Manual](https://www.pjrc.com/teensy/K64P144M120SF5RM.pdf)
- [Teensy 3.5 Schematic](https://www.pjrc.com/teensy/schematic35.png)
- [ARM Cortex-M4 Technical Reference](https://developer.arm.com/documentation/100166/0001)

---

**Last Updated**: 2026-02-10
**Phase**: 1 - Foundation Complete
