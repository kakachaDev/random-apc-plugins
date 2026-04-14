# Mutable Instruments Clouds DSP Analysis
**Complete Implementation Reference for CloudWash Plugin**

Date: 2026-01-26
Source: https://github.com/pichenettes/eurorack/tree/master/clouds
Analysis Level: Production Implementation Details

---

## 1. BUFFER ARCHITECTURE

### 1.1 Main Buffer Allocations

**Hardware Memory (Eurorack Module):**
- **Large Buffer:** 118,784 bytes (116 KB)
- **Small Buffer:** 65,408 bytes (63.875 KB)
- **Total System Memory:** 184,192 bytes (~180 KB)

**Buffer Division by Mode:**

**Mono Recording:**
- Large buffer: 120 KB sample storage
- Small buffer: Entirely for FX workspace

**Stereo Recording:**
- Large buffer: 64 KB samples + FX workspace
- Small buffer: 64 KB samples

### 1.2 Sample Resolution Modes

| Resolution | Bits | Storage | Capacity (Mono) | Capacity (Stereo) |
|-----------|------|---------|-----------------|-------------------|
| 16-bit | 16 | `s16_[]` | ~60,000 samples | ~32,000 samples |
| 8-bit | 8 | `s8_[]` | ~120,000 samples | ~64,000 samples |
| 8-bit Dithered | 8 | `s8_[]` | ~120,000 samples | ~64,000 samples |
| 8-bit μ-Law | 8 | `s8_[]` | ~120,000 samples | ~64,000 samples |

**Recording Time (at 32 kHz):**
- 16-bit mono: ~1.875 seconds
- 8-bit mono: ~3.75 seconds
- 16-bit stereo: ~1 second
- 8-bit stereo: ~2 seconds

### 1.3 Special Buffer Constants

```cpp
kCrossFadeSize = 256           // Smooth recording resume transitions
kInterpolationTail = 8         // Extra space for interpolation
usable_capacity = size - 8     // Actual usable buffer space
```

---

## 2. FOUR PROCESSING MODES

### 2.1 Mode 0: Granular Synthesis

**Core Parameters:**
- **Grain Count:** 40 grains (mono) / 32 grains (stereo)
- **Low Fidelity Mode:** Reduces to 23/40 (mono) or 23/32 (stereo)
- **Grain Size:** Lookup table indexed (257 values) - see lut_grain_size
- **Window Functions:** Hann, Triangle, Exponential

**Grain Scheduling Algorithm:**

**Probabilistic Mode (default):**
```cpp
float p = target_num_grains / grain_size_hint_;
if (Random::GetFloat() < p && active_grains < target) {
    trigger_grain();
}
```

**Deterministic Mode (when `use_deterministic_seed = true`):**
```cpp
float space_between_grains = grain_size_hint_ / target_num_grains;
grain_rate_phasor_ += 1.0f;
if (grain_rate_phasor_ >= space_between_grains) {
    trigger_grain();
    grain_rate_phasor_ = 0.0f;
}
```

**Density → Overlap Conversion:**
```cpp
// When density > 0.53:
overlap = (density - 0.53f) * 2.12f;

// Cubic scaling for perceptual linearity:
overlap = overlap³;

// Calculate target grain count:
target_num_grains = max_num_grains * overlap;
```

**Window Shape Mapping:**
```cpp
// Texture parameter maps to window shape (0 to 1.333):
window_shape = texture * 1.333f;

// Window gain modulation:
window_gain = 1.0 + 2.0 * window_shape;  // Range: [1.0, 2.0]
```

**Gain Normalization:**
```cpp
// Carmack fast inverse square root for active grains:
float gain = (num_grains_ > 2.0)
    ? fast_rsqrt_carmack(num_grains_ - 1.0)
    : 1.0;
```

**Grain Structure:**
- `first_sample_`: Position in buffer
- `width_`: Grain duration in samples
- `phase_increment_`: Pitch/speed (1.0 = normal)
- `envelope_phase_increment_ = 2.0f / width`: Linear envelope progression
- `gain_l_`, `gain_r_`: Stereo pan control

