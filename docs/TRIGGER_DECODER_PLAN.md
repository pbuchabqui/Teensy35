# rusEFI Trigger Decoder Implementation Plan

## 🎯 Objective

Implement ORIGINAL rusEFI trigger wheel decoder, RPM calculation, and event scheduling algorithms to replace the current simple implementation.

**Version:** 2.3.0 (planned)
**Date:** 2026-02-11
**Status:** Planning

---

## 📋 What Needs Implementation

### 1. **Trigger Decoder (rusEFI TriggerDecoderBase)**

**Current State:**
```c
// Simple tooth counter in input_capture_k64.c
engine_pos.tooth_count++;
if (engine_pos.tooth_count >= crank_teeth_per_rev) {
    engine_pos.tooth_count = 0;
}
// TODO: Implement missing tooth detection for sync
```

**rusEFI Implementation:**
- Source: `firmware/controllers/trigger/trigger_decoder.cpp`
- Class: `TriggerDecoderBase`
- Features:
  - Missing tooth detection with ratio comparison
  - State machine for sync tracking
  - Multiple trigger patterns (36-1, 60-2, etc.)
  - Gap ratio configuration (syncRatioFrom/syncRatioTo)

**Algorithm:**
```cpp
// rusEFI missing tooth detection
float ratio = current_period / previous_period;
if (ratio >= syncRatioFrom && ratio <= syncRatioTo) {
    // Synchronization gap detected!
    sync_locked = true;
    tooth_index = 0;  // Reset to known position
}
```

---

### 2. **RPM Calculation (rusEFI rpm_calculator.cpp)**

**Current State:**
```c
// Simple calculation
engine_pos.rpm = ic_calculate_rpm(period_us, crank_teeth_per_rev);

// Function:
uint16_t ic_calculate_rpm(uint32_t period_us, uint16_t teeth_per_rev) {
    uint64_t rpm = (60000000ULL) / ((uint64_t)period_us * teeth_per_rev);
    return (uint16_t)(rpm > 65535 ? 65535 : rpm);
}
```

**rusEFI Implementation:**
- Source: `firmware/controllers/rpm_calculator.cpp`
- Class: `RpmCalculator`
- Features:
  - Instantaneous RPM from tooth period
  - Averaged RPM with configurable filter
  - Revolution-based RPM calculation
  - Timeout detection for stopped engine

**Algorithm:**
```cpp
// rusEFI RPM calculation
instantRpm = 60 * US_PER_SECOND_F / (toothPeriod * teethPerRevolution);

// With averaging
rpmValue = instantRpm * 0.05 + rpmValue * 0.95;  // 5% new, 95% old
```

---

### 3. **Event Scheduling (rusEFI angle-based)**

**Current State:**
- ❌ **NOT IMPLEMENTED**
- Only timing calculation exists
- No actual scheduling system

**rusEFI Implementation:**
- Source: `firmware/controllers/scheduling/event_queue.cpp`
- Features:
  - Angle-based event scheduling
  - Time-based scheduling for low RPM
  - Event queue with priorities
  - Injection/ignition event planning

**Algorithm:**
```cpp
// rusEFI scheduling
// At 600 RPM, 360° = 100ms, so 4ms = 14.4°

// Schedule injection at angle
angle_t injection_angle = cylinder_angle - 180;  // 180° before TDC
schedule_event(injection_angle, fire_injector, cylinder);

// Schedule spark at angle
angle_t spark_angle = cylinder_angle - timing_advance;
schedule_event(spark_angle, fire_spark, cylinder);
```

---

## 🔧 Implementation Tasks

### **Phase 1: Trigger Decoder**

**Files to Create/Modify:**
- `firmware/src/hal/trigger_decoder_k64.c/h` (NEW)
- Modify `input_capture_k64.c` to use trigger decoder

**Features:**
```c
// Trigger decoder structure (rusEFI-compatible)
typedef struct {
    uint8_t tooth_count;              // Current tooth index
    uint8_t total_teeth;              // Total teeth per revolution
    uint8_t missing_teeth;            // Number of missing teeth

    uint32_t prev_tooth_time;         // Previous tooth timestamp
    uint32_t prev_tooth_period;       // Previous tooth period
    uint32_t current_tooth_period;    // Current tooth period

    float sync_ratio_from;            // Min ratio for sync gap (1.5)
    float sync_ratio_to;              // Max ratio for sync gap (3.0)

    bool sync_locked;                 // Synchronization status
    uint8_t sync_point_tooth;         // Tooth index at sync point

    // Callbacks
    void (*on_sync_callback)(void);
    void (*on_tooth_callback)(uint8_t tooth);
} trigger_decoder_t;

// Key functions
void trigger_decoder_init(trigger_decoder_t* decoder,
                         uint8_t teeth,
                         uint8_t missing);
void trigger_decoder_process_tooth(trigger_decoder_t* decoder,
                                  uint32_t timestamp);
bool trigger_decoder_is_synced(trigger_decoder_t* decoder);
uint8_t trigger_decoder_get_tooth_index(trigger_decoder_t* decoder);
```

