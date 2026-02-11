# ECU Testing and Validation Guide

**Comprehensive testing procedures for the Teensy 3.5 Russefi ECU**

---

## ⚠️ Safety First

**CRITICAL SAFETY WARNINGS:**
- ✋ **Never test on a running vehicle without proper safety measures**
- 🔥 **Have fire extinguisher readily available**
- 🔌 **Use proper fuses and circuit protection**
- 🚗 **Test on bench or dyno before road use**
- 📞 **Have backup ECU available**
- 🛑 **Implement kill switch for immediate shutdown**

---

## Phase 1: Bench Testing (No Engine)

### 1.1 Power Supply Test

**Objective:** Verify ECU powers on correctly

**Equipment:**
- 12V bench power supply (or car battery)
- Multimeter
- USB cable for serial monitoring

**Procedure:**
1. Connect 12V to VIN pin, GND to ground
2. Verify 3.3V on 3.3V output pin
3. Check LED blinks at 1 Hz
4. Open serial console at 115200 baud
5. Verify startup banner displays

**Expected Results:**
```
========================================
   Russefi Teensy 3.5 ECU Firmware
========================================
Version: 0.1.0 (Phase 4)
CPU Speed: 120 MHz
...
Heartbeat: 1 seconds uptime
```

**Pass Criteria:** ✅ LED blinking, serial output visible

---

### 1.2 ADC Input Test

**Objective:** Verify analog sensor reading

**Equipment:**
- Variable power supply (0-5V)
- Potentiometer (10kΩ)
- Multimeter

**Procedure:**
1. Connect potentiometer to A0 (TPS input)
2. Sweep voltage from 0V to 5V
3. Monitor ADC readings via serial
4. Verify linear response

**Test Code:**
```cpp
void loop() {
    float voltage = adc_read_voltage(ADC_0, ADC0_DP0);
    Serial.print("TPS Voltage: ");
    Serial.println(voltage, 3);
    delay(100);
}
```

**Expected Results:**
- 0.0V input → 0.000V reading (±0.05V)
- 2.5V input → 2.500V reading (±0.05V)
- 5.0V input → 5.000V reading (±0.05V)

**Pass Criteria:** ✅ Linearity within ±2% full scale

---

### 1.3 PWM Output Test

**Objective:** Verify PWM generation accuracy

**Equipment:**
- Oscilloscope (100 MHz recommended)
- Test LED with resistor (optional)

**Procedure:**
1. Configure PWM on Pin 3 (FTM0_CH0)
2. Set frequency to 100 Hz
3. Sweep duty cycle 0% → 100%
4. Measure with oscilloscope

**Test Code:**
```cpp
pwm_config_t cfg = {
    .frequency_hz = 100,
    .alignment = PWM_EDGE_ALIGNED,
    .enable_prescaler_auto = true
};
pwm_init(PWM_FTM0, &cfg);

pwm_channel_config_t ch_cfg = {
    .polarity = PWM_POLARITY_HIGH,
    .duty_cycle_percent = 50,
    .enable_output = true
};
pwm_channel_init(PWM_FTM0, PWM_CHANNEL_0, &ch_cfg);
```

**Expected Results:**
- Frequency: 100 Hz ±1%
- 25% duty: 2.5ms high, 7.5ms low
- 50% duty: 5.0ms high, 5.0ms low
- 75% duty: 7.5ms high, 2.5ms low

**Pass Criteria:** ✅ Frequency within ±1%, duty within ±2%

---

### 1.4 PIT Timer Test

**Objective:** Verify precision timing

**Equipment:**
- Oscilloscope
- GPIO pin for timing measurement

**Procedure:**
1. Configure PIT for 1ms periodic interrupt
2. Toggle GPIO in callback
3. Measure period with scope

**Test Code:**
```cpp
void pit_callback(void) {
    gpio_toggle(GPIO_PORT_C, GPIO_PIN_5);
}

pit_config_t pit_cfg = {
    .period_us = 1000,  // 1ms
    .enable_interrupt = true,
    .enable_chain = false
};
pit_channel_init(PIT_CHANNEL_0, &pit_cfg);
pit_register_callback(PIT_CHANNEL_0, pit_callback);
pit_start(PIT_CHANNEL_0);
```

**Expected Results:**
- Period: 1.000ms ±1µs
- Jitter: < 5µs

**Pass Criteria:** ✅ Period within ±0.1%, jitter < 10µs

---

## Phase 2: Simulated Engine Testing

### 2.1 Crank Signal Simulation

**Objective:** Test engine position sensing

**Equipment:**
- Function generator or Arduino signal generator
- 36-1 crank wheel simulation

**Signal Parameters:**
- Frequency: 60 Hz (360 RPM for 36-tooth wheel)
- Voltage: 5V square wave
- Missing tooth: 1 pulse gap

