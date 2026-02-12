# Integration Guide: Phase 1 Features

**Version:** 2.3.1
**Date:** 2026-02-12
**Status:** Production Ready

---

## 📋 Overview

This guide shows how to integrate Phase 1 features into your existing injection and ignition systems.

**What you'll integrate:**
1. Hardware Timer Scheduler
2. Multi-Stage Events (injection/ignition)
3. Cam Sync (sequential injection)

**Time required:** 2-4 hours

---

## 🔧 Step 1: Setup Hardware Scheduler

### 1.1 Include Headers

```c
// In your main.c or engine_control.c
#include "hardware_scheduler_k64.h"
#include "event_scheduler.h"
#include "multi_stage_scheduler.h"
#include "cam_sync_k64.h"
```

### 1.2 Declare Global Instances

```c
// Global scheduler instances
static event_scheduler_t angle_scheduler;
static multistage_scheduler_t ms_scheduler;
static cam_sync_state_t cam_sync;
```

### 1.3 Initialize in setup()

```c
void setup() {
    // ... existing initialization ...

    // Initialize angle-based scheduler
    scheduler_init(&angle_scheduler);

    // Initialize multi-stage scheduler
    multistage_scheduler_init(&ms_scheduler, &angle_scheduler);

    // Initialize cam sync
    cam_sync_init(&cam_sync);

    // ... rest of setup ...
}
```

---

## ⚙️ Step 2: Integrate with Crank Sensor

### 2.1 Update Crank Sensor Callback

Modify your crank sensor interrupt to update scheduler angle:

```c
static void crank_sensor_callback(uint32_t timestamp) {
    // Process tooth through rusEFI trigger decoder
    trigger_decoder_process_tooth(&crank_decoder, timestamp);

    // Update engine position from decoder
    engine_pos.sync_locked = trigger_decoder_is_synced(&crank_decoder);

    if (engine_pos.sync_locked) {
        // Get tooth index and period
        uint8_t tooth = trigger_decoder_get_tooth_index(&crank_decoder);
        uint32_t period_us = trigger_decoder_get_tooth_period(&crank_decoder);

        // Calculate RPM
        rpm_calculator_on_tooth(&rpm_calc, period_us, crank_teeth_per_rev, timestamp);
        uint16_t rpm = rpm_calculator_get_rpm(&rpm_calc);

        // Calculate current crank angle (0-360°)
        // For 36-1 wheel: angle = (tooth / 36) * 360
        uint16_t crank_angle = (tooth * 360) / crank_teeth_per_rev;

        // NEW: Update angle scheduler with current angle and RPM
        scheduler_update_angle(&angle_scheduler,
                              crank_angle,
                              rpm,
                              timestamp);
    }
}
```

### 2.2 Process Cam Sensor (if available)

```c
static void cam_sensor_callback(uint32_t timestamp) {
    // Read cam sensor digital input
    bool cam_signal = digitalRead(CAM_SENSOR_PIN);

    // Get current crank tooth
    uint8_t crank_tooth = trigger_decoder_get_tooth_index(&crank_decoder);

    // Process cam event
    cam_sync_process_event(&cam_sync,
                          cam_signal,
                          crank_tooth,
                          timestamp);

    // Check if we just synced
    if (cam_sync_is_synced(&cam_sync)) {
        // Log or callback
        Serial.println("Cam synced!");
    }
}
```

---

## 💉 Step 3: Integrate Injection Control

### 3.1 Injection Callback Functions

Define your injector control functions:

```c
// Injector control (adapt to your hardware)
void open_injector_0(uint8_t cyl) { digitalWrite(INJ_PIN_0, HIGH); }
void close_injector_0(uint8_t cyl) { digitalWrite(INJ_PIN_0, LOW); }

void open_injector_1(uint8_t cyl) { digitalWrite(INJ_PIN_1, HIGH); }
void close_injector_1(uint8_t cyl) { digitalWrite(INJ_PIN_1, LOW); }

// ... repeat for all cylinders ...
```

### 3.2 Schedule Injection Events

Replace your old injection timing code with:

