# Implementation Plan: rusEFI Complete System

**Goal:** Production-ready ECU with all rusEFI features
**Target:** Teensy 3.5 (MK64FX512)
**Timeline:** 8-10 weeks
**Current Version:** 2.4.0
**Target Version:** 3.0.0

---

## 📋 Implementation Phases

### **Phase 4: Main Control Loop & Hardware Integration**
**Duration:** 2 weeks
**Version:** 2.5.0
**Priority:** CRITICAL

#### Week 1: Main Control Loop & Sensors
- [ ] Main engine control loop (`engine_controller.c/h`)
  - [ ] Control loop state machine
  - [ ] Sensor reading system
  - [ ] Load calculation
  - [ ] Map table lookup system

- [ ] ADC sensor inputs (`sensor_adc.c/h`)
  - [ ] MAP sensor (manifold pressure)
  - [ ] TPS sensor (throttle position)
  - [ ] CLT sensor (coolant temperature)
  - [ ] IAT sensor (intake air temperature)
  - [ ] O2 sensor (oxygen/lambda)
  - [ ] Battery voltage sensing

- [ ] Sensor calibration system (`sensor_calibration.c/h`)
  - [ ] Linear calibration curves
  - [ ] Thermistor curves (Steinhart-Hart)
  - [ ] Voltage to physical units conversion

#### Week 2: Actuator Outputs & Integration
- [ ] Injector control (`injector_control.c/h`)
  - [ ] Pin configuration
  - [ ] On/off control functions
  - [ ] Bank configuration

- [ ] Ignition coil control (`ignition_control.c/h`)
  - [ ] Pin configuration
  - [ ] Charge/fire functions
  - [ ] Dwell time management

- [ ] Auxiliary outputs (`aux_outputs.c/h`)
  - [ ] Fuel pump control
  - [ ] IAC valve PWM
  - [ ] VVT solenoid PWM
  - [ ] Check engine light
  - [ ] Cooling fan control

**Deliverables:**
- Complete sensor input system
- Complete actuator output system
- Main control loop skeleton
- Basic engine operation possible

---

### **Phase 5: Fuel System**
**Duration:** 1 week
**Version:** 2.6.0
**Priority:** CRITICAL

#### Days 1-3: Fuel Calculation Core
- [ ] Fuel calculator (`fuel_calculator.c/h`)
  - [ ] VE (Volumetric Efficiency) tables
  - [ ] Air mass calculation (ideal gas law)
  - [ ] Fuel mass calculation (AFR target)
  - [ ] Injection duration calculation

- [ ] Injector characterization (`injector_data.c/h`)
  - [ ] Flow rate (cc/min)
  - [ ] Dead time compensation
  - [ ] Voltage correction
  - [ ] Non-linear flow correction

#### Days 4-5: Fuel Corrections
- [ ] Fuel corrections (`fuel_corrections.c/h`)
  - [ ] Coolant temperature correction
  - [ ] Intake air temperature correction
  - [ ] Battery voltage correction
  - [ ] Barometric pressure correction
  - [ ] Acceleration enrichment
  - [ ] Deceleration fuel cut

- [ ] AFR control (`afr_control.c/h`)
  - [ ] Target AFR tables (RPM x Load)
  - [ ] Open-loop AFR targets
  - [ ] Closed-loop preparation (O2 feedback)

**Deliverables:**
- Complete fuel calculation system
- Injection duration accurate to target AFR
- Temperature/voltage compensations working
- Engine can run (open-loop)

---

### **Phase 6: Timing System**
**Duration:** 1 week
**Version:** 2.7.0
**Priority:** CRITICAL

#### Days 1-3: Timing Calculation Core
- [ ] Timing calculator (`timing_calculator.c/h`)
  - [ ] Base timing tables (RPM x Load)
  - [ ] Spark angle calculation
  - [ ] Dwell time calculation
  - [ ] BTDC (Before Top Dead Center) conversion

- [ ] Coil characterization (`coil_data.c/h`)
  - [ ] Dwell time curves
  - [ ] Voltage compensation
  - [ ] Maximum dwell limits

#### Days 4-5: Timing Corrections
- [ ] Timing corrections (`timing_corrections.c/h`)
  - [ ] Coolant temperature correction
  - [ ] Acceleration timing advance
  - [ ] VVT timing correction
  - [ ] Knock timing retard (preparation)
  - [ ] Idle timing adjustment

- [ ] Timing safety (`timing_safety.c/h`)
  - [ ] Maximum advance limits
  - [ ] Minimum advance limits
  - [ ] Dwell safety limits

