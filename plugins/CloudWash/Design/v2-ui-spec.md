# CloudWash - UI Specification v2

## Design Summary
Clean, modern flat design with teal/gray cloud-themed palette. Grid-based layout optimized for a wider 800x500px window with improved meter placement and reorganized footer layout.

## Changes from v1
- **Color:** Changed primary blue (#4A9EFF) to teal (#427E88)
- **Knobs:** Fixed starting position to 7 o'clock (210°) instead of 11 o'clock
- **Meters:** Increased height to match knob rows (~240px tall)
- **Layout:** Widened window to 800px, moved meters to left/right edges
- **Footer:** Combined settings (Quality, Sample Mode) and Freeze into centered container

## Layout
- **Window Size:** 800x500px (increased from 700px)
- **Style:** Clean modern flat with subtle depth
- **Theme:** Teal and grays evoking ocean clouds
- **Framework:** WebView (HTML5 Canvas with JUCE integration)

## Layout Structure

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  CLOUDWASH                                        [Mode Tabs]               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │               GRAIN ACTIVITY VISUALIZATION                            │ │
│  │                  [Real-time particle display]                         │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
│  ┌──┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐      ┌──┐  │
│  │IN│  │ POSITION │  │   SIZE   │  │  PITCH   │  │ DENSITY  │      │OU│  │
│  │  │  │  [knob]  │  │  [knob]  │  │  [knob]  │  │  [knob]  │      │T │  │
│  │M │  │   0.50   │  │   0.50   │  │  +0.00   │  │   0.50   │      │  │  │
│  │E │  └──────────┘  └──────────┘  └──────────┘  └──────────┘      │M │  │
│  │T │                                                                │E │  │
│  │E │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐      │T │  │
│  │R │  │ TEXTURE  │  │ IN GAIN  │  │  BLEND   │  │  SPREAD  │      │E │  │
│  │  │  │  [knob]  │  │  [knob]  │  │  [knob]  │  │  [knob]  │      │R │  │
│  │  │  │   0.50   │  │   0.80   │  │   0.50   │  │   0.00   │      │  │  │
│  │  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘      │  │  │
│  │  │                                                                │  │  │
│  │  │  ┌──────────┐  ┌──────────┐                                   │  │  │
│  │  │  │ FEEDBACK │  │  REVERB  │                                   │  │  │
│  │  │  │  [knob]  │  │  [knob]  │                                   │  │  │
│  │  │  │   0.00   │  │   0.00   │                                   │  │  │
│  └──┘  └──────────┘  └──────────┘                                   └──┘  │
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
  - Inactive tabs: muted gray
  - Tab width: 100px each
  - Tab height: 32px

### 2. Grain Activity Visualization (Mode Indicator Graphic)
- **Position:** Top section, below header
- **Size:** 720x80px
- **Type:** Canvas-based particle system
- **Behavior:**
  - Displays real-time grain generation activity
  - Particle density reflects Density parameter
  - Particle size reflects Size parameter
  - Visual changes based on active mode (color shifts, movement patterns)
  - Freezes particle animation when Freeze is active

### 3. Core Controls (Rotary Knobs - Row 1)

| Parameter | Position | Range Display | Default | Color Accent |
|-----------|----------|---------------|---------|--------------|
| POSITION  | Grid (0,0) | 0% - 100% | 50% | Teal |
| SIZE      | Grid (1,0) | 0% - 100% | 50% | Teal |
| PITCH     | Grid (2,0) | -2.0 - +2.0 | +0.00 | Cyan Accent |
| DENSITY   | Grid (3,0) | 0% - 100% | 50% | Teal |

**Knob Specifications:**
- **Diameter:** 75px
- **Arc Range:** 300° (start: 210°, end: 510°) - FIXED: starts at 7 o'clock
- **Track Color:** #2A2A3E (dark blue-gray)
- **Active Arc:** #427E88 (teal - CHANGED from baby blue)
- **Indicator Dot:** #FFFFFF (white, 8px)
- **Value Display:** Below knob, 14px, #A0A0B0
- **Label:** Above knob, 11px uppercase, #707085

### 4. Secondary Controls (Rotary Knobs - Row 2 & 3)

| Parameter | Position | Range Display | Default |
|-----------|----------|---------------|---------|
| TEXTURE   | Grid (0,1) | 0% - 100% | 50% |
| IN GAIN   | Grid (1,1) | 0% - 100% | 80% |
| BLEND     | Grid (2,1) | 0% - 100% | 50% |
| SPREAD    | Grid (3,1) | 0% - 100% | 0% |
| FEEDBACK  | Grid (0,2) | 0% - 100% | 0% |
| REVERB    | Grid (1,2) | 0% - 100% | 0% |

**Same knob specifications as core controls**

### 5. Input/Output Meters (REDESIGNED)
- **Position:** Left and right edges of control grid
- **Type:** Vertical LED-style meters
- **Size:** 35px wide x 240px tall (each) - INCREASED HEIGHT
- **Segments:** 24 bars per meter (increased from 20)
- **Colors:**
  - Green (#4CAF50): -60dB to -12dB
  - Yellow (#FFC107): -12dB to -3dB
  - Red (#FF4444): -3dB to 0dB
- **Labels:** "IN" above input meter (left), "OUT" above output meter (right)
- **Alignment:** Vertically centered with knob grid

### 6. Footer Container (REDESIGNED)
- **Position:** Bottom center, below control grid
- **Background:** #2A2A3E with subtle border
- **Padding:** 16px
- **Border-radius:** 8px
- **Layout:** Horizontal flex container with centered items

**Contents (left to right):**
1. **Quality Dropdown** (100px)
2. **Sample Mode Dropdown** (100px)
3. **Freeze Button** (150px) with LED indicator (20px)

### 7. Freeze Control
- **Type:** Toggle button with LED indicator
- **Button Size:** 150x50px
- **States:**
  - **OFF:** Gray background (#3A3A4E), white text
  - **ON:** Teal background (#427E88), white text
- **LED Indicator:**
  - **Position:** Right of button within footer container
  - **Size:** 20px circle
  - **OFF State:** #2A2A3E (dark)
  - **ON State:** #FF4444 (red, pulsing glow effect)

### 8. Quality & Sample Mode Dropdowns
- **Position:** Footer container (left side)
- **Size:** 100x32px each
- **Style:** Flat dropdown with rounded corners (4px)
- **Background:** #1A1A2E (darker for contrast)
- **Text:** #A0A0B0, 12px
- **Options:**
  - **Quality:** "HiFi" | "Balanced" | "Extended" | "Maximum"
  - **Sample Mode:** "Vintage" | "Modern"

## Color Palette (UPDATED)

### Primary Colors
- **Background:** `#1A1A2E` (deep blue-black)
- **Surface:** `#2A2A3E` (dark blue-gray)
- **Primary Teal:** `#427E88` (ocean teal - CHANGED)
- **Accent Cyan:** `#5AC8D8` (lighter cyan - adjusted to match teal)

### Text Colors
- **Primary Text:** `#FFFFFF` (white)
- **Secondary Text:** `#A0A0B0` (light gray)
- **Muted Text:** `#707085` (medium gray)

### Semantic Colors
- **Active/Hot:** `#427E88` (primary teal)
- **Warning:** `#FFC107` (amber)
- **Alert:** `#FF4444` (red)
- **Success:** `#4CAF50` (green)

### Atmospheric Gradient (for visualization)
- **Top:** `#3A4A6E` (muted blue)
- **Bottom:** `#1A1A2E` (background blend)

## Typography
- **Font Family:** 'Inter', 'Segoe UI', system-ui, sans-serif
- **Plugin Title:** 18px, 700 weight, uppercase, letter-spacing: 2px
- **Parameter Labels:** 11px, 600 weight, uppercase, letter-spacing: 1px
- **Value Display:** 14px, 400 weight, tabular-nums
- **Mode Tabs:** 12px, 600 weight, uppercase

## Spacing & Layout Grid
- **Grid Spacing:** 16px baseline grid
- **Knob Grid:**
  - 4 columns x 3 rows
  - Column gap: 24px (increased)
  - Row gap: 30px
  - Container padding: 40px left/right, 30px top/bottom
- **Meter Spacing:**
  - Left meter: 20px from left edge
  - Right meter: 20px from right edge
- **Section Margins:**
  - Header: 20px top/bottom
  - Visualization: 20px margin
  - Controls section: 30px padding
  - Footer container: 20px margin, centered

## Interaction States

### Knob Interactions
- **Hover:** Slight glow on active arc (#427E88 with 20% opacity outer ring)
- **Active/Dragging:** Brighter glow, cursor changes to vertical resize
- **Double-click:** Reset to default value with subtle animation

### Button States
- **Hover:** 10% brightness increase
- **Active:** Pressed effect (2px translate-y)
- **Focus:** 2px outline in primary teal

### Tab States
- **Hover:** Background lightens to #3A3A4E
- **Active:** Background #427E88, white text, bottom border accent
- **Transition:** 200ms ease-in-out

## Animation & Effects
- **Parameter Changes:** Smooth 50ms easing on value updates
- **Mode Switching:** 300ms fade transition on visualization
- **Freeze LED:** 1.5s pulsing animation (opacity 0.6 to 1.0)
- **Grain Visualization:** 60fps particle animation, throttled to CPU budget
- **Meter Updates:** Real-time (tied to audio processing callback), with 20ms smoothing

## Responsive Behavior
- **Minimum Size:** 700x400px (scaled layout)
- **Maximum Size:** 1000x650px (increased spacing, larger knobs)
- **Scaling Strategy:** Proportional scaling of all elements

## Canvas Integration Notes (WebView)
- **Primary Canvas:** Grain activity visualization (hardware-accelerated)
- **Secondary Canvas (optional):** Custom knob rendering for performance
- **Update Strategy:** RequestAnimationFrame for visualizations, CSS transforms for controls
- **JUCE Bridge:** Parameter changes via `window.__JUCE__.backend.setParameterValue(id, value)`

## Accessibility
- **Color Contrast:** WCAG AA compliant (4.5:1 minimum)
- **Keyboard Navigation:** Tab order: Mode tabs → Controls → Freeze → Menus
- **Focus Indicators:** 2px outline on all interactive elements
- **Screen Reader Labels:** ARIA labels on all controls

## File Structure
```
plugins/CloudWash/Design/
├── v1-ui-spec.md          (previous version)
├── v1-style-guide.md      (previous version)
├── v1-test.html           (previous version)
├── v2-ui-spec.md          (this file - UPDATED)
├── v2-style-guide.md      (updated style guide)
└── v2-test.html           (working HTML preview with v2 changes)
```

## Implementation Notes
- Use HTML5 Canvas API for grain visualization
- CSS Grid for control layout with explicit meter placement
- CSS Custom Properties for theming
- JavaScript module system for parameter management
- JUCE WebView integration via `window.__JUCE__` bridge

## Next Steps
1. Review and approve v2 specification
2. Create v2-style-guide.md with updated color tokens
3. Build v2-test.html working preview with all changes
4. Test in browser before JUCE integration
5. Iterate if needed (creates v3-*) or approve for implementation