**Procedure:**
1. Connect signal to Pin 33 (crank input)
2. Monitor RPM calculation
3. Verify sync lock

**Test Code:**
```cpp
crank_sensor_init(36, 1, SENSOR_TYPE_HALL);

void loop() {
    if (is_engine_synced()) {
        uint16_t rpm = get_engine_rpm();
        Serial.print("RPM: ");
        Serial.println(rpm);
    } else {
        Serial.println("No sync");
    }
    delay(100);
}
```

**Expected Results:**
- 60 Hz input → 360 RPM reading
- 100 Hz input → 600 RPM reading
- Sync locks within 2 revolutions

**Pass Criteria:** ✅ RPM accuracy within ±5 RPM

---

### 2.2 Fuel Injection Test

**Objective:** Verify fuel pulse calculation

**Simulated Conditions:**
- RPM: 3000
- MAP: 50 kPa (idle/cruise)
- TPS: 20%
- CLT: 80°C
- IAT: 25°C

**Expected Pulse Width:**
- Base calculation: ~2-4ms typical
- With corrections: ±20%

**Test Procedure:**
1. Set simulated sensor values
2. Calculate fuel pulse
3. Output to PWM
4. Measure with oscilloscope

**Pass Criteria:** ✅ Pulse width in reasonable range (1-10ms)

---

### 2.3 Ignition Timing Test

**Objective:** Verify timing calculation

**Test Scenarios:**

| RPM | MAP | Expected Timing |
|-----|-----|-----------------|
| 1000 | 30 kPa | ~10° BTDC |
| 3000 | 50 kPa | ~20° BTDC |
| 6000 | 80 kPa | ~30° BTDC |

**Procedure:**
1. Simulate engine conditions
2. Calculate ignition timing
3. Verify lookup table interpolation

**Pass Criteria:** ✅ Timing values match table ±2°

---

## Phase 3: Component Integration Test

### 3.1 Full Sensor Suite Test

**Objective:** All sensors reading simultaneously

**Equipment:**
- 6x potentiometers (TPS, MAP, CLT, IAT, O2, Vbat)
- Bench power supply

**Procedure:**
1. Connect all analog sensors
2. Read all channels in loop
3. Verify no crosstalk
4. Check update rate

**Expected Results:**
- All 6 sensors read correctly
- No interference between channels
- Update rate > 100 Hz

**Pass Criteria:** ✅ All sensors within ±3% accuracy

---

### 3.2 Control Loop Test

**Objective:** Sensor → Calculate → Actuate cycle

**Test Flow:**
```
Read Sensors → Calculate Fuel → Set PWM → Read Sensors
     ↓              ↓              ↓            ↓
  < 100µs        < 500µs        < 50µs      < 100µs
```

**Procedure:**
1. Measure loop execution time
2. Verify deterministic timing
3. Check for missed deadlines

**Pass Criteria:** ✅ Loop time < 1ms, no missed cycles

---

## Phase 4: Real Engine Testing (If Available)

### ⚠️ PRE-FLIGHT CHECKLIST

**MANDATORY before engine start:**
- [ ] Fuel pressure regulator set to 3 bar
- [ ] Fuel pump relay wired through kill switch
- [ ] Ignition coils have proper dwell limiting
- [ ] Injectors are correct flow rate
- [ ] Wideband O2 sensor installed
- [ ] Coolant temperature sensor verified
- [ ] MAP sensor vacuum line connected
- [ ] TPS calibrated (0% closed, 100% WOT)
- [ ] Crank sensor gap verified (VR: 0.5-1.0mm, Hall: per spec)
- [ ] Fire extinguisher within reach
- [ ] Backup ECU ready to swap

---

### 4.1 Initial Crank Test (No Fuel/Spark)

**Objective:** Verify position sensing on real engine

**Procedure:**
1. Disconnect fuel pump and ignition
2. Crank engine with starter
3. Verify sync lock
4. Check RPM reading (should be cranking speed ~200-300 RPM)

**Pass Criteria:** ✅ Sync within 2 revolutions, stable RPM

---

### 4.2 First Start Attempt

**⚠️ HIGH RISK - PROCEED WITH CAUTION**

**Procedure:**
1. Enable fuel pump
2. Prime fuel system
3. Check for leaks
4. Enable ignition
5. Set base fuel table to conservative values
6. Crank engine
7. **BE READY TO HIT KILL SWITCH**

**Conservative Starting Values:**
- Base pulse width: 3ms
- Ignition timing: 10° BTDC
- Cranking enrichment: 1.5x

**Expected Behavior:**
- Engine should fire within 3-5 seconds
- Idle RPM: 600-1200 (varies by engine)
- AFR: 13-15 (slightly rich is OK)