**Deliverables:**
- Complete timing calculation system
- Ignition timing accurate and safe
- Dwell time properly controlled
- Engine runs smoothly

---

### **Phase 7: Engine Control Features**
**Duration:** 1 week
**Version:** 2.8.0
**Priority:** HIGH

#### Days 1-2: Idle Control
- [ ] Idle controller (`idle_controller.c/h`)
  - [ ] Target idle RPM
  - [ ] PID controller implementation
  - [ ] IAC valve control
  - [ ] Idle timing adjustment
  - [ ] Cold start idle-up

#### Days 3-4: Closed-Loop Control
- [ ] O2 feedback (`o2_feedback.c/h`)
  - [ ] Lambda calculation
  - [ ] Closed-loop enable conditions
  - [ ] PID controller for AFR
  - [ ] Fuel trim (short-term + long-term)
  - [ ] Integration with fuel calculator

#### Day 5: Engine Modes
- [ ] Engine modes (`engine_modes.c/h`)
  - [ ] Cranking mode
  - [ ] Warm-up mode
  - [ ] Running mode
  - [ ] Decel fuel cut-off
  - [ ] Post-start enrichment

**Deliverables:**
- Stable idle control
- Closed-loop AFR control (when O2 sensor available)
- Smooth mode transitions
- Professional engine behavior

---

### **Phase 8: Safety Systems**
**Duration:** 3 days
**Version:** 2.9.0
**Priority:** HIGH

- [ ] Rev limiter (`rev_limiter.c/h`)
  - [ ] Hard cut (fuel/spark cut)
  - [ ] Soft cut (progressive)
  - [ ] Hysteresis

- [ ] Protection systems (`engine_protection.c/h`)
  - [ ] Over-boost protection
  - [ ] Overheat protection (coolant)
  - [ ] Low voltage protection
  - [ ] Low oil pressure (if sensor available)
  - [ ] Limp mode

- [ ] Fault detection (`fault_detection.c/h`)
  - [ ] Sensor fault detection (out of range)
  - [ ] Trigger signal fault detection
  - [ ] Actuator fault detection
  - [ ] Fault code storage (DTC)

**Deliverables:**
- Safe rev limiting
- Engine protection active
- Fault detection and logging
- Limp mode for critical failures

---

### **Phase 9: Phase 2 - Trigger Patterns**
**Duration:** 1 week
**Version:** 2.10.0
**Priority:** MEDIUM

- [ ] Trigger pattern framework (`trigger_patterns.c/h`)
  - [ ] Pattern definition structure
  - [ ] Pattern selection system
  - [ ] State machine framework

- [ ] Missing-tooth variants
  - [ ] 36-2, 36-2-2-2 (Subaru)
  - [ ] 60-2, 60-2-2-2
  - [ ] 12-1, 24-1

- [ ] Non-missing-tooth patterns
  - [ ] Honda K-series (12+1)
  - [ ] Nissan patterns (360 tooth, 4 tooth)
  - [ ] Mazda Miata NB (4+1)
  - [ ] GM 24x

- [ ] Dual-wheel patterns
  - [ ] Basic crank+cam patterns
  - [ ] Pattern verification

**Deliverables:**
- Support for 20+ common trigger patterns
- Pattern auto-detection (optional)
- Easy pattern switching
- Universal compatibility

---

### **Phase 10: Advanced Features Part 1**
**Duration:** 1 week
**Version:** 2.11.0
**Priority:** MEDIUM

#### Days 1-2: Knock Detection
- [ ] Knock detection (`knock_detector.c/h`)
  - [ ] Knock sensor input (ADC)
  - [ ] Knock window (angle-based)
  - [ ] Threshold detection
  - [ ] Knock counting
  - [ ] Timing retard on knock
  - [ ] Progressive recovery

#### Days 3-4: Boost Control
- [ ] Boost controller (`boost_control.c/h`)
  - [ ] Target boost table (RPM x TPS)
  - [ ] PID controller
  - [ ] Wastegate duty cycle control
  - [ ] Over-boost detection
  - [ ] Boost cut (safety)

#### Day 5: Launch Control
- [ ] Launch controller (`launch_control.c/h`)
  - [ ] Launch enable (clutch/button)
  - [ ] RPM limiting (two-step)
  - [ ] Timing retard (anti-lag)
  - [ ] Soft limiter (alternating spark cut)

**Deliverables:**
- Knock detection and retard
- Boost control for turbo engines
- Launch control (two-step)

---