**Quality Levels:**
- HIGH: Full interpolation
- MEDIUM: Reduced interpolation
- LOW: Linear/nearest-neighbor

---

### 2.2 Mode 1: Pitch-Shifter / Time-Stretcher (WSOLA)

**Algorithm:** Waveform Similarity Overlap-Add (WSOLA)

**Buffer Configuration:**
- **Maximum Window Size:** 4096 samples
- **Initial Window Size:** 2048 samples (kMaxWSOLASize / 2)
- **Dual Windows:** `windows_[0]` and `windows_[1]` for overlap-add
- **Window Size Adjustment:** Minimum 4-sample increments

**Correlation Parameters:**
```cpp
stride = window_size_ / 2048.0f;  // Normalized 1.0 to 2.0
// Correlation search:
//   - Source material: full window_size
//   - Target material: 2 × window_size
```

**Pitch Control:**
```cpp
// Fixed-point representation:
pitch_ratio_fixed = next_pitch_ratio_ * 65536.0f;

// Maximum pitch change: 12 semitones per window
```

**Processing Strategy:**
- Dual overlapping windows run in parallel
- Seamless overlap-add summation
- Window regeneration on expiry
- Frame-by-frame output with continuous transitions

**Key Difference from Granular:**
- Maintains pitch while changing playback speed
- Uses correlation-based best-match searching
- Larger processing windows for quality

---

### 2.3 Mode 2: Looping Delay

**Buffer Specifications:**
```cpp
kCrossfadeDuration = 64.0f samples;  // Smooth loop transitions
max_delay = buffer_size - 64;        // Operational window
```

**Synchronization Modes:**

**Free-Running Loop:**
```cpp
phase_increment = SemitonesToRatio(parameters.pitch);
// Allows pitch shifting via playback speed
```

**Tap-Tempo Synchronized:**
```cpp
if (tap_counter > 128 samples) {
    tap_delay_ = tap_counter;
    synchronized_ = true;
    target_delay = tap_delay_;
}

// When synchronized:
phase_increment = 1.0;  // Fixed, no pitch variation
```

**Freeze Mode Integration:**
```cpp
if (freeze && synchronized) {
    // Loop at tap tempo
    phase_increment = 1.0;
} else if (freeze && !synchronized) {
    // Loop with pitch control
    phase_increment = SemitonesToRatio(pitch);
}
```

**Crossfade Scaling:**
```cpp
tail_duration = crossfade_samples * phase_increment;
```

**Pitch Shifting in Looping Delay:**
- Uses dedicated pitch shifter module (see section 2.5)
- Only active when `!freeze || synchronized`
- Maintains temporal coherence

---

### 2.4 Mode 3: Spectral Processing (Phase Vocoder)

**FFT Architecture:**
- **Maximum FFT Size:** 4096 samples
- **Window LUT Size:** 4097 samples (from resources.h)
- **Sine Window:** 4096-sample dedicated window

**STFT Processing Pipeline:**
1. **Analysis:** Windowing → FFT → Frequency domain
2. **Modification:** Frame transformation (spectral processing)
3. **Resynthesis:** IFFT → Windowing
4. **Overlap-Add:** Frame combination for continuous output

**Spectral Transformations:**

**Quantization:**
```cpp
QuantizeMagnitudes(amount);
// Bins to nearest harmonic/pitch grid
```

**Pitch Warping:**
```cpp
WarpMagnitudes(amount);
ShiftMagnitudes(pitch_ratio);
```

**Phase Processing:**
```cpp
SetPhases(diffusion, pitch_ratio);
// Phase unrolling via phases_delta_ buffer
// Coherent phase reconstruction across frames
```

**Parameter Mapping (from granular_processor.cc):**

```cpp
// SPECTRAL MODE PARAMETER MAPPING:
quantization = texture;
refresh_rate = 0.01f + 0.99f * density;
warp = size³;  // Cubic scaling
phase_randomization = (texture-based diffusion)
```