```c
void schedule_injection_events(void) {
    uint32_t current_time = micros();
    uint16_t rpm = get_engine_rpm();

    // Don't schedule if RPM too low or not synced
    if (rpm < 100 || !trigger_decoder_is_synced(&crank_decoder)) {
        return;
    }

    // Calculate injection duration based on engine load
    uint32_t inj_duration_us = calculate_injection_duration(rpm, load, map);

    // For each cylinder
    for (uint8_t cyl = 0; cyl < 4; cyl++) {
        // Get cylinder's TDC angle
        uint16_t tdc_angle = get_cylinder_tdc_angle(cyl);

        // If cam synced, use full cycle angle (0-720°)
        if (cam_sync_is_synced(&cam_sync)) {
            tdc_angle = cam_sync_get_full_cycle_angle(&cam_sync, tdc_angle);
        }

        // Schedule injection 180° before TDC (intake stroke)
        uint16_t inj_angle = (tdc_angle + 720 - 180) % 720;

        // Schedule multi-stage injection (open + close)
        multistage_schedule_injection(&ms_scheduler,
                                     cyl,
                                     inj_angle,
                                     inj_duration_us,
                                     get_injector_open_callback(cyl),
                                     get_injector_close_callback(cyl),
                                     rpm,
                                     current_time);
    }
}
```

### 3.3 Helper Functions

```c
// Get injector callbacks per cylinder
static void (*get_injector_open_callback(uint8_t cyl))(uint8_t) {
    switch(cyl) {
        case 0: return open_injector_0;
        case 1: return open_injector_1;
        case 2: return open_injector_2;
        case 3: return open_injector_3;
        default: return NULL;
    }
}

static void (*get_injector_close_callback(uint8_t cyl))(uint8_t) {
    switch(cyl) {
        case 0: return close_injector_0;
        case 1: return close_injector_1;
        case 2: return close_injector_2;
        case 3: return close_injector_3;
        default: return NULL;
    }
}

// Calculate injection duration (your existing formula)
static uint32_t calculate_injection_duration(uint16_t rpm, uint8_t load, uint16_t map) {
    // Your fuel calculation here
    // Example: base duration + corrections
    uint32_t base_duration = 2000;  // 2ms base
    uint32_t duration = base_duration * load / 100;

    // Add corrections (temp, voltage, etc.)
    // ...

    return duration;
}

// Get cylinder TDC angle based on firing order
static uint16_t get_cylinder_tdc_angle(uint8_t cyl) {
    // Example for 4-cylinder, firing order 1-3-4-2
    switch(cyl) {
        case 0: return 0;     // Cylinder 1: 0°
        case 1: return 180;   // Cylinder 3: 180°
        case 2: return 360;   // Cylinder 4: 360°
        case 3: return 540;   // Cylinder 2: 540°
        default: return 0;
    }
}
```

---

## ⚡ Step 4: Integrate Ignition Control

### 4.1 Ignition Callback Functions

```c
// Ignition coil control
void start_dwell_0(uint8_t cyl) { digitalWrite(IGN_PIN_0, HIGH); }
void fire_spark_0(uint8_t cyl) { digitalWrite(IGN_PIN_0, LOW); }

void start_dwell_1(uint8_t cyl) { digitalWrite(IGN_PIN_1, HIGH); }
void fire_spark_1(uint8_t cyl) { digitalWrite(IGN_PIN_1, LOW); }

// ... repeat for all cylinders ...
```

### 4.2 Schedule Ignition Events

```c
void schedule_ignition_events(void) {
    uint32_t current_time = micros();
    uint16_t rpm = get_engine_rpm();

    if (rpm < 100 || !trigger_decoder_is_synced(&crank_decoder)) {
        return;
    }

    // Calculate timing advance based on RPM and load
    uint16_t timing_advance = calculate_timing_advance(rpm, load);

    // Calculate dwell time (typically 3-4ms)
    uint32_t dwell_time_us = 3000;

    // For each cylinder
    for (uint8_t cyl = 0; cyl < 4; cyl++) {
        // Get cylinder's TDC angle
        uint16_t tdc_angle = get_cylinder_tdc_angle(cyl);

        // If cam synced, use full cycle angle
        if (cam_sync_is_synced(&cam_sync)) {
            tdc_angle = cam_sync_get_full_cycle_angle(&cam_sync, tdc_angle);
        }

        // Calculate spark angle (TDC - advance)
        uint16_t spark_angle = (tdc_angle + 720 - timing_advance) % 720;

        // Calculate dwell angle (spark angle - dwell duration in degrees)
        uint32_t us_per_degree = 60000000UL / ((uint32_t)rpm * 360);
        uint16_t dwell_degrees = dwell_time_us / us_per_degree;
        uint16_t dwell_angle = (spark_angle + 720 - dwell_degrees) % 720;

        // Schedule ignition (dwell + spark)
        multistage_schedule_ignition(&ms_scheduler,
                                    cyl,
                                    dwell_angle,
                                    spark_angle,
                                    get_dwell_start_callback(cyl),
                                    get_spark_fire_callback(cyl),
                                    rpm,
                                    current_time);
    }
}
```

