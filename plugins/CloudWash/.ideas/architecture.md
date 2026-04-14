# CloudWash - DSP Architecture Specification

## Overview
CloudWash is a granular texture synthesizer based on Mutable Instruments Clouds, featuring 4 processing modes with shared parameter control but different DSP routing. The architecture must support real-time buffer manipulation, multi-mode processing, and quality-dependent configurations.

---

## Core Components

### 1. Input Processing Chain
- **Stereo Input Stage**: Handles stereo input with mono fallback (mirror left to right)
- **Sample Rate Converter (Input)**: Downsamples to 32kHz when in Vintage mode, bypassed in Modern mode
- **Input Gain Stage**: Applies `in_gain` parameter (0.0-1.0 scaling)
- **Input Buffer Manager**: Circular buffer (256 stereo frames) for grain source material
- **Freeze Controller**: Schmitt trigger-based freeze functionality to stop/resume recording

### 2. Granular Engine Core
- **Grain Scheduler**: Manages grain triggering based on `density` parameter
- **Grain Generator**: Creates individual grains with envelope shaping
- **Position Controller**: Reads from buffer at position specified by `position` parameter
- **Size Controller**: Determines grain window length via `size` parameter
- **Texture Processor**: Mode-dependent grain shaping (envelope, formant, diffusion, smearing)

### 3. Pitch Processing Module
- **Pitch Shifter**: ±2 octave range with 1V/oct scaling
- **Time Stretcher**: Independent time-stretching for pitch mode
- **Formant Preservation**: Controlled by `texture` parameter in pitch mode

### 4. Multi-Mode Processor
- **Mode Selector**: 4-state switch (Granular, Pitch-shifter, Looping Delay, Spectral)
- **Mode-Specific Routing**: Different signal paths per mode
- **Blend Function Router**: Context-dependent blend parameter behavior

### 5. Modulation & Effects
- **Stereo Spread Processor**: Panning/width control via `spread` parameter
- **Feedback Network**: Recirculation for delay/reverb modes
- **Reverb Engine**: Internal reverb for textural processing
- **Blend/Mix Stage**: Wet/dry or mode-specific blend control

### 6. Output Processing
- **Output Gain Stage**: Final level control
- **Quality Processor**: Handles bit depth reduction and additional downsampling for Quality modes 2-3
- **Sample Rate Converter (Output)**: Upsamples from 32kHz to host rate when in Vintage mode, bypassed in Modern mode
- **Stereo Output**: Final stereo output with clipping protection

### 7. State Management
- **Sample Mode Manager**: Switches between Vintage (32kHz) and Modern (native) processing rates
- **Quality Configuration Manager**: Manages 4 quality presets (buffer size, bit depth, additional downsampling)
- **Buffer Memory Manager**: Allocates/deallocates buffers based on quality and sample mode settings
- **Sample Rate Converter Manager**: Controls input/output SRC bypass state based on sample mode
- **Parameter Smoothing**: Prevents zipper noise on all continuous parameters
- **Freeze State Machine**: Manages freeze toggle and buffer looping

---

## Processing Chain

### Main Signal Flow
```
Stereo Input (Host Sample Rate: 44.1-192kHz)
  → [Sample Rate Converter: Host → 32kHz (Vintage mode only)]
  → Input Gain Stage
  → Input Buffer Manager (256 frames)
  → [Freeze Controller branches here]
  → Mode Router (processes at internal rate: 32kHz Vintage or Native Modern):
     ├─ Mode 0 (Granular): Grain Scheduler → Grain Generator → Texture (Envelope) → Blend (Wet/Dry)
     ├─ Mode 1 (Pitch): Pitch Shifter → Time Stretcher → Texture (Formant) → Blend (Spread)
     ├─ Mode 2 (Looping Delay): Delay Line → Texture (Diffusion) → Blend (Feedback) → Feedback Network
     └─ Mode 3 (Spectral): FFT Analysis → Spectral Processor → Texture (Smearing) → Blend (Reverb)
  → Stereo Spread Processor
  → Quality Processor (bit-reduction / additional downsampling for Quality 2-3)
  → [Sample Rate Converter: 32kHz → Host (Vintage mode only)]
  → Output Clipping Protection
  → Stereo Output (Host Sample Rate)
```

