# Style Guide v1 — ColourBrickwallScream

## Design Language
**Theme:** Industrial heat. Cracked concrete meets burning metal.
Think: abandoned factory floor, warning panels still live, heat distortion in the air.
High contrast, worn surfaces, aggressive glow.

---

## Color Palette

### Core
| Role | Name | Hex | Usage |
| :--- | :--- | :--- | :--- |
| Background | Furnace Black | `#0D0A09` | Body background |
| Panel | Soot | `#131010` | Header + bottom row |
| Raised | Worn Steel | `#1C1815` | Knob bezels, raised surfaces |
| Separator | Carbon | `#241E1C` | Section dividers |
| Border | Iron | `#2A2220` | Button borders, panel edges |

### Accent (The Heat)
| Role | Name | Hex | Usage |
| :--- | :--- | :--- | :--- |
| Primary Accent | Scream Orange | `#FF3C00` | Active arcs, active buttons, indicator dots |
| Secondary Accent | Ember | `#FF6830` | Indicator dot tips, value text |
| Glow | Heat Haze | `rgba(255, 60, 0, 0.35)` | Box-shadow glow on active elements |
| Deep Glow | Buried Fire | `rgba(255, 60, 0, 0.12)` | Active button background tint |

### Text
| Role | Hex | Usage |
| :--- | :--- | :--- |
| Primary text | `#DDD0C8` | Plugin name, values |
| Label text | `#887068` | Parameter labels, "IN / GR / OUT" |
| Dim text | `#5A4D48` | Inactive button labels, meter dB values |
| Sub-label | `#FF3C00` | Plugin sub-title, accent text |

### Meter Colors (LED segments)
| Level | Inactive | Active |
| :--- | :--- | :--- |
| 0–60% | `#1A5C1A` | `#22CC22` |
| 60–75% | `#2A5A10` | `#88CC00` |
| 75–85% | `#7A5500` | `#FFAA00` |
| 85–95% | `#8A2A00` | `#FF4400` |
| 95–100% | `#6A0000` | `#FF1100` |

---

## Typography

| Role | Font | Size | Weight | Letter Spacing | Case |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Plugin name | `'Courier New', monospace` | 17px | Bold | 3px | Mixed |
| Sub-label | `'Courier New', monospace` | 8px | Normal | 2px | Upper |
| Parameter labels | `'Courier New', monospace` | 9px | Normal | 2px | Upper |
| Value displays | `'Courier New', monospace` | 9px | Normal | 1px | Mixed |
| Meter labels | `'Courier New', monospace` | 8px | Normal | 1.5px | Upper |
| Button labels | `'Courier New', monospace` | 8px | Normal | 1px | Upper |

**Note:** Courier New reinforces the industrial/technical aesthetic. All caps + letter-spacing for labels. Mixed case for values (showing numbers, units).

---

## Knob Visual Design

### Geometry (all knobs share this visual language)
```
         ┌──────────────┐
         │   outer rim  │  ← dark metallic gradient (#3C3330 → #141210)
         │  ┌──────────┐│
         │  │ bezel rim ││  ← 1.5px stroke at #2E2825
         │  │ ┌────────┐││
         │  │ │  body  │││  ← radial gradient (#2A2420 → #111010)
         │  │ │ ╱━━━╮  │││  ← indicator line + dot (#FF3C00 / #FF6830)
         │  │ │     ●  │││  ← center dot (#2E2825)
         │  │ └────────┘││
         │  └──────────┘│
         └──────────────┘
```

### Track Arc
- **Background:** Full 300° arc at `#2A2220`, lineWidth=3 (small) / 4 (large)
- **Active (unipolar):** Orange arc from 7 o'clock → current, gradient `#CC3000` → `#FF6830`
- **Active (bipolar):** Orange arc from 12 o'clock → current direction (both sides)
- **Line cap:** round

### Sweep
- Min position (value=0): 7 o'clock (120° from east, clockwise)
- Max position (value=1): 5 o'clock (60° from east, clockwise, via top)
- Sweep: 300° clockwise

### Indicator
- Line: from 20% to 68% of inner body radius, `#FF3C00`, 2px (small) / 2.5px (large)
- Dot tip: 2.5px radius (small) / 3.5px radius (large), `#FF6830`
- Center dot: 3px radius (small) / 4px radius (large), `#2E2825`

### High-Drive Glow (Drive knob only)
- At value > 0.8: outer glow ring appears, `rgba(255,60,0, (value-0.8)*1.5)`, 3px stroke

---

## Character Selector Buttons

```css
/* Inactive state */
.char-btn {
  background: #1C1815;
  border: 1px solid #2A2220;
  color: #5A4D48;
  font-size: 8px;
  letter-spacing: 1px;
  padding: 5px 7px;
  border-radius: 2px;
}

/* Active state */
.char-btn.active {
  background: rgba(255, 60, 0, 0.12);
  border-color: #FF3C00;
  color: #FF3C00;
  box-shadow: 0 0 8px rgba(255, 60, 0, 0.35),
              inset 0 0 6px rgba(255, 60, 0, 0.08);
}

/* Hover state */
.char-btn:hover {
  border-color: #FF3C00;
  color: #DDD0C8;
}
```

---

## Separator Lines

```css
.sep-line {
  height: 1px;
  background: #241E1C;
  /* Optional: subtle orange glow */
  box-shadow: 0 0 4px rgba(255, 60, 0, 0.06);
}
```

---

## Texture Overlay

Subtle SVG fractal noise at 6% opacity, `position: fixed`, `pointer-events: none`, `z-index: 999`.
Creates concrete/grain texture on all surfaces without impacting readability.

```css
body::after {
  content: '';
  position: fixed;
  inset: 0;
  pointer-events: none;
  z-index: 999;
  opacity: 0.06;
  background-image: url("data:image/svg+xml, [SVG noise filter]");
  background-size: 300px 300px;
}
```

---

## Spacing & Padding

| Context | Value |
| :--- | :--- |
| Header horizontal padding | 16px |
| Bottom row horizontal padding | 24px |
| Gap between meter rows | 5px |
| Gap between knob and label | 4px |
| Gap between label and value | 3px |
| Character button gap | 4px |
| Main area top padding | 10px |

---

## Visual References / Mood
- A nuclear plant control panel — functional, dangerous, alive
- Metal rack gear with worn paint and glowing VUs
- Industrial warning signage: orange on black
- Heat distortion through glass over molten metal
- Never clean, never sterile — this thing has been used hard

---

## Anti-patterns (What NOT to do)
- No rounded corners > 2px (except buttons at 2px)
- No gradients that go light → dark top-to-bottom (wrong direction for heat)
- No blue, purple, or cool-tone accents anywhere
- No white backgrounds, ever
- No smooth/gentle animations — meters should feel reactive, not floaty
- No sans-serif fonts — Courier New only (monospace = industrial precision)