### **Phase 11: Advanced Features Part 2**
**Duration:** 1 week
**Version:** 2.12.0
**Priority:** MEDIUM

#### Days 1-2: Traction Control
- [ ] Traction controller (`traction_control.c/h`)
  - [ ] Wheel speed inputs
  - [ ] Slip calculation
  - [ ] Progressive power reduction
  - [ ] Timing retard for traction

#### Days 3-4: Flex Fuel
- [ ] Flex fuel system (`flex_fuel.c/h`)
  - [ ] Flex sensor input (frequency)
  - [ ] Ethanol percentage calculation
  - [ ] Stoichiometric ratio adjustment
  - [ ] Timing correction for ethanol
  - [ ] Fuel table blending (E0/E85)

#### Day 5: Additional Features
- [ ] Nitrous control (`nitrous_control.c/h`)
  - [ ] Stage 1/2 activation
  - [ ] Timing retard on nitrous
  - [ ] Fuel enrichment on nitrous

- [ ] Water/meth injection (`water_meth.c/h`)
  - [ ] Activation based on boost/load
  - [ ] Progressive duty cycle

**Deliverables:**
- Traction control system
- Flex fuel support (E0-E85)
- Nitrous control
- Water/methanol injection

---

### **Phase 12: Communication & Tuning**
**Duration:** 1 week
**Version:** 2.13.0
**Priority:** MEDIUM

#### Days 1-3: CAN Bus
- [ ] CAN bus system (`can_bus.c/h`)
  - [ ] CAN driver (Teensy FlexCAN)
  - [ ] Message definitions
  - [ ] Transmit engine data
  - [ ] Receive commands
  - [ ] OBD-II protocol (basic)

#### Days 4-5: TunerStudio Protocol
- [ ] TunerStudio integration (`tunerstudio.c/h`)
  - [ ] Serial protocol implementation
  - [ ] INI file generation
  - [ ] Real-time data streaming
  - [ ] Table read/write
  - [ ] Configuration read/write
  - [ ] Data logging

**Deliverables:**
- CAN bus communication
- TunerStudio compatible
- Real-time tuning
- Data logging

---

### **Phase 13: Calibration & Testing**
**Duration:** 2 weeks
**Version:** 3.0.0
**Priority:** CRITICAL

#### Week 1: Calibration Data
- [ ] Engine configurations (`engine_configs/`)
  - [ ] 4-cylinder inline configuration
  - [ ] Default VE tables
  - [ ] Default timing tables
  - [ ] Default AFR targets
  - [ ] Sensor calibrations

- [ ] Base calibrations
  - [ ] Honda B-series example
  - [ ] Mazda Miata example
  - [ ] Generic 4-cylinder
  - [ ] Documentation for creating custom configs

#### Week 2: Testing & Validation
- [ ] Unit tests (`tests/`)
  - [ ] Fuel calculation tests
  - [ ] Timing calculation tests
  - [ ] Sensor calibration tests
  - [ ] Safety system tests

- [ ] Integration tests
  - [ ] Bench testing procedures
  - [ ] Signal simulation tests
  - [ ] Hardware-in-the-loop tests

- [ ] Documentation
  - [ ] Complete user manual
  - [ ] Tuning guide
  - [ ] Wiring diagrams
  - [ ] Safety procedures
  - [ ] Troubleshooting guide

**Deliverables:**
- Complete calibration system
- Tested and validated code
- Full documentation
- **Production-ready ECU v3.0.0**

---

## 📁 New File Structure

