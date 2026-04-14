# CloudWash - Implementation Plan

## Complexity Score: 5/5 (Research-Level)

**Justification:**
CloudWash is a multi-mode granular processor with 4 distinct DSP algorithms, advanced buffer management, spectral processing, and real-time state management. While the VCV Rack source provides a reference implementation, porting and optimizing this for VST requires expert-level DSP knowledge and careful real-time design.

---

## Implementation Strategy: Phased Implementation (Required for Score ≥3)

Given the high complexity, implementation will be broken into 3 major phases with incremental testing and validation at each stage.

---

## Phase 2.1.1: Core Infrastructure & Mode 0 (Granular)

**Goal:** Establish the foundational systems and implement the primary granular mode.

### Tasks:
- [ ] **Sample Mode Manager**
  - Implement Vintage (32kHz) vs Modern (native) processing rate switch
  - Build high-quality sample rate converter (input: host→32kHz, output: 32kHz→host)
  - Use JUCE interpolators or custom polyphase resampler
  - Add SRC bypass logic for Modern mode (zero-copy)
  - Report latency to host DAW when in Vintage mode
  - Test SRC quality and verify no aliasing artifacts
  - Test mode switching during playback (should be seamless)

- [ ] **Quality Configuration Manager**
  - Implement 4 quality presets with buffer size and bit depth settings
  - Update to work with Sample Mode (buffer sizes scale with sample rate in Modern mode)
  - Build buffer allocation system (off audio thread)
  - Add µ-law compression for quality modes 2-3
  - Test quality switching without audio dropouts

- [ ] **Circular Buffer with Freeze**
  - Create lockless ring buffer (256 stereo frames)
  - Implement Schmitt trigger-based freeze controller
  - Add seamless record/freeze state transitions
  - Test buffer wrapping and freeze toggle

- [ ] **Parameter Smoothing System**
  - Build exponential smoothing for all 10 continuous parameters
  - Set smoothing time: 10-50ms to prevent zipper noise
  - Test automation and rapid parameter changes

- [ ] **Mode Routing Framework**
  - Create mode selector with 4-way routing
  - Implement mode crossfading (50-100ms) to prevent clicks
  - Build parameter context system for mode-dependent behavior
  - Test mode switching with audio playing

- [ ] **Granular Engine (Mode 0)**
  - Build grain scheduler (density-based triggering, 1-100 Hz)
  - Implement grain generator with window size control (1-500ms)
  - Create texture processor with envelope shaping (Hann, Triangle, Exponential)
  - Add position-based buffer reading
  - Implement pitch parameter for grain pitch shift (±2 octaves)
  - Add blend control (wet/dry mix)
  - Optimize: Pre-calculate envelope tables, limit max concurrent grains (64)
  - Test: Individual grains, density sweeps, position/size/texture modulation

### Validation Criteria:
- Quality switching works without glitches
- Freeze toggle is seamless and click-free
- Granular mode produces expected textures
- No parameter zipper noise
- CPU usage within acceptable range (<20% on reference system)

### Estimated Milestone:
**Core infrastructure + Mode 0 functional**

---

## Phase 2.1.2: Additional Modes (1, 2, 3)

**Goal:** Implement the remaining 3 processing modes with full parameter integration.

### Tasks:

#### Mode 1: Pitch-shifter/Time-stretcher
- [ ] **Pitch Shifting Engine**
  - Implement phase vocoder or equivalent algorithm
  - ±2 octave range with 1V/oct scaling
  - Low-latency design (target <20ms)

- [ ] **Time Stretching**
  - Independent time-stretching control via position parameter
  - Preserve transients and formants

- [ ] **Texture (Formant Preservation)**
  - Implement formant-preserving algorithm
  - Texture parameter controls preservation amount (0% = robot, 100% = natural)

- [ ] **Blend (Stereo Spread)**
  - Route blend parameter to stereo spread processor
  - Implement width control (0 = mono, 1 = wide stereo)

- [ ] **Testing**
  - Test pitch shifts at extremes (-2, +2 octaves)
  - Verify formant preservation on vocal material
  - Check for artifacts and glitches

#### Mode 2: Looping Delay
- [ ] **Delay Line**
  - Build variable-length delay buffer
  - Position parameter controls tap position
  - Size parameter controls buffer size

- [ ] **Diffusion Network**
  - Implement all-pass diffusion
  - Density parameter controls diffusion complexity
  - Texture parameter controls diffusion amount

- [ ] **Feedback Processor**
  - Blend parameter routes to feedback amount
  - Implement feedback stability limiting (prevent runaway)
  - Add high-frequency damping for natural decay

- [ ] **Testing**
  - Test feedback stability at 100%
  - Verify diffusion creates textural complexity
  - Check for metallic ringing or artifacts

#### Mode 3: Spectral Processing
- [ ] **FFT Engine**
  - Implement FFT (power-of-2 sizes: 512, 1024, 2048, 4096)
  - Size parameter controls FFT window size
  - Use overlap-add reconstruction for smooth output

- [ ] **Frequency Analysis**
  - Position parameter shifts frequency analysis window
  - Implement spectral grain extraction