---

## 🔄 Step 5: Main Loop Integration

### 5.1 Remove Old Polling Code

**OLD (remove this):**
```c
void loop() {
    // ❌ Remove this - no longer needed!
    scheduler_process_events(&angle_scheduler, micros());

    // ... rest of loop ...
}
```

### 5.2 New Main Loop

**NEW:**
```c
void loop() {
    // Events fire automatically via hardware interrupts!
    // No polling needed!

    // Schedule events periodically (e.g., every revolution or every N ms)
    static uint32_t last_schedule_time = 0;
    uint32_t now = micros();

    if (now - last_schedule_time > 10000) {  // Every 10ms
        last_schedule_time = now;

        // Schedule next cycle of injection events
        schedule_injection_events();

        // Schedule next cycle of ignition events
        schedule_ignition_events();
    }

    // Your other application code
    update_sensors();
    update_displays();
    process_serial_commands();

    // ... etc ...
}
```

---

## 🧪 Step 6: Testing

### 6.1 Bench Testing (No Engine)

```c
void test_hardware_scheduler() {
    Serial.println("Testing hardware scheduler...");

    // Test callback
    static bool callback_fired = false;
    auto test_callback = [](void* ctx) {
        callback_fired = true;
        Serial.println("✅ Hardware event fired!");
    };

    // Schedule test event in 1ms
    uint32_t fire_time = micros() + 1000;
    hw_scheduler_t hw_sched;
    hw_scheduler_init(&hw_sched);

    int8_t id = hw_scheduler_schedule(&hw_sched, fire_time, test_callback, NULL);

    Serial.println("Waiting for event...");
    delay(10);

    if (callback_fired) {
        Serial.println("✅ Test PASSED - Hardware timing works!");
    } else {
        Serial.println("❌ Test FAILED - Check FTM configuration");
    }
}
```

### 6.2 Crank Simulation

Use a signal generator or Arduino to simulate crank signal:

```c
void test_with_simulated_crank() {
    // Generate 36-1 pattern at ~600 RPM
    // Tooth period at 600 RPM: ~2.77ms per tooth

    for (int tooth = 0; tooth < 36; tooth++) {
        if (tooth == 35) {
            delayMicroseconds(5540);  // Missing tooth (2× normal)
        } else {
            delayMicroseconds(2770);  // Normal tooth
        }

        // Trigger crank sensor callback
        uint32_t timestamp = micros();
        crank_sensor_callback(timestamp);
    }
}
```

### 6.3 Monitor Output

```c
void print_scheduler_status() {
    Serial.println("=== Scheduler Status ===");

    // Trigger decoder
    if (trigger_decoder_is_synced(&crank_decoder)) {
        Serial.println("✅ Crank synced");
        uint8_t tooth = trigger_decoder_get_tooth_index(&crank_decoder);
        Serial.printf("  Tooth: %d\n", tooth);
    } else {
        Serial.println("❌ Crank NOT synced");
    }

    // RPM
    uint16_t rpm = rpm_calculator_get_rpm(&rpm_calc);
    Serial.printf("RPM: %d\n", rpm);

    // Cam sync
    if (cam_sync_is_synced(&cam_sync)) {
        Serial.println("✅ Cam synced");
        engine_cycle_phase_t phase = cam_sync_get_phase(&cam_sync);
        Serial.printf("  Phase: %s\n",
                     phase == CYCLE_PHASE_FIRST_360 ? "First 360°" : "Second 360°");
    } else {
        Serial.println("⏳ Cam NOT synced yet");
    }

    // Event scheduler
    uint8_t active = scheduler_get_active_count(&angle_scheduler);
    Serial.printf("Active events: %d\n", active);

    // Hardware scheduler stats
    uint32_t fired, missed;
    hw_scheduler_get_stats(&hw_sched, &fired, &missed);
    Serial.printf("HW Events fired: %lu, missed: %lu\n", fired, missed);
}
```