### Block Processing Flow

**Vintage Mode (32kHz):**
```
Host audio @ 44.1-192kHz
  ↓
Downsample to 32kHz
  ↓
32-sample blocks @ 32kHz internal rate
  ↓
Input Buffer (256 stereo frames @ 32kHz = ~8ms)
  ↓
Grain Engine (processes in 32-sample chunks)
  ↓
Mode-specific processing
  ↓
Output buffer aggregation
  ↓
Upsample to host rate
```

**Modern Mode (Native):**
```
Host audio @ 44.1-192kHz (no conversion)
  ↓
Variable-size blocks @ host rate (scaled from 32-sample @ 32kHz)
  ↓
Input Buffer (256 stereo frames @ host rate)
  ↓
Grain Engine (processes at native rate)
  ↓
Mode-specific processing
  ↓
Output buffer aggregation (no conversion)
```

---

## Parameter Mapping

| Parameter | Primary Component | Secondary Component | Function | DSP Range | Display Range |
|-----------|------------------|---------------------|----------|-----------|---------------|
| `position` | Input Buffer Manager | Mode-dependent reader | Buffer read position | 0.0 - 1.0 | 0% - 100% |
| `size` | Grain Generator | Window size calculator | Grain window length (ms) | 0.0 - 1.0 | 1ms - 500ms |
| `pitch` | Pitch Shifter | Grain pitch modifier | Pitch shift amount | -2.0 - 2.0 | -2 oct - +2 oct |
| `density` | Grain Scheduler | Trigger frequency | Grains per second | 0.0 - 1.0 | 1 - 100 Hz |
| `texture` | Texture Processor | Mode-specific shaper | Mode-dependent shaping | 0.0 - 1.0 | 0% - 100% |
| `in_gain` | Input Gain Stage | Input scaling | Input level | 0.0 - 1.0 | -inf - 0 dB |
| `blend` | Blend Router | Mode-dependent mixer | Mode-specific blend | 0.0 - 1.0 | 0% - 100% |
| `spread` | Stereo Spread Processor | Pan/width control | Stereo imaging | 0.0 - 1.0 | Mono - Wide |
| `feedback` | Feedback Network | Recirculation amount | Feedback level | 0.0 - 1.0 | 0% - 100% |
| `reverb` | Reverb Engine | Reverb mix | Reverb amount | 0.0 - 1.0 | 0% - 100% |
| `mode` | Mode Router | Processing path selector | Mode selection | 0 - 3 | 4 states |
| `freeze` | Freeze Controller | Buffer recording toggle | Freeze state | 0 - 1 | Off/On |
| `quality` | Quality Config Manager | Buffer/bit depth | Quality preset | 0 - 3 | 4 presets |
| `sample_mode` | Sample Mode Manager | SRC bypass control | Processing rate | 0 - 1 | Vintage/Modern |

---

## Mode-Specific Routing

### Mode 0: Granular
- **Position**: Read position in buffer
- **Size**: Grain window length
- **Density**: Grain trigger frequency
- **Texture**: Grain envelope shape (Hann, Triangle, Exponential)
- **Blend**: Wet/dry mix
- **Primary Components**: Grain Scheduler, Grain Generator, Texture Processor (Envelope)

### Mode 1: Pitch-shifter/Time-stretcher
- **Position**: Time-stretch anchor point
- **Size**: Analysis window size
- **Pitch**: ±2 octave pitch shift
- **Texture**: Formant preservation amount
- **Blend**: Stereo spread
- **Primary Components**: Pitch Shifter, Time Stretcher, Formant Processor

### Mode 2: Looping Delay
- **Position**: Delay tap position
- **Size**: Delay buffer size
- **Density**: Diffusion density
- **Texture**: Diffusion amount
- **Blend**: Feedback amount
- **Primary Components**: Delay Line, Diffusion Network, Feedback Processor

### Mode 3: Spectral Processing
- **Position**: Frequency analysis window shift
- **Size**: FFT window size
- **Density**: Spectral grain density
- **Texture**: Frequency smearing amount
- **Blend**: Reverb amount
- **Primary Components**: FFT Engine, Spectral Processor, Frequency Smearing