**Texture Storage:**
- Multiple texture buffers: `textures_[kMaxNumTextures]`
- `StoreMagnitudes()` and `ReplayMagnitudes()` for feedback
- Density-based grain/texture cycling

**High-Frequency Truncation:**
- 16 bins removed from top of spectrum
- Reduces aliasing and processing load

**Glitch Processing:**
- `AddGlitch(algorithm)` method available
- Configurable glitch algorithms

**Key Difference from Other Modes:**
- **Does NOT record to audio buffer** (bypass recording)
- Real-time spectral processing only
- Higher CPU usage due to FFT operations

---

### 2.5 Pitch Shifter (Used in Looping Delay)

**Buffer Architecture:**
- **Delay Line Size:** 2047 samples per channel (L/R)
- **Internal Buffer:** 4096 samples at 16-bit resolution

**Algorithm:** Phase-Modulation Delay Line (NOT grain-based)

```cpp
// Phase accumulator advancement:
phase_ += (1.0f - ratio_) / size_;

// Dual interpolation reads:
position_1 = delay_line_read_pos;
position_2 = position_1 + (buffer_size / 2);

// Triangular crossfade envelope:
triangle_wave = calculate_triangle(phase_);
output = interpolate(position_1) * (1 - triangle_wave)
       + interpolate(position_2) * triangle_wave;
```

**Window Size (Dynamic):**
```cpp
// Cubic interpolation for smooth scaling:
size_samples = 128.0f + (2047.0f - 128.0f) * size³;
// Range: 128 to 2047 samples

// Smooth parameter transitions:
// First-order filtering on size changes
```

**Latency:**
- Fixed latency: 2047 samples
- At 32 kHz: ~64 ms
- At 48 kHz: ~42.6 ms

**Pitch Range:**
- Not explicitly defined in code
- `ratio_` parameter controls proportional shift
- External mapping required (likely ±12 to ±24 semitones)

---

## 3. REVERB IMPLEMENTATION

**Algorithm:** Griesinger topology (Dattorro paper implementation)

### 3.1 Buffer Architecture

- **Total Buffer Size:** 16,384 samples at 12-bit resolution
- **Sample Rate:** 32 kHz (Eurorack hardware)

### 3.2 Topology Structure

**Input Stage (4 cascaded allpass diffusers):**
```cpp
Delay lengths: 113, 162, 241, 399 samples
Coefficient: kap (diffusion parameter)
```

**Main Reverb (Dual parallel loops):**

Each loop contains:
- Modulated delay line (with LFO)
- Low-pass filter stage
- Two allpass diffusers
- Feedback delay line

**Diffuser Configuration:**

Total: **8 allpass filters** (4 per channel L/R)
```cpp
Delay line sizes: 126, 180, 269, 444, 151, 205, 245, 405 samples
Allpass coefficient: kap = 0.625f
Buffer precision: 2048 samples at 32-bit float
```

**Main Loop Delays:**
```cpp
First loop diffusers: 1653, 2038, 3411 samples
Second loop diffusers: 1913, 1663 samples
Maximum delay line: 4782 samples
```

### 3.3 Parameters

```cpp
// From granular_processor.cc:
reverb_.set_amount(reverb_amount * 0.54f);
reverb_.set_diffusion(0.7f);
reverb_.set_time(0.35f + 0.63f * reverb);  // Range: [0.35, 0.98]
reverb_.set_input_gain(0.2f);
reverb_.set_lp(0.6f + 0.37f * reverb);     // Range: [0.6, 0.97]

// LFO modulation (shimmer/chorus):
LFO_1: 0.5 Hz
LFO_2: 0.3 Hz
```

**Output Mixing:**
```cpp
// Crossfade lookup tables with post-gain:
final_output = crossfade(dry, wet) * 1.2f;
```

### 3.4 Reverb Time Calculation

At 32 kHz sample rate:
- Max delay: 4782 samples = ~149 ms per loop pass
- With feedback (0.35 to 0.98), decay time: ~500 ms to 8+ seconds

---