---

## ⚠️ Troubleshooting

### Problem: Events not firing

**Check:**
1. FTM interrupt enabled in NVIC?
2. FTM clock running?
3. Callback function not NULL?
4. Event scheduled in future (not past)?

**Debug:**
```c
// Add to hw_scheduler_schedule():
Serial.printf("Scheduling event at %lu us (now: %lu)\n",
             fire_time_us, hw_scheduler_micros());
```

### Problem: Timing jitter

**Check:**
1. Other interrupts disabled during critical sections?
2. Main loop not too slow?
3. Serial.print() not in ISR?

**Debug:**
```c
// Measure actual vs expected timing
static uint32_t last_fire_time = 0;
void my_callback(void* ctx) {
    uint32_t now = micros();
    uint32_t delta = now - last_fire_time;
    Serial.printf("Delta: %lu us\n", delta);
    last_fire_time = now;
}
```

### Problem: Cam sync not working

**Check:**
1. Cam sensor connected to correct pin?
2. Signal edge detection correct (rising/falling)?
3. Crank synced before cam?

**Debug:**
```c
// Log cam events
void cam_sensor_callback(uint32_t timestamp) {
    bool cam_signal = digitalRead(CAM_SENSOR_PIN);
    Serial.printf("Cam event: %d at tooth %d\n",
                 cam_signal,
                 trigger_decoder_get_tooth_index(&crank_decoder));

    cam_sync_process_event(&cam_sync, cam_signal, tooth, timestamp);
}
```

---

## 📊 Performance Validation

### Expected Results

| Metric | Target | How to Measure |
|--------|--------|----------------|
| **Timing precision** | < 10µs | Oscilloscope on injector pin |
| **CPU load** | < 5% | `micros()` before/after main loop |
| **Event capacity** | 8+ simultaneous | Schedule 8 events, check all fire |
| **Missed events** | 0 | Check `events_missed` counter |

### Validation Code

```c
void validate_performance() {
    // Test 1: Timing precision
    Serial.println("Test 1: Timing precision");
    uint32_t fire_times[10];
    for (int i = 0; i < 10; i++) {
        fire_times[i] = 0;

        auto callback = [](void* ctx) {
            *(uint32_t*)ctx = micros();
        };

        uint32_t target = micros() + 1000;
        hw_scheduler_schedule(&hw_sched, target, callback, &fire_times[i]);
        delay(10);

        int32_t error = fire_times[i] - target;
        Serial.printf("  Error: %ld us\n", error);
    }

    // Test 2: CPU load
    Serial.println("Test 2: CPU load");
    uint32_t start = micros();
    for (int i = 0; i < 1000; i++) {
        // Simulate main loop
        delayMicroseconds(100);
    }
    uint32_t elapsed = micros() - start;
    Serial.printf("  1000 loops in %lu us (%.1f%% overhead)\n",
                 elapsed, (float)(elapsed - 100000) / 1000.0);
}
```

---

## ✅ Success Checklist

- [ ] Hardware scheduler initialized
- [ ] Crank sensor updates angle scheduler
- [ ] Cam sensor updates cycle phase
- [ ] Injection events scheduled and fire
- [ ] Ignition events scheduled and fire
- [ ] No polling in main loop
- [ ] Timing precision < 10µs
- [ ] CPU load < 5%
- [ ] No missed events
- [ ] Sequential injection works (if cam synced)

---

## 🎯 Next Steps

Once integration is complete and tested:

1. **Calibration:** Tune injection duration and timing advance maps
2. **Optimization:** Fine-tune dwell time and injection timing
3. **Features:** Add idle control, rev limiter, etc.
4. **Production:** Deploy to vehicle and monitor

---

**Good luck with integration!** 🚀