---

## Sample Mode & Quality Configuration Matrix

### Sample Mode Settings

| Sample Mode | Processing Rate | CPU Impact | Audio Character | Recommendation |
|-------------|----------------|------------|-----------------|----------------|
| 0 (Vintage) | 32 kHz | Lower | Hardware-faithful, warm, slight aliasing | Authentic Clouds emulation |
| 1 (Modern) | Host rate (44.1-192 kHz) | Higher at 96kHz+ | Transparent, pristine high-freq response | Studio production, mastering |

### Quality Settings (Independent of Sample Mode)

**Note:** Buffer durations listed are for Vintage mode (32kHz). In Modern mode, durations scale proportionally.

| Quality | Buffer @ 32kHz | Buffer @ 48kHz (Modern) | Bit Depth | Lo-Fi Processing | Memory (32kHz) | Use Case |
|---------|---------------|------------------------|-----------|------------------|----------------|----------|
| 0 (High Fidelity) | 1 second | ~0.67 seconds | 16-bit stereo | None | ~128 KB | Short textures, pristine quality |
| 1 (Balanced) | 2 seconds | ~1.33 seconds | 16-bit stereo | None | ~256 KB | General purpose |
| 2 (Extended) | 4 seconds | ~2.67 seconds | 8-bit µ-law stereo | Downsampling to 16kHz + bit reduction | ~128 KB | Lo-fi character, longer buffer |
| 3 (Maximum) | 8 seconds | ~5.33 seconds | 8-bit µ-law mono | Aggressive downsample to 16kHz + bit reduction | ~128 KB | Maximum time, vintage lo-fi |

### Sample Mode + Quality Combinations

| Combination | Character | Best For |
|-------------|-----------|----------|
| **Vintage + Quality 0-1** | Hardware-accurate, clean | Authentic Clouds sound |
| **Vintage + Quality 2-3** | Hardware-accurate, lo-fi | Vintage tape/cassette aesthetic |
| **Modern + Quality 0-1** | Transparent, studio-grade | Professional production |
| **Modern + Quality 2-3** | Clean processing with lo-fi texture | Creative bit-crushing effects |

**Memory Management Strategy:**
- Pre-allocate maximum buffer size to avoid audio-thread allocation
- Use lockless ring buffers for real-time safety
- Implement µ-law compression for quality modes 2-3
- Sample rate conversion uses high-quality interpolation (linear phase FIR or similar)

---

## Complexity Assessment

**Score: 5/5 (Research-Level Complexity)**

### Rationale:

**Complex Multi-Mode Architecture (+2):**
- 4 distinct processing modes with shared parameter mapping
- Each mode requires different DSP routing and algorithms
- Mode-dependent parameter behavior (texture, blend)

**Advanced DSP Algorithms (+1.5):**
- Granular synthesis engine with real-time grain scheduling
- Pitch-shifting and time-stretching (phase vocoder or similar)
- Spectral processing (FFT-based)
- Reverb and diffusion networks

**Real-Time Buffer Management (+1):**
- Dynamic buffer allocation based on quality settings
- Lockless ring buffer for audio thread safety
- Freeze functionality with seamless recording toggle
- 256-frame circular buffer with variable read positions

**State Machine Complexity (+0.5):**
- Quality setting changes require buffer reallocation
- Sample mode switching requires SRC reconfiguration
- Freeze state transitions must be glitch-free
- Mode switching must handle parameter value context
- Parameter smoothing across all 10 continuous parameters

**Reference Implementation Available (mitigation):**
- Open-source VCV Rack Clouds implementation provides DSP reference
- Reduces "from scratch" complexity but porting still non-trivial

**Total Complexity Factors:**
- Multi-mode processor design
- Advanced granular synthesis
- Spectral processing (FFT)
- Real-time buffer management
- State machine coordination
- Parameter smoothing and automation
- Quality-dependent configuration

This is a **research-level plugin** requiring deep DSP knowledge, careful real-time design, and extensive testing across modes and quality settings.

---

## Critical Implementation Notes