- [ ] **Spectral Manipulation**
  - Texture parameter controls frequency smearing
  - Density parameter controls spectral grain density
  - Blend parameter routes to reverb amount

- [ ] **Reverb Engine**
  - Implement reverb for spectral mode
  - Blend parameter controls reverb mix

- [ ] **Testing**
  - Test FFT at different window sizes
  - Verify spectral smearing produces "madness" effect
  - Check phase coherence and avoid artifacts

### Validation Criteria:
- All 4 modes functional and switchable
- Mode-specific parameter behaviors work correctly
- No CPU spikes when switching modes
- Each mode produces expected sonic character
- Blend parameter routes correctly per mode

### Estimated Milestone:
**All 4 modes implemented and tested**

---

## Phase 2.1.3: Effects, Optimization, and Polish

**Goal:** Add final effects, optimize performance, and handle edge cases.

### Tasks:

- [ ] **Stereo Spread Processor**
  - Implement global stereo spread control
  - Spread parameter creates panning/width effect
  - Works in all modes

- [ ] **Feedback Network (Global)**
  - Implement global feedback routing
  - Feedback parameter affects delay/reverb modes primarily
  - Add stability limiting

- [ ] **Reverb Engine (Global)**
  - Build or integrate reverb algorithm
  - Reverb parameter controls global reverb amount
  - Optimize for low CPU usage

- [ ] **Mode Transition Crossfading**
  - Implement 50-100ms crossfade when mode changes
  - Prevent clicks and pops during transitions
  - Preserve buffer contents across mode changes

- [ ] **Edge Case Handling**
  - Test extreme parameter values (0.0, 1.0, -2.0, +2.0)
  - Handle denormal numbers (add DC offset or use JUCE denormal prevention)
  - Test freeze with empty buffer
  - Test quality changes during playback
  - Verify mono input handling (mirror to right channel)

- [ ] **Performance Optimization**
  - Profile CPU usage per mode
  - Optimize grain mixing with SIMD if needed
  - Reduce memory allocations in audio thread
  - Target: <30% CPU on reference system at 48kHz, 512 samples

- [ ] **State Management**
  - Implement state save/load for DAW session recall
  - Save buffer contents if freeze is active
  - Restore mode, quality, and all parameters correctly

- [ ] **Parameter Validation**
  - Ensure all parameters stay within valid ranges
  - Implement input clamping where necessary
  - Verify parameter smoothing doesn't overshoot

- [ ] **Testing & Debugging**
  - Test in multiple DAWs (Reaper, Ableton, FL Studio)
  - Verify automation works smoothly
  - Test freeze functionality extensively
  - Check for memory leaks with Valgrind or similar
  - Stress test with long sessions

### Validation Criteria:
- All effects functional and integrated
- Mode transitions are seamless
- No crashes or audio dropouts under stress testing
- CPU usage within target range
- State save/load works perfectly
- Plugin passes AU/VST validation

### Estimated Milestone:
**Plugin fully functional, optimized, and polished**

---

## Dependencies

### Required JUCE Modules:
- `juce_audio_basics` - Core audio buffer and sample handling
- `juce_audio_processors` - AudioProcessor base class, parameters
- `juce_dsp` - DSP utilities, FFT, filters, interpolation
- `juce_core` - Threading, memory management, utilities

### Optional JUCE Modules:
- `juce_gui_basics` - For Visage UI framework
- `juce_gui_extra` - For WebView2 integration (if WebView selected)
- `juce_audio_utils` - Additional audio utilities

### External Dependencies:
- **VCV Rack Clouds source** (reference): https://github.com/VCVRack/AudibleInstruments/blob/v2/src/Clouds.cpp
- **Mutable Instruments Clouds firmware** (optional deep reference): https://github.com/pichenettes/eurorack

### Custom Code Requirements:
- Granular synthesis engine (port or adapt from Clouds)
- Phase vocoder for pitch-shifting (can use JUCE `juce_dsp::PhaseVocoder` or custom)
- µ-law compression/decompression for quality modes 2-3
- Spectral processing algorithms

---

## Risk Assessment

### High Risk (Critical Path)

**1. Granular Engine Complexity**
- **Risk:** Real-time grain scheduling with variable density is CPU-intensive
- **Mitigation:**
  - Limit max concurrent grains (64)
  - Use pre-calculated envelope tables
  - Implement grain pooling/recycling
  - Profile and optimize inner loops

**2. Multi-Mode State Management**
- **Risk:** Mode switching could cause glitches, clicks, or state corruption
- **Mitigation:**
  - Implement crossfading between modes
  - Use atomic operations for mode state
  - Test extensively with rapid mode changes
  - Preserve buffer contents across switches

**3. Spectral Processing Latency**
- **Risk:** FFT-based processing adds latency, may not be suitable for live use
- **Mitigation:**
  - Use smallest practical FFT sizes
  - Implement overlap-add efficiently
  - Report latency to host DAW
  - Consider real-time vs offline mode