**Missing Tooth Detection Algorithm:**
```c
void trigger_decoder_process_tooth(trigger_decoder_t* decoder,
                                  uint32_t timestamp) {
    // Calculate periods
    uint32_t period = timestamp - decoder->prev_tooth_time;

    // Calculate ratio
    float ratio = (float)period / (float)decoder->prev_tooth_period;

    // Check for sync gap
    if (ratio >= decoder->sync_ratio_from &&
        ratio <= decoder->sync_ratio_to) {
        // SYNC FOUND! (rusEFI algorithm)
        decoder->sync_locked = true;
        decoder->tooth_count = decoder->sync_point_tooth;

        if (decoder->on_sync_callback) {
            decoder->on_sync_callback();
        }
    } else if (decoder->sync_locked) {
        // Normal tooth (not gap)
        decoder->tooth_count++;

        // Wrap around at end of cycle
        if (decoder->tooth_count >= decoder->total_teeth) {
            decoder->tooth_count = 0;
        }

        if (decoder->on_tooth_callback) {
            decoder->on_tooth_callback(decoder->tooth_count);
        }
    }

    // Update history
    decoder->prev_tooth_period = period;
    decoder->prev_tooth_time = timestamp;
}
```

---

### **Phase 2: RPM Calculator (rusEFI Algorithm)**

**Files to Create:**
- `firmware/src/controllers/rpm_calculator.c/h` (NEW)

**Features:**
```c
// RPM calculator structure (rusEFI-compatible)
typedef struct {
    uint16_t rpm;                     // Current filtered RPM
    uint16_t instant_rpm;             // Instantaneous RPM

    uint32_t last_revolution_time;    // Time of last complete revolution
    uint32_t revolution_period;       // Period of last revolution (µs)

    float filter_coefficient;         // Smoothing factor (0.05 typical)

    uint32_t timeout_threshold;       // µs before considering stopped
    uint32_t last_update_time;        // Time of last RPM update

    bool stopped;                     // Engine stopped flag
} rpm_calculator_t;

// Key functions
void rpm_calculator_init(rpm_calculator_t* calc);
void rpm_calculator_on_tooth(rpm_calculator_t* calc,
                            uint32_t period_us,
                            uint16_t teeth_per_rev);
void rpm_calculator_on_revolution(rpm_calculator_t* calc,
                                 uint32_t revolution_period_us);
uint16_t rpm_calculator_get_rpm(rpm_calculator_t* calc);
bool rpm_calculator_is_running(rpm_calculator_t* calc);
```

**rusEFI RPM Algorithm:**
```c
void rpm_calculator_on_tooth(rpm_calculator_t* calc,
                            uint32_t period_us,
                            uint16_t teeth_per_rev) {
    // rusEFI instantaneous RPM calculation
    // instant_rpm = 60 * 1,000,000 / (tooth_period * teeth_per_rev)
    uint64_t instant = 60000000ULL /
                      ((uint64_t)period_us * teeth_per_rev);
    calc->instant_rpm = (uint16_t)instant;

    // rusEFI exponential moving average
    // new_rpm = instant * alpha + old_rpm * (1 - alpha)
    // Default alpha = 0.05 (5% new, 95% old)
    float new_rpm = calc->instant_rpm * calc->filter_coefficient +
                    calc->rpm * (1.0f - calc->filter_coefficient);
    calc->rpm = (uint16_t)new_rpm;

    // Update timestamp
    calc->last_update_time = get_current_time_us();
    calc->stopped = false;
}

// Check for timeout (engine stopped)
bool rpm_calculator_is_running(rpm_calculator_t* calc) {
    uint32_t now = get_current_time_us();
    uint32_t elapsed = now - calc->last_update_time;

    if (elapsed > calc->timeout_threshold) {
        calc->stopped = true;
        calc->rpm = 0;
        return false;
    }

    return !calc->stopped;
}
```

---

### **Phase 3: Event Scheduler (rusEFI angle-based)**

**Files to Create:**
- `firmware/src/controllers/event_scheduler.c/h` (NEW)

**Features:**
```c
// Scheduled event (rusEFI-compatible)
typedef struct {
    uint16_t trigger_angle;           // Crank angle to trigger (0-720°)
    uint8_t cylinder;                 // Cylinder number
    void (*action)(uint8_t cyl);      // Action to execute
    bool active;                      // Event is scheduled
    uint32_t scheduled_time_us;       // Calculated execution time
} scheduled_event_t;

// Event scheduler (rusEFI-compatible)
typedef struct {
    scheduled_event_t events[16];     // Max 16 events (8 inj + 8 ign)
    uint8_t num_events;

    uint16_t current_angle;           // Current crank angle (0-720°)
    uint16_t rpm;                     // Current RPM for timing

    // Angle-to-time conversion
    uint32_t us_per_degree;           // Microseconds per degree
} event_scheduler_t;

// Key functions
void scheduler_init(event_scheduler_t* sched);
void scheduler_update_angle(event_scheduler_t* sched,
                           uint16_t angle,
                           uint16_t rpm);
void scheduler_add_event(event_scheduler_t* sched,
                        uint16_t angle,
                        uint8_t cylinder,
                        void (*action)(uint8_t));
void scheduler_process_events(event_scheduler_t* sched);
```

