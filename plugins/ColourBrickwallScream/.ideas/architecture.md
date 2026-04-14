# DSP Architecture Specification: ColourBrickwallScream

## Core Components

### 1. Input Stage
- **InputGain:** Unity-gain passthrough. Used as metering tap before Drive is applied.
- **DryBuffer:** Latency-compensated copy of clean input signal for parallel Mix blend at output.

### 2. Drive Stage
- **DriveGain:** Linear pre-gain, mapped from normalised [0.0–1.0] to [0–40 dB]. Applied before the saturation curve. Smoothed to avoid clicks on automation.

### 3. Saturation Engine (Temper Stage)
The core of the "Scream." Six distinct transfer function curves, selected by the `character` enum parameter. Each is a waveshaping function applied per-sample:

| Variant | Transfer Function | Notes |
| :--- | :--- | :--- |
| **Tube** | `y = (3x/2)(1 - x²/3)` — Devereux soft-clip | Even harmonics, soft knee, tanh-like but asymmetric |
| **Tape** | `y = tanh(x) + subtle HF lowpass post-clip` | Dominant 2nd harmonic; LPF at ~14kHz at high drive |
| **Transformer** | `y = x / (1 + |x|^0.7)` asymmetric | Low-end weight via DC-coupled soft clip; 3rd harmonic bloom |
| **Diode** | `y = hardclip(x) with asymmetric threshold` | Upper clip at 0.9, lower clip at -0.75; odd harmonics + presence |
| **Bitcrush** | `y = round(x * bits) / bits` + aliasing fold | Bit depth: 4–16 bits mapped from Drive; high-order harmonics |
| **Full Scream** | Wavefolding: `y = sin(π * x * drive)` | Asymmetric fold with Drive-scaled period; full harmonic stack |

All variants: smoothed switching (crossfade over 4ms on character change to prevent clicking).

### 4. Colour Stage
Applied after saturation, before Tone EQ. The Colour knob blends in a **Character-dependent resonance layer**:

| Variant | Colour Effect |
| :--- | :--- |
| Tube | Adds low-mid warmth (200–400Hz gentle boost) |
| Tape | Adds presence air (8–12kHz subtle shelve lift) |
| Transformer | Adds mid-range bloom (500Hz–1.5kHz resonant peak) |
| Diode | Adds upper-mid bite (3–5kHz presence push) |
| Bitcrush | Adds aliasing overtone resonance (variable high-end grit) |
| Full Scream | Adds subharmonic fold (–1 octave ring modulation blend) |

Implementation: per-variant IIR biquad filter with `colour` controlling wet blend amount (0 = off, 1 = full colour gain from table).

### 5. Tone Stage
Post-saturation tilt EQ. A first-order shelving network pivoting at 1 kHz:
- `tone < 0`: Low-shelf boost + high-shelf cut (darker)
- `tone > 0`: High-shelf boost + low-shelf cut (brighter)
- `tone == 0`: Unity pass-through

Implemented as a single biquad shelf filter; max ±6dB at extremes.

### 6. Brickwall Limiter
True-peak lookahead brickwall limiter:
- **Lookahead buffer:** 2ms fixed (64 samples at 48kHz; scaled at runtime)
- **Attack time:** Mapped from `attack_character` [0.0–1.0] → [0.1ms–50ms]. The attack time controls how aggressively transients are caught before they breach the ceiling.
- **Release time:** Fixed at 200ms (non-user-adjustable in v1)
- **Ceiling:** True peak ceiling in dBFS. Uses 4x oversampled peak detection via juce::dsp::Oversampling.
- **Gain Reduction:** Computed in log domain; smoothed attack/release envelope applied to gain reduction signal.

### 7. Output Stage
- **OutputGain:** Post-limiter makeup gain ±12dB, applied after ceiling.
- **MixBlend:** Parallel dry/wet crossfade between DryBuffer and processed signal. The DryBuffer is delay-compensated to match limiter lookahead latency.

### 8. Metering
- **InputMeter:** Peak+RMS tap before DriveGain.
- **GRMeter:** Reads gain reduction value from limiter each block.
- **OutputMeter:** Peak+RMS tap after OutputGain, before final output.
- **CharacterDisplay:** Enum indicator — highlights active Temper Variant in UI.

All meters: ballistic smoothing with configurable fallback rate. Written to `std::atomic<float>` for thread-safe GUI polling.

---

## Processing Chain

```
Input
  │
  ├──► [DryBuffer tap] ──────────────────────────────────────────────────────┐
  │                                                                           │
  ├──► [InputMeter tap]                                                       │
  │                                                                           │
  └──► DriveGain (0–40dB pre-gain)                                           │
          │                                                                   │
          └──► Saturation Engine (Temper Variant: Tube/Tape/.../Full Scream)  │
                  │                                                           │
                  └──► Colour Stage (per-variant biquad blend)               │
                          │                                                   │
                          └──► Tone Stage (tilt EQ, ±6dB at 1kHz pivot)     │
                                  │                                           │
                                  └──► Brickwall Limiter (lookahead 2ms)     │
                                          │                                   │
                                          ├──► [GRMeter tap]                 │
                                          │                                   │
                                          └──► OutputGain (±12dB makeup)     │
                                                  │                           │
                                                  ├──► [OutputMeter tap]     │
                                                  │                           │
                                                  └──► MixBlend ◄────────────┘
                                                          │
                                                        Output
```

---

## Parameter Mapping

| Parameter | Stage | DSP Function | Notes |
| :--- | :--- | :--- | :--- |
| `drive` | DriveGain | Pre-saturation gain: 0.0→0dB, 1.0→40dB | Log taper; smoothed |
| `character` | Saturation Engine | Selects transfer function curve | Crossfade on change |
| `colour` | Colour Stage | Blend amount for per-variant biquad | Linear taper |
| `tone` | Tone Stage | Tilt EQ centre ±6dB | Linear bipolar |
| `attack_character` | Brickwall Limiter | Attack time: 0.1ms–50ms | Log taper |
| `ceiling` | Brickwall Limiter | True-peak ceiling dBFS | Linear dB |
| `output_gain` | OutputGain | Post-limiter gain ±12dB | Linear dB |
| `mix` | MixBlend | Wet/dry ratio | Linear; DryBuffer latency-compensated |

---

## Complexity Assessment

**Score: 4 / 5**

**Rationale:**
- **Six distinct transfer functions** each with different mathematical character — not trivial to implement cleanly and consistently across sample rates.
- **True-peak brickwall limiter** with lookahead requires careful buffer management, oversampled detection, and log-domain gain reduction smoothing.
- **Per-variant Colour stage** means 6 different biquad configurations that must be precomputed and blended.
- **Latency compensation** for the parallel Mix blend (must match limiter lookahead exactly).
- **Thread-safe metering** across audio/GUI threads requires atomic reads.
- **Smooth parameter transitions** on Temper switching without audio artefacts.

This is solidly expert-tier DSP for a production-quality plugin.

---

## JUCE Modules Required

| Module | Purpose |
| :--- | :--- |
| `juce_audio_processors` | Plugin processor base class, parameter management |
| `juce_dsp` | `juce::dsp::Gain`, `juce::dsp::IIR::Filter`, `juce::dsp::Oversampling`, `juce::dsp::Limiter` |
| `juce_audio_basics` | Buffer management, sample rate handling |
| `juce_gui_basics` | UI base layer |
