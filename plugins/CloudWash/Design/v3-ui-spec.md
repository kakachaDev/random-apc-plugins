# CloudWash - UI Specification v3

## Design Summary
Clean, modern flat design with teal/gray cloud-themed palette. 2-column knob layout with knobs rotated -90° (gap at bottom) for optimal 800x500px window.

## Changes from v2
- **Knob rotation:** Rotated -90° so gap is at bottom (arc from 120° to 420°)
- **Layout:** Changed from 4×3 grid to 2×5 grid (2 columns, 5 rows of knobs)
- **Knob order:** Position/Texture, Size/In Gain, Pitch/Blend, Density/Spread, Feedback/Reverb
- **Spacing:** Reduced vertical gap between knob rows from 30px to 15px
- **Visualization:** Reduced height from 80px to 60px to make room for taller control section
- **Meters:** Increased to 280px height

## Layout
- **Window Size:** 800x500px
- **Style:** Clean modern flat with subtle depth
- **Theme:** Teal and grays evoking ocean clouds
- **Framework:** WebView (HTML5 Canvas with JUCE integration)

## Layout Structure

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  CLOUDWASH                                        [Mode Tabs]               │
├─────────────────────────────────────────────────────────────────────────────┤
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │           GRAIN ACTIVITY VISUALIZATION (60px height)                  │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
│  ┌─┐   ┌──────────┐      ┌──────────┐                              ┌─┐    │
│  │I│   │ POSITION │      │ TEXTURE  │                              │O│    │
│  │N│   │  [knob]  │      │  [knob]  │                              │U│    │
│  │ │   │   0.50   │      │   0.50   │                              │T│    │
│  │M│   └──────────┘      └──────────┘                              │ │    │
│  │E│                                                                │M│    │
│  │T│   ┌──────────┐      ┌──────────┐                              │E│    │
│  │E│   │   SIZE   │      │ IN GAIN  │                              │T│    │
│  │R│   │  [knob]  │      │  [knob]  │                              │E│    │
│  │ │   │   0.50   │      │   0.80   │                              │R│    │
│  │ │   └──────────┘      └──────────┘                              │ │    │
│  │2│                                                                │2│    │
│  │8│   ┌──────────┐      ┌──────────┐                              │8│    │
│  │0│   │  PITCH   │      │  BLEND   │                              │0│    │
│  │p│   │  [knob]  │      │  [knob]  │                              │p│    │
│  │x│   │  +0.00   │      │   0.50   │                              │x│    │
│  │ │   └──────────┘      └──────────┘                              │ │    │
│  │ │                                                                │ │    │
│  │ │   ┌──────────┐      ┌──────────┐                              │ │    │
│  │ │   │ DENSITY  │      │  SPREAD  │                              │ │    │
│  │ │   │  [knob]  │      │  [knob]  │                              │ │    │
│  │ │   │   0.50   │      │   0.00   │                              │ │    │
│  │ │   └──────────┘      └──────────┘                              │ │    │
│  │ │                                                                │ │    │
│  │ │   ┌──────────┐      ┌──────────┐                              │ │    │
│  │ │   │ FEEDBACK │      │  REVERB  │                              │ │    │
│  │ │   │  [knob]  │      │  [knob]  │                              │ │    │
│  └─┘   │   0.00   │      │   0.00   │                              └─┘    │
│        └──────────┘      └──────────┘                                      │
│                                                                             │
│       ┌───────────────────────────────────────────────────────┐            │
│       │  [Quality ▼]  [Sample Mode ▼]  [FREEZE Button] [LED] │            │
│       └───────────────────────────────────────────────────────┘            │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Component Specifications

### 1. Header Section
- **Plugin Title:** "CLOUDWASH" (left-aligned, 18px, bold, light gray)
- **Mode Tabs:** Horizontal tab bar (right-aligned)
  - Tabs: "GRANULAR" | "PITCH" | "DELAY" | "SPECTRAL"
  - Active tab: highlighted with teal accent
  - Tab width: 100px each
  - Tab height: 32px

### 2. Grain Activity Visualization
- **Size:** 720x60px (reduced from 80px)
- **Type:** Canvas-based particle system
- **Behavior:** Real-time grain visualization that responds to parameters

### 3. Knob Layout Grid (2 Columns × 5 Rows)

**Column 1 (Left):**
- Row 1: POSITION
- Row 2: SIZE
- Row 3: PITCH
- Row 4: DENSITY
- Row 5: FEEDBACK

**Column 2 (Right):**
- Row 1: TEXTURE
- Row 2: IN GAIN
- Row 3: BLEND
- Row 4: SPREAD
- Row 5: REVERB

**Grid Specifications:**
- Columns: 2
- Column gap: 40px
- Row gap: 15px (reduced from 30px)
- Container padding: 40px horizontal

### 4. Rotary Knob Specifications

