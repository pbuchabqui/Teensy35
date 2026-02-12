# Phase 1: Hardware Timer Integration - Progress Report

**Status:** ✅ **IN PROGRESS** (50% complete)
**Date:** 2026-02-12

---

## ✅ What's Been Implemented

### 1. Hardware Timer Scheduler (`hardware_scheduler_k64.c/h`)

**Created:** Complete FTM-based hardware timer scheduler

**Features:**
- ✅ Uses FTM Output Compare mode for precise timing
- ✅ Automatic interrupt-driven event firing (no polling!)
- ✅ Support for up to 8 simultaneous hardware-scheduled events
- ✅ Microsecond-precision timing
- ✅ Event cancellation and statistics
- ✅ FTM1 and FTM2 interrupt handlers

**API:**
```c
// Initialize
hw_scheduler_init(&hw_sched);

// Schedule event at absolute time
int8_t event_id = hw_scheduler_schedule(&hw_sched,
                                        fire_time_us,
                                        callback,
                                        context);

// Cancel event
hw_scheduler_cancel(&hw_sched, event_id);

// Get precise time
uint32_t now = hw_scheduler_micros();
```

**Key Implementation Details:**
- Uses FTM Output Compare (`CnSC = 0x50`)
- Automatic interrupt when FTM counter matches compare value
- FTM1_IRQHandler and FTM2_IRQHandler implemented
- Allocates FTM channels dynamically
- Tracks events fired and missed

---

## 🔧 What Needs to Be Completed

### 2. Integration with Event Scheduler

**Status:** Partially done

**Need to:**
1. Modify `scheduler_add_event()` to use `hw_scheduler_schedule()`
2. Remove polling-based `scheduler_process_events()`
3. Add hardware event callback wrapper
4. Update event_scheduler.h API documentation

**Current approach (polling):**
```c
// OLD (v2.3.0):
scheduler_add_event(&sched, angle, cyl, action, time);
// ... later in main loop:
scheduler_process_events(&sched, micros());  // ❌ Manual polling
```

**New approach (hardware):**
```c
// NEW (v2.3.1):
scheduler_add_event(&sched, angle, cyl, action, time);
// Event fires automatically via hardware interrupt! ✅
```

---

## 📊 Implementation Strategy

### Option A: Transparent Integration (Recommended)

Keep existing event_scheduler API, use hardware scheduler internally:

```c
bool scheduler_add_event(event_scheduler_t* sched,
                        uint16_t angle,
                        uint8_t cylinder,
                        void (*action)(uint8_t),
                        uint32_t current_time_us)
{
    // Calculate timing (same as before)
    uint32_t time_until_event = angle_delta * sched->us_per_degree;
    uint32_t fire_time = current_time_us + time_until_event;

    // Store event info
    sched->events[i].trigger_angle = angle;
    sched->events[i].cylinder = cylinder;
    sched->events[i].action = action;

    // NEW: Schedule via hardware timer
    sched->events[i].hw_event_id = hw_scheduler_schedule(&hw_sched,
                                                         fire_time,
                                                         hw_event_callback,
                                                         &sched->events[i]);

    return true;
}

// Hardware callback wrapper
static void hw_event_callback(void* context)
{
    scheduled_event_t* event = (scheduled_event_t*)context;
    // Fire the actual event
    event->action(event->cylinder);
}
```

**Benefits:**
- ✅ Keeps existing API
- ✅ No changes needed in user code
- ✅ `scheduler_process_events()` becomes optional (for diagnostics only)

---

## 🎯 Next Steps

1. **Complete hardware integration** (1-2 hours)
   - Modify scheduler_add_event()
   - Add hw_event_callback wrapper
   - Update event cancellation

2. **Test hardware timing** (1-2 hours)
   - Verify microsecond precision
   - Test event firing accuracy
   - Measure jitter

3. **Multi-stage events** (2-3 hours)
   - Implement start + end events
   - Duration-based scheduling
   - Event chaining

4. **Cam sync** (2-3 hours)
   - Add cam sensor support
   - Cycle phase detection
   - Sequential injection support

---

## 📈 Progress Tracking

| Task | Status | Time |
|------|--------|------|
| Hardware scheduler core | ✅ Done | 2h |
| FTM interrupt handlers | ✅ Done | 1h |
| Event scheduler integration | ⏳ In progress | 2h |
| Multi-stage events | ⏳ Pending | 3h |
| Cam sync basic | ⏳ Pending | 3h |
| Testing | ⏳ Pending | 2h |
| **TOTAL** | **50% complete** | **~13h** |

---

## 🚀 Expected Results

After Phase 1 completion:

**Before (v2.3.0):**
- ⚠️ Polling-based timing (depends on main loop speed)
- ⚠️ No duration control
- ⚠️ No sequential injection

**After (v2.3.1):**
- ✅ Hardware-precise timing (μs accuracy)
- ✅ Multi-stage events (start + end)
- ✅ Cam sync (sequential injection ready)
- ✅ Interrupt-driven (no polling overhead)

---

## 💾 Commit Plan

1. Commit hardware scheduler (this commit)
2. Commit integration with event_scheduler
3. Commit multi-stage events
4. Commit cam sync
5. Final commit with Phase 1 summary

**Current commit:** Hardware timer scheduler implementation