**4. Freeze Functionality Glitches**
- **Risk:** Toggling freeze could cause clicks or buffer corruption
- **Mitigation:**
  - Use Schmitt trigger for hysteresis
  - Crossfade when entering/exiting freeze
  - Ensure thread-safe buffer access
  - Test freeze toggle during heavy processing

**5. Quality Changes During Playback**
- **Risk:** Changing quality setting requires buffer reallocation, could dropout
- **Mitigation:**
  - Pre-allocate maximum buffer size
  - Perform reallocation off audio thread
  - Fade out/in during quality change
  - Consider locking quality during playback

**6. Sample Rate Conversion Quality & CPU**
- **Risk:** Poor SRC can introduce aliasing, phase distortion. High-quality SRC adds CPU overhead (especially at 192kHz)
- **Mitigation:**
  - Use proven high-quality resampler (JUCE interpolators or custom polyphase)
  - Test for aliasing with sweep tones and noise
  - Profile CPU usage at different host sample rates
  - Consider adaptive SRC quality based on host rate
  - In Modern mode, completely bypass SRC for zero overhead
  - Report combined SRC latency accurately to host

### Medium Risk

**1. Parameter Smoothing Performance**
- **Risk:** Smoothing 10 parameters per sample could add CPU overhead
- **Mitigation:**
  - Use efficient exponential smoothing
  - Consider block-rate smoothing for less critical params
  - Profile and optimize

**2. Pitch-Shifting Artifacts**
- **Risk:** Phase vocoder can produce robotic or phasey artifacts
- **Mitigation:**
  - Tune analysis/synthesis window sizes
  - Implement formant preservation (texture param)
  - Test on variety of source material

**3. Feedback Stability**
- **Risk:** Feedback in delay/reverb modes could runaway
- **Mitigation:**
  - Implement soft clipping in feedback path
  - Add high-frequency damping
  - Limit max feedback to 95%

**4. Memory Management**
- **Risk:** Large buffers (8 seconds @ 32kHz) could fragment memory
- **Mitigation:**
  - Use pre-allocated buffer pools
  - Avoid allocations in audio thread
  - Test with memory profiling tools

### Low Risk

**1. Input/Output Gain**
- Straightforward linear scaling
- Low CPU impact

**2. Stereo Spread**
- Standard panning/width algorithms
- Well-documented implementations

**3. Mode Routing**
- Conditional processing paths
- Standard C++ control flow

**4. Parameter Normalization**
- JUCE handles parameter range mapping
- Minimal implementation risk

---

## Testing Strategy

### Unit Testing
- Test each DSP component in isolation
- Verify parameter ranges and clamping
- Check for denormals and NaN propagation
- Validate buffer wrapping and indexing

### Integration Testing
- Test mode switching with audio playing
- Verify parameter routing per mode
- Test freeze functionality across modes
- Validate quality changes

### Performance Testing
- Profile CPU usage per mode
- Measure latency (FFT mode)
- Test with various buffer sizes (64, 128, 256, 512, 1024)
- Stress test with automation on all parameters

### DAW Compatibility Testing
- **Windows:** Reaper, Ableton Live, FL Studio, Cubase
- **Mac:** Logic Pro, Ableton Live, Reaper (if Mac build planned)
- Verify automation works
- Test state save/load
- Check for crashes or hangs

### Sonic Validation
- Compare output to VCV Rack Clouds
- Test with variety of source material (drums, vocals, pads, noise)
- Verify each mode produces expected character
- Check for unwanted artifacts

---

## Success Criteria

**Phase 2.1.1 Complete:**
- [x] Quality system functional
- [x] Circular buffer with freeze works
- [x] Parameter smoothing prevents zipper noise
- [x] Mode routing framework operational
- [x] Granular mode (Mode 0) produces expected textures

**Phase 2.1.2 Complete:**
- [x] All 4 modes implemented
- [x] Mode-specific parameter behaviors work
- [x] No crashes or glitches when switching modes
- [x] Each mode sonically validated

**Phase 2.1.3 Complete:**
- [x] All effects integrated
- [x] CPU usage <30% (reference system)
- [x] No audio dropouts or clicks
- [x] State save/load works
- [x] Passes DAW validation in 3+ DAWs

**Final Deliverable:**
- Fully functional CloudWash VST3 plugin
- All 4 modes operational
- 13 parameters fully automatable
- Stable, optimized, and DAW-ready

---

## Implementation Timeline (Relative)

**Phase 2.1.1:** ~40% of implementation effort
- Core infrastructure is foundational
- Granular mode is most complex single mode

**Phase 2.1.2:** ~40% of implementation effort
- 3 additional modes with unique DSP
- Spectral mode (FFT) is most complex

**Phase 2.1.3:** ~20% of implementation effort
- Integration and polish
- Bug fixes and optimization

**Total Estimated Effort:** Expert-level DSP developer, multiple weeks of focused work

---

## Next Phase

After planning is complete and approved, proceed to:

**Phase 3: DESIGN** - Create UI mockups, layout specifications, and visual design
- Command: `/design CloudWash`
- Will select UI framework (Visage vs WebView)
- Will define control layout, visual style, and interaction design