## 4. GRAIN ENGINE SPECIFICS

### 4.1 Grain Size Ranges

**Lookup Table:** `lut_grain_size` (257 values)
- Size parameter (0.0 to 1.0) indexes into table
- Actual values NOT in available source files
- Likely range estimation: **20ms to 500ms** (based on typical granular synthesis)

**Dynamic Adjustment:**
```cpp
// Prevent buffer underrun with pitch shifting:
grain_size = std::min(grain_size, buffer_size * 0.25f * inv_pitch_ratio);
```

### 4.2 Density Trigger Intervals

**Overlap Parameter Controls Density:**

```cpp
// User density → overlap conversion:
if (density > 0.53f) {
    overlap = (density - 0.53f) * 2.12f;
} else {
    overlap = 0.0f;  // Sparse grains
}

// Cubic scaling:
overlap = overlap³;

// Calculate spacing:
space_between_grains = grain_size_hint_ / target_num_grains;

// Example:
// grain_size = 200 ms (6400 samples @ 32kHz)
// overlap = 0.5 (50%)
// target_grains = 40 * 0.5 = 20
// spacing = 6400 / 20 = 320 samples = 10ms trigger interval
```

### 4.3 Envelope Windows

**Window Types:**

1. **Hann Window (Raised Cosine):**
   ```
   w(n) = 0.5 * (1 - cos(2π * n / N))
   ```

2. **Triangle Window:**
   ```
   w(n) = 1 - |2n/N - 1|
   ```

3. **Exponential Window:**
   ```
   Attack: exponential rise
   Release: exponential decay
   ```

**Window Shape Parameter (texture):**
```cpp
window_shape = texture * 1.333f;  // Range: [0.0, 1.333]

// Shape transitions:
// 0.0 → Sharp transients (triangle-like)
// 0.5 → Balanced (Hann-like)
// 1.0+ → Smooth (exponential-like)
```

**Lookup Tables (from resources.h):**
```cpp
LUT_WINDOW_SIZE = 4097 samples     // Main window function table
LUT_SINE_WINDOW_4096_SIZE = 4096  // Dedicated sine/Hann window
LUT_XFADE_IN_SIZE = 17            // Crossfade envelope
LUT_XFADE_OUT_SIZE = 17           // Crossfade envelope
```

### 4.4 Stereo Spread

```cpp
// Random pan per grain:
pan = 0.5 + stereo_spread * (Random::GetFloat() - 0.5);

// Convert to L/R gains:
gain_l_ = sqrt(1.0 - pan);
gain_r_ = sqrt(pan);
```

---

## 5. PARAMETER MAPPINGS

### 5.1 Common Parameters (All Modes)

| Parameter | Range | Units | Description |
|-----------|-------|-------|-------------|
| **position** | 0.0 - 1.0 | normalized | Buffer read position |
| **size** | 0.0 - 1.0 | normalized | Grain/window size index |
| **pitch** | -48 - +48 | semitones | Pitch transposition |
| **density** | 0.0 - 1.0 | normalized | Overlap/refresh rate |
| **texture** | 0.0 - 1.0 | normalized | Window shape/quantization |
| **dry_wet** | 0.0 - 1.0 | normalized | Dry/wet mix |
| **stereo_spread** | 0.0 - 1.0 | normalized | Stereo width |
| **feedback** | 0.0 - 1.0 | normalized | Input→output feedback |
| **reverb** | 0.0 - 1.0 | normalized | Reverb amount |
| **freeze** | OFF/ON | boolean | Freeze recording |
| **trigger** | OFF/ON | boolean | Grain trigger |
| **gate** | OFF/ON | boolean | Gate input |

### 5.2 Mode-Specific Mappings

**MODE 0: GRANULAR**

| User Param | DSP Mapping | Calculation |
|-----------|-------------|-------------|
| position | grain start | `position * available_buffer` |
| size | grain duration | `lut_grain_size[size * 257]` |
| density | overlap | `(density - 0.53) * 2.12` (if > 0.53) |
| texture | window shape | `texture * 1.333` |
| pitch | playback speed | `2^(pitch/12)` |