```
firmware/
├── src/
│   ├── controllers/
│   │   ├── engine_controller.c/h       [Phase 4] - Main control loop
│   │   ├── fuel_calculator.c/h         [Phase 5] - Fuel calculation
│   │   ├── fuel_corrections.c/h        [Phase 5] - Fuel corrections
│   │   ├── afr_control.c/h             [Phase 5] - AFR control
│   │   ├── timing_calculator.c/h       [Phase 6] - Timing calculation
│   │   ├── timing_corrections.c/h      [Phase 6] - Timing corrections
│   │   ├── timing_safety.c/h           [Phase 6] - Timing limits
│   │   ├── idle_controller.c/h         [Phase 7] - Idle control
│   │   ├── o2_feedback.c/h             [Phase 7] - Closed-loop
│   │   ├── engine_modes.c/h            [Phase 7] - Engine modes
│   │   ├── rev_limiter.c/h             [Phase 8] - Rev limiting
│   │   ├── engine_protection.c/h       [Phase 8] - Protections
│   │   ├── fault_detection.c/h         [Phase 8] - Fault codes
│   │   ├── trigger_patterns.c/h        [Phase 9] - Trigger patterns
│   │   ├── knock_detector.c/h          [Phase 10] - Knock detection
│   │   ├── boost_control.c/h           [Phase 10] - Boost control
│   │   ├── launch_control.c/h          [Phase 10] - Launch control
│   │   ├── traction_control.c/h        [Phase 11] - Traction control
│   │   ├── flex_fuel.c/h               [Phase 11] - Flex fuel
│   │   ├── nitrous_control.c/h         [Phase 11] - Nitrous
│   │   └── water_meth.c/h              [Phase 11] - Water/meth
│   │
│   ├── hal/
│   │   ├── sensor_adc.c/h              [Phase 4] - ADC sensors
│   │   ├── sensor_calibration.c/h      [Phase 4] - Calibrations
│   │   ├── injector_control.c/h        [Phase 4] - Injectors
│   │   ├── ignition_control.c/h        [Phase 4] - Ignition
│   │   ├── aux_outputs.c/h             [Phase 4] - Aux outputs
│   │   ├── can_bus.c/h                 [Phase 12] - CAN driver
│   │   └── tunerstudio.c/h             [Phase 12] - TunerStudio
│   │
│   ├── data/
│   │   ├── injector_data.c/h           [Phase 5] - Injector data
│   │   ├── coil_data.c/h               [Phase 6] - Coil data
│   │   └── engine_config.c/h           [Phase 13] - Engine config
│   │
│   └── config/
│       ├── engine_configs/             [Phase 13] - Config files
│       │   ├── honda_b_series.h
│       │   ├── mazda_miata.h
│       │   └── generic_4cyl.h
│       └── default_tables.c/h          [Phase 13] - Default tables
│
├── tests/                               [Phase 13] - Unit tests
│   ├── test_fuel.c
│   ├── test_timing.c
│   └── test_safety.c
│
└── examples/                            [Phase 13] - Example code
    ├── basic_4cyl/
    ├── turbo_4cyl/
    └── sequential_injection/
```

---

## 🎯 Success Criteria

### Phase 4-8 (Critical Path)
- ✅ Engine starts and runs
- ✅ Idle control stable (±50 RPM)
- ✅ Smooth acceleration/deceleration
- ✅ Closed-loop AFR control working
- ✅ Rev limiter functions correctly
- ✅ No safety issues

### Phase 9-11 (Advanced Features)
- ✅ Support for 20+ trigger patterns
- ✅ Knock detection functional
- ✅ Boost control holds target
- ✅ Launch control limits RPM
- ✅ Traction control reduces slip
- ✅ Flex fuel adjusts for ethanol

### Phase 12-13 (Integration)
- ✅ TunerStudio connects and tunes
- ✅ CAN bus transmits data
- ✅ Complete documentation
- ✅ Example configurations work
- ✅ All tests pass

---

## 📊 Timeline Overview

```
Week 1-2:  Phase 4  - Main Control & Hardware        [CRITICAL]
Week 3:    Phase 5  - Fuel System                    [CRITICAL]
Week 4:    Phase 6  - Timing System                  [CRITICAL]
Week 5:    Phase 7  - Engine Control Features        [HIGH]
Week 5:    Phase 8  - Safety Systems (3 days)        [HIGH]
Week 6:    Phase 9  - Trigger Patterns               [MEDIUM]
Week 7:    Phase 10 - Advanced Features Part 1       [MEDIUM]
Week 8:    Phase 11 - Advanced Features Part 2       [MEDIUM]
Week 9:    Phase 12 - Communication & Tuning         [MEDIUM]
Week 10-11: Phase 13 - Calibration & Testing         [CRITICAL]
```

**Total Duration:** 10-11 weeks
**Lines of Code (estimated):** +8,000-10,000 lines
**New Files:** ~45 files

---

## 🚀 Getting Started

### Immediate Next Steps:

1. **Start Phase 4 - Week 1: Main Control Loop & Sensors**
   - Implement main engine controller
   - Add ADC sensor inputs
   - Create sensor calibration system

2. **Review and approve this plan**
   - Adjust priorities if needed
   - Confirm feature set
   - Set realistic timeline

3. **Prepare development environment**
   - Set up testing hardware
   - Prepare sensor inputs
   - Configure actuator outputs

---

## 💡 Notes

- Each phase builds on previous phases
- Phases 4-8 are critical path (engine must run)
- Phases 9-12 add advanced features
- Phase 13 is polish and production readiness
- Testing is continuous throughout all phases

---

**Ready to start Phase 4?** 🚀
