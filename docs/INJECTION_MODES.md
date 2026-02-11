# Injection Modes - rusEFI Compatible Implementation

## Overview

This document describes the four injection modes implemented in the Teensy 3.5 ECU firmware, following **ORIGINAL rusEFI** architecture and behavior.

**Version:** 2.2.0
**Date:** 2026-02-11
**Source:** [rusEFI Fuel Overview Wiki](https://wiki.rusefi.com/Fuel-Overview/)

---

## 🎯 Injection Modes

rusEFI supports four fuel injection strategies, each with different characteristics and use cases:

| Mode | Description | Fires per 720° | Best For |
|------|-------------|----------------|----------|
| **Sequential** | Each injector individually | 1× per cylinder | Performance, emissions |
| **Batch** | Injectors in pairs | 2× per pair | Reliability, simplicity |
| **Simultaneous** | All injectors together | 1× all cylinders | Cranking, simple setups |
| **Single Point** | One injector for all | Continuous | TBI systems |

---

## 1. Sequential Injection

### **Description**
Each injector fires **individually** at the optimal time in the engine cycle, typically during the intake stroke (180° before TDC).

### **Characteristics**
- ✅ **Best fuel economy** - precise per-cylinder metering
- ✅ **Best emissions** - optimal timing for combustion
- ✅ **Per-cylinder trim** - individual cylinder tuning possible
- ⚠️ **Requires cam sync** - must know exact cylinder position

### **Timing Diagram (4-cylinder, 1-3-4-2 firing order)**
```
Crank:    0°      180°    360°    540°    720°
          |-------|-------|-------|-------|
Cyl 1:    INJ                             INJ
Cyl 3:            INJ
Cyl 4:                    INJ
Cyl 2:                            INJ
```

### **Implementation**
```c
// Set sequential mode
ecu->fuel.injection_mode = INJECTION_MODE_SEQUENTIAL;

// Each cylinder fires once per 720° cycle
// Timing: cylinder * (720° / num_cylinders) - 180°
```

### **Use Cases**
- Modern engines with cam position sensor
- Performance applications requiring precise control
- Emissions-critical applications
- Variable cam timing (VVT) systems

---

## 2. Batch Injection

### **Description**
Injectors fire in **pairs**, with paired cylinders firing together. Each pair fires **twice per 720° cycle** (once per 360°).

### **Characteristics**
- ✅ **Good reliability** - simpler than sequential
- ✅ **Works without cam sync** - only needs crank position
- ✅ **Even fuel distribution** - pairs are 360° apart
- ⚠️ **Slightly less efficient** - timing not perfect for both cylinders

### **Timing Diagram (4-cylinder)**
```
Crank:    0°      180°    360°    540°    720°
          |-------|-------|-------|-------|
Pair 0:   INJ             INJ
(Cyl 1,4)

Pair 1:           INJ             INJ
(Cyl 3,2)
```

### **Pairing Strategy**
Cylinders are paired based on being **360° apart** in the cycle:

**4-Cylinder (1-3-4-2):**
- Pair 0: Cylinders 1 & 4 (360° apart)
- Pair 1: Cylinders 3 & 2 (360° apart)

**6-Cylinder (1-5-3-6-2-4):**
- Pair 0: Cylinders 1 & 6 (360° apart)
- Pair 1: Cylinders 5 & 2 (360° apart)
- Pair 2: Cylinders 3 & 4 (360° apart)

### **Implementation**
```c
// Set batch mode
ecu->fuel.injection_mode = INJECTION_MODE_BATCH;

// Initialize pairs (done automatically in ecu_init)
init_batch_injection_pairs(ecu);

// Pairs fire at: pair_index * (360° / num_pairs)
// Each pair fires twice: at 0° and 360° offset
```

### **Use Cases**
- Engines without cam position sensor
- Retrofit applications (OEM batch → aftermarket ECU)
- Intermediate step between simultaneous and sequential
- Wasted spark ignition systems (similar logic)

---

## 3. Simultaneous Injection

### **Description**
**All injectors fire together** at the same time, typically at a crank sync point.

### **Characteristics**
- ✅ **Maximum reliability** - simplest mode
- ✅ **Best for cranking** - ensures fuel availability
- ✅ **Works with minimal sensors** - only crank position needed
- ⚠️ **Least efficient** - timing not optimal for any cylinder

### **Timing Diagram (4-cylinder)**
```
Crank:    0°      180°    360°    540°    720°
          |-------|-------|-------|-------|
All Cyls: INJ                             INJ
(1,2,3,4)
```

### **Implementation**
```c
// Set simultaneous mode
ecu->fuel.injection_mode = INJECTION_MODE_SIMULTANEOUS;

// All injectors fire together at crank sync (0°)
```

### **Use Cases**
- **Cranking** (rusEFI recommendation)
- Emergency limp-home mode
- Initial startup before transitioning to sequential/batch
- Very simple engine setups
- Diagnostic/bench testing

### **rusEFI Recommendation**
> "For cranking, it is suggested to use 'Simultaneous' mode to ensure reliable engine starting."
>
> — [rusEFI Cranking Wiki](https://wiki.rusefi.com/Cranking/)

---

## 4. Single Point Injection (TBI)

### **Description**
**One injector** provides fuel for all cylinders, typically mounted in the throttle body.

### **Characteristics**
- ✅ **Simplest hardware** - one injector total
- ✅ **Lowest cost** - minimal wiring
- ⚠️ **Poor distribution** - relies on manifold mixing
- ⚠️ **Worst performance** - cannot tune per cylinder

### **Timing Diagram**
```
Crank:    0°      180°    360°    540°    720°
          |-------|-------|-------|-------|
Injector: ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
(TBI)     (Continuous duty cycle)
```

### **Implementation**
```c
// Set single point mode
ecu->fuel.injection_mode = INJECTION_MODE_SINGLE_POINT;

// Only injector 0 is used
// Fuel delivered based on duty cycle, not discrete pulses
```

### **Use Cases**
- Throttle Body Injection (TBI) systems
- Carbureted engine conversions (single injector)
- Low-budget projects
- Older vehicles (pre-port injection)

---

## 🔬 Technical Implementation

### **Mode Selection**
```c
typedef enum {
    INJECTION_MODE_SEQUENTIAL = 0,   // Each injector individually
    INJECTION_MODE_BATCH = 1,        // Paired injectors
    INJECTION_MODE_SIMULTANEOUS = 2, // All together
    INJECTION_MODE_SINGLE_POINT = 3  // One injector
} injection_mode_t;

// Set mode in ECU configuration
ecu->fuel.injection_mode = INJECTION_MODE_SEQUENTIAL;
```

### **Determining Which Injectors to Fire**
```c
// Get bitmask of injectors that should fire at current crank angle
uint8_t injector_mask = get_injectors_to_fire(ecu, crank_angle);

// Example: 0b00001010 means fire cylinders 1 and 3
for (uint8_t cyl = 0; cyl < num_cylinders; cyl++) {
    if (injector_mask & (1 << cyl)) {
        fire_injector(cyl, pulse_width_us);
    }
}
```

### **Mode-Specific Timing Calculation**
```c
float injection_angle = calculate_injection_timing_for_mode(ecu,
                                                           crank_angle,
                                                           cylinder);
```

---

## 📊 Performance Comparison

| Metric | Sequential | Batch | Simultaneous | Single Point |
|--------|------------|-------|--------------|--------------|
| **Fuel Economy** | ★★★★★ | ★★★★☆ | ★★★☆☆ | ★★☆☆☆ |
| **Emissions** | ★★★★★ | ★★★★☆ | ★★★☆☆ | ★★☆☆☆ |
| **Reliability** | ★★★☆☆ | ★★★★☆ | ★★★★★ | ★★★★★ |
| **Complexity** | ★★★★★ | ★★★☆☆ | ★★☆☆☆ | ★☆☆☆☆ |
| **Sensor Requirements** | Crank + Cam | Crank only | Crank only | Crank only |
| **Power** | Maximum | High | Medium | Low |

---

## 🚀 Usage Examples

### **Example 1: Cranking Sequence**
```c
// During cranking, use simultaneous for reliability
if (ecu->sensors.rpm < 400) {
    ecu->fuel.injection_mode = INJECTION_MODE_SIMULTANEOUS;
}
// After engine starts, switch to sequential
else if (ecu->sensors.sync_locked && ecu->sensors.rpm > 400) {
    ecu->fuel.injection_mode = INJECTION_MODE_SEQUENTIAL;
}
```

### **Example 2: Fallback Strategy**
```c
// Start with sequential (best performance)
ecu->fuel.injection_mode = INJECTION_MODE_SEQUENTIAL;

// If cam sync is lost, fall back to batch
if (!ecu->sensors.cam_synced) {
    ecu->fuel.injection_mode = INJECTION_MODE_BATCH;
}

// If crank sync is unstable, fall back to simultaneous
if (!ecu->sensors.sync_locked) {
    ecu->fuel.injection_mode = INJECTION_MODE_SIMULTANEOUS;
}
```

### **Example 3: 4-Cylinder Batch Pairing**
```c
// Firing order: 1-3-4-2 (common 4-cylinder)
// Engine config
ecu->config.num_cylinders = 4;
ecu->config.firing_order[0] = 0; // Cylinder 1
ecu->config.firing_order[1] = 2; // Cylinder 3
ecu->config.firing_order[2] = 3; // Cylinder 4
ecu->config.firing_order[3] = 1; // Cylinder 2

// Initialize batch pairs
init_batch_injection_pairs(ecu);

// Result:
// Pair 0: Cylinders 0 & 2 (1 & 4) - fire at 0° and 360°
// Pair 1: Cylinders 1 & 3 (3 & 2) - fire at 180° and 540°
```

---

## 🔧 Configuration Guide

### **TunerStudio Configuration**
In TunerStudio (when protocol is implemented):
1. Navigate to: **Settings → Engine Configuration → Injection**
2. Select **Injection Mode** dropdown:
   - Sequential (requires cam sync)
   - Batch (crank sync only)
   - Simultaneous (cranking/simple)
   - Single Point (TBI systems)
3. Click **Burn** to save

### **Code Configuration**
```c
// In main.cpp or configuration file
void configure_injection_mode(ecu_state_t* ecu) {
    // Set mode based on hardware capabilities
    if (has_cam_sensor()) {
        ecu->fuel.injection_mode = INJECTION_MODE_SEQUENTIAL;
    } else {
        ecu->fuel.injection_mode = INJECTION_MODE_BATCH;
    }

    // Initialize batch pairs if using batch mode
    if (ecu->fuel.injection_mode == INJECTION_MODE_BATCH) {
        init_batch_injection_pairs(ecu);
    }
}
```

---

## 📚 rusEFI Sources

### **Primary References**
- [rusEFI Fuel Overview](https://wiki.rusefi.com/Fuel-Overview/)
- [rusEFI Cranking Guide](https://wiki.rusefi.com/Cranking/)
- [rusEFI Forum: Injection Modes](https://www.rusefi.com/forum/viewtopic.php?t=1213)

### **Source Code**
- rusEFI GitHub: `firmware/controllers/scheduling/fuel_schedule.cpp`
- InjectionEvent class reference

### **Technical Papers**
- SAE 810494 - C. F. Aquino (transient fuel compensation applies to all modes)

---

## ⚠️ Important Notes

### **Cranking Recommendation**
rusEFI recommends **SIMULTANEOUS mode for cranking** to ensure reliable fuel delivery during engine start. After the engine starts and sync is achieved, transition to sequential or batch mode.

### **Cam Sync Requirement**
- **Sequential mode REQUIRES** cam position sensor for cylinder identification
- **Batch/Simultaneous modes** only need crank position sensor

### **Fuel Delivery**
- Sequential: 100% of fuel at optimal time
- Batch: 50% at optimal time + 50% during exhaust/compression
- Simultaneous: Timing varies by cylinder position

### **Duty Cycle Considerations**
At high RPM, batch and simultaneous modes may have **higher injector duty cycle** since fuel is delivered in fewer, larger events.

---

**Implementation Version:** 2.2.0
**Compatibility:** 100% rusEFI mode behavior
**Status:** Complete - All 4 modes implemented

**Last Updated:** 2026-02-11
**Maintainer:** rusEFI Teensy 3.5 Project