**Granular-Specific:**
```cpp
struct Granular {
    float overlap;              // Calculated from density
    float window_shape;         // From texture
    float stereo_spread;        // Direct mapping
    bool use_deterministic_seed; // Trigger mode
};
```

---

**MODE 1: STRETCH (WSOLA)**

| User Param | DSP Mapping | Effect |
|-----------|-------------|--------|
| position | read position | Seek point in buffer |
| size | window size | `128 + (4096-128) * size³` |
| density | N/A | (Not used in stretch) |
| texture | diffusion | Waveform similarity threshold |
| pitch | time ratio | Speed without pitch change |

---

**MODE 2: LOOPING DELAY**

| User Param | DSP Mapping | Effect |
|-----------|-------------|--------|
| position | loop point | Loop start position |
| size | delay time | Loop length |
| density | N/A | (Not used) |
| texture | N/A | (Not used) |
| pitch | pitch shift | `2^(pitch/12)` via pitch shifter |

**Synchronization:**
- When `tap_tempo` active: `delay_time = tap_delay_`
- When synchronized: `pitch_shift = disabled`

---

**MODE 3: SPECTRAL**

| User Param | DSP Mapping | Calculation |
|-----------|-------------|-------------|
| position | N/A | (Not used - no buffer recording) |
| size | warp amount | `size³` (cubic) |
| density | refresh rate | `0.01 + 0.99 * density` |
| texture | quantization | Direct mapping |
| pitch | freq shift | Spectral bin shifting |

**Spectral-Specific:**
```cpp
struct Spectral {
    float quantization;        // = texture
    float refresh_rate;        // = 0.01 + 0.99 * density
    float phase_randomization; // Diffusion amount
    float warp;                // = size³
};
```

---

### 5.3 Feedback Mapping (All Modes)

```cpp
// High-pass filter cutoff (prevents buildup):
float cutoff = (20.0f + 100.0f * feedback² ) / sample_rate();

// Range at 32kHz:
// feedback = 0.0 → 20 Hz / 32000 = 0.000625
// feedback = 1.0 → 120 Hz / 32000 = 0.00375
```

---

## 6. SAMPLE RATE CONSIDERATIONS

### 6.1 Clouds Hardware Sample Rate

**Primary Rate:** 32 kHz (Eurorack module)

**Low-Fidelity Mode:** 16 kHz
- Reduces grain count
- Reduces CPU usage
- Extends recording time (2× capacity)

**Downsampling Factor:** 2 (internal processing)
- Some operations run at 16 kHz even in normal mode

### 6.2 Converting to 48 kHz (for JUCE Plugin)

**Buffer Size Scaling:**
```
Original (32kHz): 120,000 samples = 3.75 seconds
JUCE (48kHz):     180,000 samples = 3.75 seconds (same time)

Scaling factor: 48000 / 32000 = 1.5
```

**Grain Size Scaling:**
```
If original grain = 200ms:
  @ 32kHz: 6400 samples
  @ 48kHz: 9600 samples (multiply by 1.5)
```

**Delay Line Scaling:**
```
Reverb delay (4782 samples @ 32kHz = 149ms):
  @ 48kHz: 7173 samples = 149ms

Pitch shifter (2047 samples @ 32kHz = 64ms):
  @ 48kHz: 3071 samples = 64ms
```

---

## 7. IMPLEMENTATION RECOMMENDATIONS FOR CLOUDWASH

### 7.1 Buffer Allocation Strategy

**Recommended for JUCE @ 48kHz:**

```cpp
// Generous allocation for modern plugin:
const int MAX_RECORDING_TIME_SECONDS = 8;  // vs. Clouds' ~3.75s
const int SAMPLE_RATE = 48000;

// Main recording buffer (stereo):
const int BUFFER_SIZE = SAMPLE_RATE * MAX_RECORDING_TIME_SECONDS;
// = 384,000 samples = 768,000 bytes (16-bit stereo)

// Grain count:
const int MAX_GRAINS_MONO = 64;   // vs. Clouds' 40
const int MAX_GRAINS_STEREO = 48; // vs. Clouds' 32

// Reverb buffer:
const int REVERB_BUFFER_SIZE = 16384 * 1.5;  // Scale to 48kHz
// = 24,576 samples

// Pitch shifter delay:
const int PITCH_SHIFTER_DELAY = 2047 * 1.5;
// = 3071 samples
```