### Real-Time Safety
- All buffer allocations must happen off audio thread
- Use atomic operations for freeze state
- Pre-allocate maximum buffer size to avoid allocation during processing
- Lock-free ring buffer for input recording

### Parameter Smoothing Strategy
- Use exponential smoothing for all continuous parameters
- Smoothing time: 10-50ms to prevent zipper noise
- Critical for: position, size, pitch, density, texture

### Mode Switching
- Crossfade between modes to avoid clicks (50-100ms)
- Preserve buffer contents during mode switch
- Reset internal state machines when mode changes

### FFT Considerations (Mode 3)
- Use power-of-2 FFT sizes (512, 1024, 2048, 4096)
- Overlap-add reconstruction for smooth output
- Window function selection based on texture parameter

### Grain Engine Optimization
- Pre-calculate envelope tables
- Use SIMD for grain mixing when possible
- Limit max concurrent grains (e.g., 64) for CPU efficiency

### Sample Rate Conversion (Vintage Mode)
- **Input SRC**: Use high-quality downsampling (linear phase FIR or polyphase)
- **Output SRC**: Use high-quality upsampling to restore to host rate
- **Latency**: Report combined SRC latency to host DAW
- **Bypass**: In Modern mode, SRC is completely bypassed (zero-copy)
- **Quality**: Use JUCE's `juce::Interpolators` or custom high-quality resampler
- **CPU Trade-off**: SRC adds overhead but enables authentic 32kHz character

---

## JUCE Module Dependencies

**Required:**
- `juce_audio_basics` - Audio buffer management
- `juce_audio_processors` - Plugin framework
- `juce_dsp` - DSP utilities, filters, FFT

**Recommended:**
- `juce_gui_basics` - For custom UI (if Visage not used)
- `juce_gui_extra` - For WebView2 integration (if WebView framework)

**Optional:**
- `juce_audio_utils` - Additional audio utilities
- Custom granular engine library (or port from Clouds source)

---

## Risk Assessment

### High Risk (Requires Careful Implementation)
- **Granular engine complexity**: Real-time grain scheduling with dynamic density
- **Multi-mode state management**: Mode switching without glitches
- **Spectral processing**: FFT-based processing with low latency
- **Freeze functionality**: Seamless buffer recording toggle without clicks
- **Quality changes**: Buffer reallocation without audio dropouts

### Medium Risk
- **Parameter smoothing**: Preventing zipper noise on 10 continuous parameters
- **Pitch-shifting**: Artifacts at extreme pitch values
- **Memory management**: Buffer allocation for different quality modes
- **Feedback stability**: Preventing runaway feedback in delay/reverb modes

### Low Risk
- **Input/output gain stages**: Standard linear scaling
- **Stereo spread**: Basic panning/width algorithms
- **Mode routing**: Straightforward conditional processing
- **Parameter normalization**: Standard JUCE parameter handling

---

## Next Steps (Implementation Phase)

1. **Phase 2.1.1: Core Infrastructure**
   - Implement quality configuration manager
   - Build circular buffer with freeze control
   - Create parameter smoothing system
   - Set up mode routing framework

2. **Phase 2.1.2: Mode Implementation**
   - Mode 0: Granular engine (grain scheduler, generator, envelopes)
   - Mode 1: Pitch-shifter/time-stretcher (phase vocoder or equivalent)
   - Mode 2: Looping delay (delay line, diffusion, feedback)
   - Mode 3: Spectral processor (FFT engine, frequency manipulation)

3. **Phase 2.1.3: Effects & Polish**
   - Stereo spread processor
   - Reverb engine
   - Feedback network
   - Mode transition crossfading
   - Edge case handling and testing

---

## Reference Materials

- **Original Hardware**: Mutable Instruments Clouds eurorack module
- **Source Code**: https://github.com/VCVRack/AudibleInstruments/blob/v2/src/Clouds.cpp
- **DSP Algorithms**:
  - Granular synthesis: Roads, Curtis "Microsound" (2001)
  - Phase vocoder: Laroche & Dolson "Improved Phase Vocoder" (1999)
  - Spectral processing: Frequency-domain techniques from MI Clouds