**Knob Arc Geometry (ROTATED -90°):**
- **Start angle:** 120° (left side, 10 o'clock)
- **End angle:** 420° (wraps around bottom to right side)
- **Total range:** 300°
- **Gap position:** Bottom (270°)
- **Diameter:** 75px
- **Track Color:** #2A2A3E (dark blue-gray)
- **Active Arc:** #427E88 (teal)
- **Pitch Arc:** #5AC8D8 (cyan accent for pitch parameter only)
- **Indicator Dot:** #FFFFFF (white, 8px)

**Knob Parameters:**

| Parameter | Default | Range | Display Format |
|-----------|---------|-------|----------------|
| POSITION  | 0.5     | 0-1   | 0-100% |
| TEXTURE   | 0.5     | 0-1   | 0-100% |
| SIZE      | 0.5     | 0-1   | 0-100% |
| IN GAIN   | 0.8     | 0-1   | 0-100% |
| PITCH     | 0.0     | -2 to +2 | ±2.00 octaves |
| BLEND     | 0.5     | 0-1   | 0-100% |
| DENSITY   | 0.5     | 0-1   | 0-100% |
| SPREAD    | 0.0     | 0-1   | 0-100% |
| FEEDBACK  | 0.0     | 0-1   | 0-100% |
| REVERB    | 0.0     | 0-1   | 0-100% |

### 5. Input/Output Meters
- **Position:** Left and right edges flanking control grid
- **Size:** 35px wide × 280px tall (increased from 240px)
- **Segments:** 24 bars per meter
- **Colors:**
  - Green (#4CAF50): segments 0-18 (safe zone)
  - Yellow (#FFC107): segments 19-22 (caution)
  - Red (#FF4444): segment 23 (peak)
- **Labels:** "IN" and "OUT" above meters

### 6. Footer Container
- **Background:** #2A2A3E with border
- **Padding:** 12px vertical, 16px horizontal (reduced)
- **Border-radius:** 8px
- **Layout:** Horizontal flex, centered

**Contents (left to right):**
1. **Quality Dropdown** (100x32px)
   - Options: "High Fidelity" | "Balanced" | "Extended" | "Maximum"
2. **Sample Mode Dropdown** (100x32px)
   - Options: "Vintage" | "Modern"
3. **Freeze Button** (150x40px) - reduced height
4. **Freeze LED** (18x18px) - slightly smaller

### 7. Freeze Control
- **Button Size:** 150x40px (reduced from 50px height)
- **States:**
  - OFF: Gray background (#3A3A4E)
  - ON: Teal background (#427E88)
- **LED Indicator:**
  - OFF: #2A2A3E (dark)
  - ON: #FF4444 (red, pulsing)

## Color Palette

### Primary Colors
- **Background:** `#1A1A2E`
- **Surface:** `#2A2A3E`
- **Primary Teal:** `#427E88`
- **Accent Cyan:** `#5AC8D8`

### Text Colors
- **Primary:** `#FFFFFF`
- **Secondary:** `#A0A0B0`
- **Muted:** `#707085`

### Semantic Colors
- **Active:** `#427E88` (teal)
- **Success:** `#4CAF50` (green)
- **Warning:** `#FFC107` (amber)
- **Danger:** `#FF4444` (red)

## Typography
- **Font Family:** 'Inter', 'Segoe UI', system-ui, sans-serif
- **Plugin Title:** 18px, 700 weight
- **Parameter Labels:** 11px, 600 weight, uppercase
- **Value Display:** 13px, 400 weight (reduced from 14px)
- **Mode Tabs:** 12px, 600 weight

## Spacing & Layout
- **Window:** 800x500px
- **Header padding:** 20px top/bottom, 40px left/right
- **Visualization margin:** 0 40px 15px 40px
- **Controls wrapper padding:** 0 40px
- **Knob grid gap:** 15px rows, 40px columns
- **Footer padding:** 0 40px 15px 40px

## Interaction States
- **Knob hover:** Teal glow effect
- **Knob active:** Brighter glow, vertical resize cursor
- **Knob double-click:** Reset to default
- **Button hover:** Brightness increase
- **Tab active:** Teal background with glow

## Animation & Effects
- **Parameter updates:** 50ms smooth easing
- **Mode switching:** 200ms transition
- **Freeze LED pulse:** 1.5s infinite animation
- **Meter updates:** Real-time with 20ms smoothing
- **Grain visualization:** 60fps particle animation

## Accessibility
- **Color contrast:** WCAG AA compliant
- **Keyboard navigation:** Full support
- **Focus indicators:** 2px teal outline
- **ARIA labels:** All controls properly labeled

## Implementation Notes
- Use CSS Grid for 2-column layout
- SVG knobs with precise arc calculations (120° to 420°)
- HTML5 Canvas for grain visualization
- JUCE WebView integration via `window.__JUCE__` bridge
- Responsive scaling for different window sizes

## File Structure
```
plugins/CloudWash/Design/
├── v1-ui-spec.md          (original 4×3 grid, 7 o'clock start)
├── v2-ui-spec.md          (4×3 grid, 7 o'clock start, taller meters)
├── v3-ui-spec.md          (this file - 2×5 grid, gap at bottom)
├── v3-test.html           (working preview with v3 changes)
└── [previous test files]
```

## Next Steps
1. Review and approve v3 specification
2. Test v3-test.html in browser
3. Iterate if needed (creates v4-*) or approve for implementation
4. Proceed to /impl phase for C++ implementation