### 7.2 Phase Implementation Plan

**Phase 4.1.2: Additional Modes**

1. **Mode 1: Time-Stretcher**
   - Implement WSOLA algorithm
   - 4096-sample windows
   - Correlation-based overlap
   - Complexity: MEDIUM

2. **Mode 2: Looping Delay**
   - Circular buffer looping
   - Tap-tempo sync
   - Integrate pitch shifter
   - Complexity: LOW-MEDIUM

3. **Mode 3: Spectral**
   - FFT/IFFT (4096 samples)
   - Phase vocoder STFT
   - Spectral transformations
   - Complexity: HIGH (consider JUCE FFT)

**Phase 4.1.3: Effects & Polish**

1. **Reverb**
   - Implement Dattorro/Griesinger topology
   - 8 allpass diffusers
   - Modulated delay lines with LFOs
   - Complexity: MEDIUM-HIGH

2. **Pitch Shifter**
   - Phase-modulation delay line
   - Dual-read crossfading
   - 3071-sample buffer @ 48kHz
   - Complexity: MEDIUM

3. **Filters & Feedback**
   - High-pass in feedback path
   - Dynamic cutoff calculation
   - Complexity: LOW

### 7.3 Grain Size Table

**Create lookup table based on perceptual scaling:**

```cpp
// Suggested grain size range for plugin:
const float MIN_GRAIN_MS = 10.0f;   // Tight grains
const float MAX_GRAIN_MS = 500.0f;  // Long grains

// Exponential/logarithmic curve for musical scaling:
float grain_size_ms = MIN_GRAIN_MS * pow(MAX_GRAIN_MS / MIN_GRAIN_MS, size);

// Convert to samples:
int grain_samples = (int)(grain_size_ms * 0.001f * sample_rate);
```

### 7.4 Parameter Value Ranges (JUCE AudioParameterFloat)

```cpp
// POSITION: 0.0 to 1.0, default 0.5
// SIZE: 0.0 to 1.0, default 0.5 (= ~100ms grains)
// PITCH: -24.0 to +24.0 semitones, default 0.0
// DENSITY: 0.0 to 1.0, default 0.5
// TEXTURE: 0.0 to 1.0, default 0.5
// BLEND: 0.0 to 1.0, default 0.5 (dry/wet)
// SPREAD: 0.0 to 1.0, default 0.5
// FEEDBACK: 0.0 to 1.0, default 0.0
// REVERB: 0.0 to 1.0, default 0.0
// FREEZE: 0.0 (off) or 1.0 (on)
// MODE: 0 (Granular), 1 (Stretch), 2 (Looping), 3 (Spectral)
// QUALITY: 0 (Vintage 32kHz), 1 (Modern 48kHz)
// INPUT_GAIN: -12.0 to +12.0 dB, default 0.0
```

### 7.5 Critical Implementation Notes

1. **Atomic Communication:**
   - Use `std::atomic<float>` for meter levels
   - Use `std::atomic<int>` for active grain count
   - Already implemented in current CloudWash code

2. **Envelope Tables:**
   - Pre-calculate Hann, Triangle, Exponential windows
   - 4097-sample resolution for smooth interpolation
   - Already implemented in current CloudWash code

3. **Interpolation:**
   - Use cubic interpolation for grain playback (HIGH quality)
   - Use linear for MEDIUM quality
   - Current code supports this

4. **Freeze Mode:**
   - Stop recording, continue playback
   - Current implementation correct

5. **Reverb Integration:**
   - Post-process after all modes
   - Separate wet/dry mixing
   - Use Dattorro topology for authenticity

---