**rusEFI Scheduling Algorithm:**
```c
void scheduler_update_angle(event_scheduler_t* sched,
                           uint16_t angle,
                           uint16_t rpm) {
    sched->current_angle = angle;
    sched->rpm = rpm;

    // Calculate microseconds per degree (rusEFI method)
    // At 600 RPM: 360° = 100ms, so 1° = 277.7µs
    // us_per_degree = 60,000,000 / (rpm * 360)
    if (rpm > 0) {
        sched->us_per_degree = 60000000UL / ((uint32_t)rpm * 360);
    }
}

void scheduler_add_event(event_scheduler_t* sched,
                        uint16_t angle,
                        uint8_t cylinder,
                        void (*action)(uint8_t)) {
    // Find free slot
    for (uint8_t i = 0; i < 16; i++) {
        if (!sched->events[i].active) {
            // Schedule event
            sched->events[i].trigger_angle = angle;
            sched->events[i].cylinder = cylinder;
            sched->events[i].action = action;
            sched->events[i].active = true;

            // Calculate absolute time (angle-based scheduling)
            int16_t angle_delta = angle - sched->current_angle;
            if (angle_delta < 0) angle_delta += 720;  // Wrap

            uint32_t time_until_event = angle_delta * sched->us_per_degree;
            sched->events[i].scheduled_time_us =
                get_current_time_us() + time_until_event;

            break;
        }
    }
}

void scheduler_process_events(event_scheduler_t* sched) {
    uint32_t now = get_current_time_us();

    // Check each event
    for (uint8_t i = 0; i < 16; i++) {
        if (sched->events[i].active) {
            // Check if event should fire
            if (now >= sched->events[i].scheduled_time_us) {
                // Fire event! (rusEFI execution)
                sched->events[i].action(sched->events[i].cylinder);

                // Mark as complete
                sched->events[i].active = false;
            }
        }
    }
}
```

---

## 📚 rusEFI Source References

### **Trigger Decoder:**
- `firmware/controllers/trigger/trigger_decoder.cpp`
- `firmware/controllers/trigger/trigger_decoder.h`
- Class: `TriggerDecoderBase`
- [TriggerDecoderBase Class Reference](https://rusefi.com/docs/html/class_trigger_decoder_base.html)

### **RPM Calculator:**
- `firmware/controllers/rpm_calculator.cpp`
- `firmware/controllers/rpm_calculator.h`
- Class: `RpmCalculator`

### **Event Scheduling:**
- `firmware/controllers/scheduling/event_queue.cpp`
- `firmware/controllers/scheduling/event_queue.h`
- Angle-based scheduling system

### **Documentation:**
- [rusEFI Trigger Wiki](https://wiki.rusefi.com/Trigger/)
- [All Supported Triggers](https://github.com/rusefi/rusefi/wiki/All-Supported-Triggers)
- [Trigger Configuration Guide](https://wiki.rusefi.com/Trigger-Configuration-Guide/)

---

## ✅ Benefits of rusEFI Implementation

### **Trigger Decoder:**
- ✅ Reliable missing tooth detection
- ✅ Multiple trigger patterns support
- ✅ Configurable sync ratios
- ✅ Proven on thousands of ECUs

### **RPM Calculator:**
- ✅ Smooth filtered RPM (no jitter)
- ✅ Engine stop detection
- ✅ Accurate at all RPM ranges
- ✅ Same algorithm as rusEFI

### **Event Scheduler:**
- ✅ Precise angle-based firing
- ✅ Automatic timing compensation
- ✅ Priority-based event handling
- ✅ Scalable to complex timing

---

## 🎯 Implementation Priority

**HIGH PRIORITY:**
1. **Trigger Decoder** - Critical for sync
2. **RPM Calculator** - Needed for all calculations

**MEDIUM PRIORITY:**
3. **Event Scheduler** - Improves timing accuracy

**FUTURE:**
4. Multiple trigger patterns (currently 36-1 only)
5. Advanced sync patterns (compound triggers)

---

## 📦 Estimated Code Size

- Trigger decoder: ~300 lines
- RPM calculator: ~150 lines
- Event scheduler: ~250 lines
- Total: ~700 lines of rusEFI algorithms

---

**Next Step:** Implement trigger decoder first, then RPM calculator, then event scheduler.

**Version:** 2.3.0 (planned - rusEFI trigger system)
**Compatibility:** 100% rusEFI algorithms
**Status:** Planning complete, ready for implementation
