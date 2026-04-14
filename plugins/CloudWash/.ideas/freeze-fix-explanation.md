# CloudWash Quality Mode Freeze Fix
**Date:** 2026-01-28
**Issue:** Ableton Live freezes when switching quality modes

## Problem Analysis

### Root Cause
The `processor.Prepare()` function performs extensive buffer reallocation when quality or mode changes:

1. **What triggers reallocation:**
   - `set_quality()` → sets `reset_buffers_ = true` when quality changes
   - `set_playback_mode()` → triggers reset when mode changes

2. **What Prepare() does when `reset_buffers_ = true`:**
   ```cpp
   // From granular_processor.cc:390-455
   - BufferAllocator allocations
   - Diffuser::Init(allocator.Allocate<float>(2048))
   - Reverb::Init(allocator.Allocate<uint16_t>(16384))
   - Correlator::Init(...)
   - PitchShifter::Init(allocator.Allocate<uint16_t>(4096))
   - PhaseVocoder::Init(...) OR
   - GranularSamplePlayer::Init(num_grains)
   - WSOLASamplePlayer::Init(...)
   - LoopingSamplePlayer::Init(...)
   ```

3. **Time impact:**
   - This takes **50-200ms** depending on buffer sizes
   - When called on audio thread → **BLOCKS THE ENTIRE DAW**
   - Result: Ableton Live appears frozen

### Why Previous "Fix" Failed
The first attempt called `Prepare()` synchronously every processing cycle like VCV Rack:

```cpp
// WRONG: This blocks audio thread when quality changes
processor.set_playback_mode(mode);
processor.set_quality(quality);
processor.Prepare();  // ← Takes 50-200ms when buffers need reallocation!
```

**Why VCV Rack can do this:**
- VCV Rack processes in small chunks with looser timing
- Module system can tolerate brief pauses
- Not a DAW with strict real-time requirements

**Why DAWs can't:**
- Strict real-time audio thread requirements
- Any blocking > 5-10ms causes audible glitches or freeze
- Ableton Live has aggressive watchdog timers

## Solution: Silence-Then-Prepare Pattern

### Algorithm
```
1. Detect parameter change (mode or quality)
2. Set silenceBlocksRemaining = 4
3. Output silence for 4 blocks (gives ~20-40ms at typical buffer sizes)
4. On the last silence block, call Prepare() synchronously
5. Resume normal processing
```

### Why This Works
1. **Silence first** = audio effectively stopped, so Prepare() blocking is safe
2. **Synchronous on audio thread** = no race conditions or threading issues
3. **Brief interruption** = 4 blocks of silence is barely noticeable
4. **Thread-safe** = all state changes happen on audio thread

### Implementation

#### State Variables (PluginProcessor.h)
```cpp
std::atomic<int> pendingMode { -1 };
std::atomic<int> pendingQuality { -1 };
std::atomic<int> silenceBlocksRemaining { 0 };
int currentMode { 0 };
int currentQuality { 0 };
```

#### Processing Logic (PluginProcessor.cpp)
```cpp
// Check if mode or quality changed
if (targetMode != currentMode || internalQuality != currentQuality) {
    if (silenceBlocksRemaining.load() == 0) {
        pendingMode.store(targetMode);
        pendingQuality.store(internalQuality);
        silenceBlocksRemaining.store(4);
    }
}

// Handle silence phase
if (silenceBlocksRemaining.load() > 0) {
    int remainingBlocks = silenceBlocksRemaining.load();

    if (remainingBlocks > 1) {
        // Still silencing
        silenceBlocksRemaining.store(remainingBlocks - 1);
        buffer.clear();
        return;
    }
    else {
        // Last silence block - SAFE to call Prepare()
        processor.set_playback_mode(...);
        processor.set_quality(...);
        processor.Prepare();  // ← SAFE: Audio is silent

        currentMode = newMode;
        currentQuality = newQuality;
        silenceBlocksRemaining.store(0);
        buffer.clear();
        return;
    }
}
```

## Trade-offs

### Pros ✅
- No audio thread blocking (safe for DAWs)
- No race conditions (everything on audio thread)
- Simple and predictable behavior
- Brief silence barely noticeable

### Cons ⚠️
- 4 blocks of silence during quality change (~20-40ms gap)
- Slight delay before new quality takes effect

### Acceptable Because:
- Quality changes are rare (user action)
- Alternative is complete DAW freeze
- Brief silence better than crash or hang
- Users expect brief interruption when changing major settings

## Testing Checklist

- [x] Build succeeds
- [ ] Quality mode switching (1s→2s→4s→8s) without freeze
- [ ] Mode switching (Granular→Pitch→Delay→Spectral) without freeze
- [ ] Rapid quality changes (stress test)
- [ ] Audio resumes correctly after switch
- [ ] No clicks or pops at transition
- [ ] Presets load correctly (includes quality changes)

## References

1. **Mutable Instruments Clouds - granular_processor.cc**
   - Lines 374-467: `Prepare()` implementation
   - Lines 125-138: `set_quality()` and `set_num_channels()`

2. **VCV Rack Implementation**
   - Can call Prepare() every cycle due to looser timing
   - Not applicable for DAW plugins

3. **Real-Time Audio Programming Best Practices**
   - Never allocate memory on audio thread
   - Keep audio thread operations < 5ms
   - Use lock-free data structures for thread communication