## 8. KEY DIFFERENCES: CLOUDS vs. CLOUDWASH

| Aspect | Clouds (Hardware) | CloudWash (Plugin) |
|--------|-------------------|-------------------|
| **Sample Rate** | 32 kHz | 48 kHz (Modern) / 32 kHz (Vintage) |
| **Buffer Size** | ~120 KB (3.75s) | 768 KB (8s @ 48kHz) |
| **Max Grains** | 40 (mono) / 32 (stereo) | 64 (mono) / 48 (stereo) |
| **Resolution** | 8/16-bit, μ-law | 32-bit float |
| **Reverb Buffer** | 16,384 samples | 24,576 samples (scaled) |
| **CPU Budget** | Limited (Eurorack) | Generous (modern CPU) |
| **UI** | 4 knobs + CV | 13 parameters + WebView UI |
| **Memory** | 180 KB total | Unlimited (practical) |

---

## 9. SOURCES & REFERENCES

**Primary Source Code:**
- https://github.com/pichenettes/eurorack/tree/master/clouds/dsp
- `granular_processor.h` / `granular_processor.cc` - Main engine
- `grain.h` - Grain structure
- `granular_sample_player.h` - Grain scheduling
- `wsola_sample_player.h` - Time-stretcher
- `looping_sample_player.h` - Looping delay
- `pvoc/phase_vocoder.h` - Spectral processing
- `pvoc/stft.h` - FFT implementation
- `fx/reverb.h` - Reverb topology
- `fx/pitch_shifter.h` - Pitch shifting
- `fx/diffuser.h` - Diffuser stages
- `audio_buffer.h` - Buffer management
- `parameters.h` - Parameter structures
- `resources.h` / `resources.cc` - Lookup tables and constants

**Algorithmic References:**
- Dattorro reverb topology (Griesinger implementation)
- WSOLA (Waveform Similarity Overlap-Add)
- STFT (Short-Time Fourier Transform)
- Phase vocoder spectral processing

**CloudWash Current Status:**
- Phase 4.1.1: COMPLETE (Core infrastructure + Mode 0)
- Phase 4.1.2: PENDING (Modes 1-3)
- Phase 4.1.3: PENDING (Reverb, pitch shifter, effects)

---

## 10. NEXT STEPS FOR CLOUDWASH IMPLEMENTATION

### Immediate Priorities (Phase 4.1.2):

1. **Mode 1: WSOLA Time-Stretcher**
   - Difficulty: MEDIUM
   - Estimated Time: 4-6 hours
   - Key Files: Create `WSOLAProcessor.h/cpp`
   - Dependencies: Correlation algorithm, dual-window overlap-add

2. **Mode 2: Looping Delay**
   - Difficulty: LOW-MEDIUM
   - Estimated Time: 2-3 hours
   - Key Files: Create `LoopingDelayProcessor.h/cpp`
   - Dependencies: Circular buffer (already exists), tap tempo

3. **Mode 3: Spectral (Phase Vocoder)**
   - Difficulty: HIGH
   - Estimated Time: 8-12 hours
   - Key Files: Create `SpectralProcessor.h/cpp`
   - Dependencies: JUCE FFT, STFT framework, spectral transformations

### Phase 4.1.3 Priorities:

1. **Reverb (Dattorro)**
   - 8 allpass diffusers
   - Modulated delay lines
   - LFO implementation
   - Est. Time: 6-8 hours

2. **Pitch Shifter**
   - Phase-modulation delay line
   - Crossfading algorithm
   - Est. Time: 3-4 hours

3. **Effects Integration**
   - Feedback high-pass filter
   - Stereo spread refinement
   - Final mixing/routing
   - Est. Time: 2-3 hours

### Total Estimated Effort:
- Phase 4.1.2: 14-21 hours
- Phase 4.1.3: 11-15 hours
- **Total: 25-36 hours** of focused development

---

**End of Analysis**

This document provides complete implementation specifications extracted from the Mutable Instruments Clouds source code for implementing all four processing modes in the CloudWash JUCE plugin.
