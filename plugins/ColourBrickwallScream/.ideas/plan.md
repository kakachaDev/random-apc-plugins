# Implementation Plan: ColourBrickwallScream

## Complexity Score: 4 / 5

## UI Framework: WebView
**Rationale:** The industrial aesthetic — cracked concrete textures, glowing heat-element meters, strained VU dynamics, illuminated character selectors — is far more achievable and maintainable in HTML5 Canvas/CSS than in Visage. The rich visual language (animated metering, glowing states, character indicator highlights) maps directly to CSS/canvas capabilities. Performance is not a bottleneck here since DSP runs in C++ and the UI is display-only polling.

---

## Implementation Strategy: Phased (Score ≥ 3)

---

### Phase 4.1: Core DSP Foundation
**Goal:** Get clean audio flowing through the full signal chain with working parameters.

- [ ] `PluginProcessor.h/cpp` skeleton — JUCE AudioProcessor boilerplate
- [ ] APVTS (AudioProcessorValueTreeState) setup with all 8 parameters
- [ ] DriveGain stage — `juce::dsp::Gain`, log-taper mapping, parameter smoothing
- [ ] Saturation Engine — implement all 6 transfer function curves (Tube, Tape, Transformer, Diode, Bitcrush, Full Scream) as inline functions
- [ ] Temper switching — 4ms crossfade on character change, no clicks
- [ ] DryBuffer — delay-compensated copy buffer for parallel Mix blend
- [ ] Basic output routing — OutputGain + MixBlend passthrough

**Acceptance:** Plugin loads in DAW, Drive knob works, all 6 Temper variants produce distinct audio character with no clicks on switching.

---

### Phase 4.2: Colour, Tone, and Limiter
**Goal:** Complete the signal chain with tonal shaping and hard ceiling.

- [ ] Tone Stage — tilt EQ biquad, ±6dB at 1kHz pivot, `juce::dsp::IIR::Filter`
- [ ] Colour Stage — 6 per-variant biquad configurations, wet blend
- [ ] Brickwall Limiter — lookahead buffer (2ms), attack/release envelope, log-domain gain reduction
- [ ] True-peak detection — 4x oversampled via `juce::dsp::Oversampling`
- [ ] Ceiling parameter binding — maps dBFS to gain reduction threshold
- [ ] Attack Character mapping — log taper 0.1ms–50ms
- [ ] MixBlend latency compensation — DryBuffer aligned to limiter lookahead
- [ ] DAW latency reporting — `getLatencySamples()` returns correct lookahead value

**Acceptance:** Ceiling clamps output hard at set dBFS with true-peak accuracy. Tone and Colour parameters audibly shape output. Mix blends clean parallel signal correctly. Latency reported to DAW.

---

### Phase 4.3: Metering Infrastructure
**Goal:** Thread-safe metering ready for UI consumption.

- [ ] InputMeter — pre-drive peak+RMS with ballistic smoothing
- [ ] GRMeter — reads limiter gain reduction per block
- [ ] OutputMeter — post-output-gain peak+RMS
- [ ] CharacterDisplay — atomic int for active variant
- [ ] All meters exposed as `std::atomic<float>` polled by GUI timer

**Acceptance:** All meter values update correctly in real-time. No data races. GUI can poll at 30fps without stutter.

---

### Phase 4.4: WebView Editor & UI Binding
**Goal:** Full UI operational with all parameters wired.

- [ ] `PluginEditor.h/cpp` — `juce::WebBrowserComponent` setup
- [ ] `Design/index.html` — industrial UI: dark background, glowing elements, canvas meters
- [ ] Drive/Scream knob
- [ ] Character selector — 6 illuminated variant buttons (Tube/Tape/Transformer/Diode/Bitcrush/Full Scream)
- [ ] Tone knob (bipolar)
- [ ] Colour knob
- [ ] Attack Character knob
- [ ] Ceiling knob
- [ ] Output Gain knob
- [ ] Mix knob
- [ ] Input meter bar (canvas animated)
- [ ] GR meter bar (canvas animated, red accent)
- [ ] Output meter bar (canvas animated)
- [ ] JS↔JUCE parameter bridge via `window.__JUCE__`
- [ ] Parameter automation recall on plugin reload

**Acceptance:** All knobs move in sync with DAW automation. Character selector highlights active variant. Meters animate in real-time. Plugin saves and recalls state correctly.

---

### Phase 4.5: Polish & Hardening
**Goal:** Production-quality stability and edge case handling.

- [ ] Parameter smoothing audit — no zippers on any control
- [ ] Silence detection — bypass saturation chain on digital silence to save CPU
- [ ] Sample rate changes — all biquad coefficients recalculated on `prepareToPlay`
- [ ] Block size edge cases — minimum 1 sample, maximum 4096+
- [ ] Mono/stereo — process each channel independently, same parameter state
- [ ] DAW compatibility pass — test in Reaper, Ableton, Logic (if macOS)
- [ ] CPU profiling — target <3% CPU on a modern machine at 44.1kHz/512 samples

**Acceptance:** No crashes, no audio glitches under stress. CPU within target. Passes DAW compatibility checks.

---

## Dependencies

**Required JUCE Modules:**
- `juce_audio_basics`
- `juce_audio_processors`
- `juce_dsp`
- `juce_gui_basics`
- `juce_gui_extra` (WebBrowserComponent)

**CMake Flags:**
- `NEEDS_WEB_BROWSER TRUE` (Linux)
- `NEEDS_WEBVIEW2 TRUE` (Windows)
- `JUCE_WEB_BROWSER=1` compile definition

---

## Risk Assessment

**High Risk:**
- **True-peak brickwall limiter** — correct lookahead + oversampled detection is tricky to get right without over- or under-shooting the ceiling. Must validate with ITU-R BS.1770 test signals.
- **Temper switching crossfade** — 4ms crossfade between 6 different nonlinear functions with no CPU spike or click requires careful buffer management.
- **Latency compensation for Mix** — DryBuffer must be exactly aligned with limiter lookahead or parallel blend will have comb-filtering artefacts.

**Medium Risk:**
- **Per-variant Colour biquad configurations** — 6 sets of filter coefficients; some may need tuning by ear after implementation.
- **WebView parameter bridge latency** — `window.__JUCE__` polling at 30fps should be fine but may need throttling if UI is complex.
- **Bitcrush Temper variant** — aliasing fold behaviour changes with sample rate; needs rate-normalised bit-depth mapping.

**Low Risk:**
- **Drive stage, Tone stage, Output Gain, Mix blend** — straightforward JUCE DSP primitives.
- **Meter atomics** — standard pattern in JUCE plugins.
- **Character selector UI** — CSS state + JS event, trivial to implement.
