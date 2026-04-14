# UI Specification v1 — ColourBrickwallScream

## Layout
- **Window:** 700 × 500 px (fixed, no resize)
- **Framework:** WebView (HTML5 Canvas + CSS)
- **Structure:** 4-zone vertical stack

```
┌──────────────────────────────────────────────────────────────────────┐
│ HEADER (700 × 62px)                                                   │
│  [Plugin Name Block]    │  IN  [═══════════════░░░░]  -6.2 dB        │
│  ColourBrickwall        │  GR  [░░░░░░░═══════════]  -4.1 dB        │
│  Scream                 │  OUT [════════════════░░░]  -1.8 dB        │
├──────────────────────────────────────────────────────────────────────┤  ← 1px sep
│ MAIN AREA (700 × 305px)                                               │
│                                                                       │
│  LEFT (155px)    CENTER (390px)              RIGHT (155px)            │
│                                                                       │
│                       ╭────────────────╮                             │
│   [TONE]             ╱   DRIVE/SCREAM   ╲            [COLOUR]        │
│  knob 100×100       │    knob 180×180    │           knob 100×100    │
│                      ╲                  ╱                            │
│                       ╰────────────────╯                             │
│                  DRIVE / SCREAM  ·  0.0 dB                           │
│           [TUBE][TAPE][XFMR][DIODE][BITCR][SCREAM]                   │
│                                                                       │
├──────────────────────────────────────────────────────────────────────┤  ← 1px sep
│ BOTTOM ROW (700 × 131px)                                              │
│                                                                       │
│    [ATTACK]       [CEILING]        [MIX]         [OUTPUT]            │
│   knob 80×80      knob 80×80     knob 80×80      knob 80×80          │
│                                                                       │
└──────────────────────────────────────────────────────────────────────┘
```

## Sections

### Header (62px)
- **Left block:** Plugin name "ColourBrickwall / Scream" (two lines), sub-label "HARMONIC SATURATOR · BRICKWALL LIMITER"
- **Right block (flex:1):** Three horizontal segmented LED meters stacked vertically:
  - `IN` — input peak+RMS, standard green→yellow→orange→red gradient
  - `GR` — gain reduction, fills right-to-left, orange/red
  - `OUT` — output peak+RMS, same as IN
  - Each row: label (20px) + canvas (flex:1) + dB value (46px)

### Main Area (305px, flex: 1)
Three columns via CSS flexbox:

**Left — TONE (155px wide)**
- Rotary knob, 100×100px canvas
- **Bipolar display:** arc extends from 12 o'clock outward in direction of value
- Center default position (0.5 norm) shows no arc — "flat"
- Label: "TONE" below knob
- Value display: "+0.00" / "-0.00" format

**Center — DRIVE (390px wide)**
- Large rotary knob, 180×180px canvas
- Standard unipolar arc (7 o'clock → 5 o'clock)
- Label: "DRIVE / SCREAM" below knob
- Value display: "0.0 dB" → "40.0 dB"
- CHARACTER SELECTOR — row of 6 buttons below label:
  `TUBE` | `TAPE` | `XFMR` | `DIODE` | `BITCR` | `SCREAM`
  - Active = orange border + orange text + glow shadow
  - Inactive = dark border + dim text

**Right — COLOUR (155px wide)**
- Rotary knob, 100×100px canvas
- Unipolar (0% → 100%)
- Label: "COLOUR" below knob
- Value display: "0%" → "100%"

### Bottom Row (131px)
Four equal-width columns (175px each), small knobs:

| Position | Parameter | Display | Type |
| :--- | :--- | :--- | :--- |
| 1 | ATTACK | "0.1ms" – "50.0ms" (log scale) | Unipolar |
| 2 | CEILING | "-12.0 dBFS" – "0.0 dBFS" | Unipolar |
| 3 | MIX | "0%" – "100%" | Unipolar |
| 4 | OUTPUT | "-12.0 dB" – "+12.0 dB" | Bipolar |

---

## Controls

| Parameter | ID | Knob Size | Position | Normalized Default | Display Format |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Drive/Scream | `drive` | 180×180 | Center | 0.25 | `X.X dB` (0–40) |
| Character | `character` | Buttons | Center-bottom | 0 (TUBE) | Illuminated button |
| Tone | `tone` | 100×100 | Left | 0.50 (bipolar) | `+X.XX` / `-X.XX` |
| Colour | `colour` | 100×100 | Right | 0.00 | `X%` |
| Attack Char. | `attack_character` | 80×80 | Bottom-1 | 0.50 | `X.Xms` (log) |
| Ceiling | `ceiling` | 80×80 | Bottom-2 | 0.975 | `X.X dBFS` |
| Mix | `mix` | 80×80 | Bottom-3 | 1.00 | `X%` |
| Output Gain | `output_gain` | 80×80 | Bottom-4 | 0.50 (bipolar) | `±X.X dB` |

---

## Knob Interaction
- **Mouse drag:** Vertical drag up/down changes value (8x / sample, 2x with Shift)
- **Double-click:** Resets to default value
- **Scroll wheel:** Fine adjustment (0.5% per tick, 0.05% with Shift)
- **Drag sensitivity:** Standard = 0.008/px, Fine (Shift) = 0.002/px

---

## Meter Behavior
- **Ballistic:** Fast attack (~70% per frame), slow decay (~10% per frame)
- **Segmented:** LED-style segments (4px wide, 1px gap)
- **GR meter:** Fills right-to-left (0 GR = empty, high GR = full from right)
- **Update rate:** requestAnimationFrame (~60fps preview, JUCE polled at 30fps)

---

## Special Visual States
- **Character button glow:** Active button has `box-shadow: 0 0 8px rgba(255,60,0,0.4)`
- **Drive knob high-value glow:** At value > 0.80, outer glow ring appears proportional to excess
- **Meter peak hold:** Orange segment stays 1s on clip before decay (implementation phase)
- **Noise texture overlay:** Fixed `position: fixed` SVG noise at 6% opacity over entire UI
