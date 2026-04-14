# Parameter Spec: ColourBrickwallScream

## Parameters

| ID | Name | Type | Range | Default | Unit | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `drive` | Drive / Scream | Float | 0.0 – 1.0 | 0.25 | dB (mapped 0–40dB) | Pre-saturation input gain. Controls how hard the signal is pushed into the saturation stage. |
| `character` | Character | Int (Enum) | 0 – 5 | 1 | — | Temper variant selector. See Temper Variants below. |
| `tone` | Tone | Float | -1.0 – 1.0 | 0.0 | — | Post-saturation tilt EQ. Negative = darker, positive = brighter. Pivot around 1kHz. |
| `colour` | Colour | Float | 0.0 – 1.0 | 0.0 | — | Harmonic colouration amount. Blends in Character-dependent mid/air resonance. |
| `attack_character` | Attack Character | Float | 0.0 – 1.0 | 0.5 | ms (mapped 0.1–50ms) | Brickwall limiter attack time. 0 = fastest/punchiest, 1 = slowest/most crushed. |
| `ceiling` | Ceiling | Float | -12.0 – 0.0 | -0.3 | dBFS | Brickwall limiter output ceiling. True peak ceiling. |
| `mix` | Mix | Float | 0.0 – 1.0 | 1.0 | % | Dry/wet blend for parallel saturation. 1.0 = fully wet. |
| `output_gain` | Output Gain | Float | -12.0 – 12.0 | 0.0 | dB | Post-limiter makeup gain. |

## Temper Variants (Character)

| Index | Name | Description | Harmonic Profile |
| :--- | :--- | :--- | :--- |
| 0 | **Tube** | Soft, even-order harmonic warmth | 2nd + 3rd harmonics, soft knee |
| 1 | **Tape** | Compressed warmth with gentle HF roll-off saturation | 2nd harmonic dominant, subtle compression feel |
| 2 | **Transformer** | Iron-core character — midrange bloom and slight LF softness | 3rd harmonic bloom, low-end weight |
| 3 | **Diode** | Asymmetric hard clipping with presence | Odd harmonics, hard knee, presence boost |
| 4 | **Bitcrush** | Digital aliasing and quantisation grit | High-order harmonics, digital edge |
| 5 | **Full Scream** | Maximum chaos — all harmonics, no mercy | Full harmonic stack, asymmetric wavefolding |

## Signal Chain Order
```
Input
  └─► Drive (pre-gain)
        └─► Saturation (Character/Temper)
              └─► Colour (harmonic blend)
                    └─► Tone (tilt EQ)
                          └─► Brickwall Limiter (Attack Character + Ceiling)
                                └─► Output Gain
                                      └─► Mix (parallel blend back to dry)
                                            └─► Output
```

## Metering
- Input level meter (pre-drive)
- Gain reduction meter (limiter GR)
- Output level meter (post-output-gain)
- Character indicator (active temper variant illuminated)