**ABORT if:**
- Backfire or popping
- AFR < 10 (too rich) or > 16 (too lean)
- Coolant temp rising rapidly
- Any fuel leaks
- Engine runs rough for > 10 seconds

---

### 4.3 Idle Tuning

**Objective:** Stable idle operation

**Target Parameters:**
- RPM: 800-1000 (varies by engine)
- AFR: 14.0-14.7 (near stoich)
- MAP: 30-40 kPa (naturally aspirated)
- Idle stability: ±50 RPM

**Tuning Procedure:**
1. Adjust base pulse width for target AFR
2. Adjust idle timing for smoothness
3. Fine-tune with closed-loop O2 correction
4. Verify hot/cold idle

**Pass Criteria:** ✅ Stable idle, AFR 14.0-14.7, smooth operation

---

### 4.4 Part-Throttle Testing

**Objective:** Verify VE table accuracy

**Test Points:**

| RPM | TPS | Expected MAP | Target AFR |
|-----|-----|--------------|------------|
| 2000 | 30% | 40-50 kPa | 14.7 |
| 3000 | 50% | 60-70 kPa | 14.7 |
| 4000 | 70% | 80-90 kPa | 14.7 |

**Procedure:**
1. Hold steady RPM/TPS
2. Log AFR for 10 seconds
3. Adjust VE table cell
4. Repeat until AFR within ±0.5

**Pass Criteria:** ✅ AFR within ±0.5 at all test points

---

### 4.5 Wide Open Throttle (WOT) Test

**⚠️ DANGER - Dyno recommended**

**Target WOT AFR:** 12.5-13.0 (rich for power/safety)

**Procedure:**
1. **Only on dyno or closed course**
2. Full throttle from 3000 → redline
3. Log AFR continuously
4. Check for knock
5. Monitor coolant temp

**ABORT if:**
- AFR > 13.5 (lean danger zone)
- Knock detected
- Coolant > 110°C
- Any abnormal sounds

**Pass Criteria:** ✅ AFR 12.5-13.0, no knock, smooth power

---

## Phase 5: Endurance Testing

### 5.1 Thermal Test

**Objective:** Verify ECU doesn't overheat

**Procedure:**
1. Run engine for 1 hour
2. Monitor ECU temperature
3. Check for thermal throttling

**Pass Criteria:** ✅ ECU temp < 85°C, no errors

---

### 5.2 Vibration Test

**Objective:** Ensure reliable operation under vibration

**Procedure:**
1. Secure ECU to engine
2. Run at various RPMs
3. Monitor for connector issues
4. Check for memory corruption

**Pass Criteria:** ✅ No errors, stable operation

---

## Test Results Template

```
Date: __________
Tester: __________
Engine: __________ (displacement, cylinders)

[ ] Phase 1: Bench Testing - PASS / FAIL
[ ] Phase 2: Simulated Engine - PASS / FAIL
[ ] Phase 3: Integration - PASS / FAIL
[ ] Phase 4: Real Engine - PASS / FAIL
[ ] Phase 5: Endurance - PASS / FAIL

Notes:
_________________________________________________
_________________________________________________

Signature: __________
```

---

## Troubleshooting Guide

### Issue: No Serial Output
- Check USB cable
- Verify baud rate (115200)
- Press reset button

### Issue: ADC Reading Stuck at 0V or 3.3V
- Check pin connections
- Verify ADC channel mapping
- Test with known voltage source

### Issue: PWM Not Outputting
- Check pin mux configuration
- Verify FTM clock enabled
- Test with oscilloscope

### Issue: No RPM Reading
- Check crank sensor gap
- Verify VR signal conditioning
- Check for 5V pullup (Hall sensors)

### Issue: Engine Won't Start
- Verify fuel pressure (3 bar typical)
- Check spark with timing light
- Verify injector pulse width (> 0.5ms)
- Check base timing (10° BTDC)

### Issue: Engine Runs Rough
- Check AFR (should be 13-15 at idle)
- Verify ignition timing
- Check for vacuum leaks
- Inspect sensor readings

---

## Final Validation Checklist

**Before declaring ECU production-ready:**

- [ ] All Phase 1-3 bench tests passed
- [ ] Simulated engine tests passed
- [ ] Real engine starts reliably
- [ ] Stable idle achieved
- [ ] Part-throttle operation smooth
- [ ] WOT test completed (dyno)
- [ ] 1+ hour endurance test
- [ ] Thermal test passed
- [ ] Vibration test passed
- [ ] Data logging verified
- [ ] All safety interlocks tested
- [ ] Backup ECU swap tested
- [ ] Fire extinguisher accessible
- [ ] Insurance notified (if applicable)

**Only proceed to road testing after ALL items checked!**

---

**Document Version:** 1.0
**Last Updated:** 2026-02-10
**Maintained By:** Teensy35 Russefi Team
