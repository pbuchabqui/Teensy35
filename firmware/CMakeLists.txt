###############################################################################
# Russefi Teensy 3.5 ECU - CMake Build Configuration
###############################################################################
#
# This CMakeLists.txt file configures the build system for compiling rusEFI
# firmware for the Teensy 3.5 platform (Kinetis MK64FX512).
#
# Prerequisites:
#   - CMake 3.15 or later
#   - ARM GCC toolchain (gcc-arm-none-eabi)
#   - Teensy Loader CLI (for flashing)
#
# Build Instructions:
#   mkdir build && cd build
#   cmake ..
#   make
#
# Flash to Teensy:
#   teensy_loader_cli -mmcu=mk64fx512 -w russefi_teensy35.hex
#
###############################################################################

cmake_minimum_required(VERSION 3.15)

# Project metadata
project(russefi_teensy35
    VERSION 0.1.0
    DESCRIPTION "Russefi ECU Firmware Port for Teensy 3.5"
    LANGUAGES C CXX ASM
)

###############################################################################
# Toolchain Configuration
###############################################################################

# Teensy 3.5 target specifications
set(MCU "mk64fx512")
set(CPU_TYPE "cortex-m4")
set(FPU_TYPE "fpv4-sp-d16")
set(FLOAT_ABI "hard")

# ARM GCC toolchain paths (adjust if needed)
set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")
set(CMAKE_ASM_COMPILER "arm-none-eabi-gcc")
set(CMAKE_OBJCOPY "arm-none-eabi-objcopy")
set(CMAKE_SIZE "arm-none-eabi-size")

# Cross-compilation settings
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)
set(CMAKE_CROSSCOMPILING TRUE)

# Prevent CMake from testing the compiler (not applicable for bare-metal)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

###############################################################################
# Compiler Flags
###############################################################################

# CPU-specific flags
set(CPU_FLAGS "-mcpu=${CPU_TYPE} -mthumb -mfloat-abi=${FLOAT_ABI} -mfpu=${FPU_TYPE}")

# Common C/C++ flags
set(COMMON_FLAGS
    "${CPU_FLAGS}"
    "-Wall"                     # Enable all warnings
    "-Wextra"                   # Enable extra warnings
    "-ffunction-sections"       # Place functions in separate sections
    "-fdata-sections"           # Place data in separate sections
    "-fno-common"               # Prevent common variables
    "-fno-builtin"              # Don't use built-in functions
)

# C-specific flags
set(CMAKE_C_FLAGS "${COMMON_FLAGS} -std=gnu11")

# C++-specific flags
set(CMAKE_CXX_FLAGS
    "${COMMON_FLAGS}"
    "-std=gnu++11"              # C++11 standard
    "-fno-exceptions"           # Disable C++ exceptions (saves flash)
    "-fno-rtti"                 # Disable run-time type info (saves flash)
    "-fno-threadsafe-statics"   # Not needed in single-threaded environment
)

# Optimization flags (change to -O0 -g for debugging)
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -DDEBUG")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS
    "${CPU_FLAGS}"
    "-Wl,--gc-sections"         # Remove unused sections
    "-Wl,--print-memory-usage"  # Display memory usage
    "-specs=nano.specs"         # Use newlib-nano (smaller C library)
    "-lm"                       # Link math library
    "-lc"                       # Link C library
)

# TODO: Add linker script when available
# set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/mk64fx512.ld")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${LINKER_SCRIPT}")

###############################################################################
# Preprocessor Definitions
###############################################################################

add_compile_definitions(
    # Teensy 3.5 identifier
    __MK64FX512__
    TEENSY35

    # Clock configuration
    F_CPU=120000000             # 120 MHz CPU frequency
    F_BUS=60000000              # 60 MHz bus frequency

    # USB configuration (for TunerStudio communication)
    USB_SERIAL

    # Build configuration
    ARDUINO=10813               # Arduino core version compatibility
)

###############################################################################
# Include Directories
###############################################################################

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/hal
    ${CMAKE_SOURCE_DIR}/src/board
    ${CMAKE_SOURCE_DIR}/src/controllers

    # TODO: Add Teensy core library paths when integrating
    # /path/to/teensy/cores/teensy3
    # /path/to/teensy/libraries
)

###############################################################################
# Source Files
###############################################################################

# Main application sources
set(SOURCES
    src/main.cpp

    # TODO: Add HAL driver sources
    # src/hal/adc_k64.cpp
    # src/hal/can_k64.cpp
    # src/hal/gpio_k64.cpp
    # src/hal/pwm_k64.cpp
    # src/hal/uart_k64.cpp

    # TODO: Add board configuration
    # src/board/board_config.cpp
    # src/board/board_pins.cpp

    # TODO: Add rusEFI controller sources
    # src/controllers/fuel_injection.cpp
    # src/controllers/ignition_timing.cpp
    # src/controllers/idle_control.cpp
)

###############################################################################
# Build Targets
###############################################################################

# Main executable
add_executable(${PROJECT_NAME}.elf ${SOURCES})

# Generate HEX file for Teensy Loader
add_custom_command(
    TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
        ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMENT "Generating HEX file for Teensy Loader"
)

# Generate binary file (alternative format)
add_custom_command(
    TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
        ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMENT "Generating binary file"
)

# Display memory usage
add_custom_command(
    TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} --format=berkeley ${PROJECT_NAME}.elf
    COMMENT "Memory usage summary:"
)

###############################################################################
# Flash Target (requires teensy_loader_cli)
###############################################################################

add_custom_target(flash
    COMMAND teensy_loader_cli -mmcu=${MCU} -w ${PROJECT_NAME}.hex -v
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Flashing firmware to Teensy 3.5 via USB"
)

###############################################################################
# Clean Target
###############################################################################

set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES
    "${PROJECT_NAME}.hex;${PROJECT_NAME}.bin"
)

###############################################################################
# Build Information
###############################################################################

message(STATUS "========================================")
message(STATUS "  Russefi Teensy 3.5 ECU Build Config")
message(STATUS "========================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "MCU: ${MCU}")
message(STATUS "CPU: ${CPU_TYPE} @ 120 MHz")
message(STATUS "FPU: ${FPU_TYPE} (${FLOAT_ABI} float ABI)")
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")
