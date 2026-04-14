# CloudWash - UI Specification v1

## Design Summary
Clean, modern flat design with cool blue/gray cloud-themed palette. Grid-based layout optimized for a 700x500px window with clear visual hierarchy and comprehensive real-time feedback.

## Layout
- **Window Size:** 700x500px
- **Style:** Clean modern flat with subtle depth
- **Theme:** Cool blues and grays evoking atmospheric clouds
- **Framework:** WebView (HTML5 Canvas with JUCE integration)

## Layout Structure

```
┌─────────────────────────────────────────────────────────────────┐
│  CLOUDWASH                                    [Mode Tabs]       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │           GRAIN ACTIVITY VISUALIZATION                    │ │
│  │              [Real-time particle display]                 │ │
│  └───────────────────────────────────────────────────────────┘ │
│                                                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │ POSITION │  │   SIZE   │  │  PITCH   │  │ DENSITY  │       │
│  │  [knob]  │  │  [knob]  │  │  [knob]  │  │  [knob]  │       │
│  │   0.50   │  │   0.50   │  │  +0.00   │  │   0.50   │       │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘       │
│                                                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │ TEXTURE  │  │ IN GAIN  │  │  BLEND   │  │  SPREAD  │       │
│  │  [knob]  │  │  [knob]  │  │  [knob]  │  │  [knob]  │       │
│  │   0.50   │  │   0.80   │  │   0.50   │  │   0.00   │       │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘       │
│                                                                 │
│  ┌──────────┐  ┌──────────┐  ┌─────────────────┐              │
│  │ FEEDBACK │  │  REVERB  │  │  [FREEZE]       │ [LED]        │
│  │  [knob]  │  │  [knob]  │  │   Toggle        │              │
│  │   0.00   │  │   0.00   │  └─────────────────┘              │
│  └──────────┘  └──────────┘                                    │
│                                                                 │
│  ┌─────────┐  ┌─────────┐  ┌───┐  ┌───┐                       │
│  │ QUALITY │  │  SAMPLE │  │ IN│  │OUT│  [Meters]              │
│  │  [menu] │  │  [menu] │  └───┘  └───┘                        │
│  └─────────┘  └─────────┘                                      │
└─────────────────────────────────────────────────────────────────┘
```

## Component Specifications

### 1. Header Section
- **Plugin Title:** "CLOUDWASH" (left-aligned, 18px, bold, light gray)
- **Mode Tabs:** Horizontal tab bar (right-aligned)
  - Tabs: "GRANULAR" | "PITCH" | "DELAY" | "SPECTRAL"
  - Active tab: highlighted with accent blue
  - Inactive tabs: muted gray
  - Tab width: 100px each
  - Tab height: 32px

### 2. Grain Activity Visualization (Mode Indicator Graphic)
- **Position:** Top section, below header
- **Size:** 650x80px
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
| POSITION  | Grid (0,0) | 0% - 100% | 50% | Primary Blue |
| SIZE      | Grid (1,0) | 0% - 100% | 50% | Primary Blue |
| PITCH     | Grid (2,0) | -2.0 - +2.0 | +0.00 | Cyan Accent |
| DENSITY   | Grid (3,0) | 0% - 100% | 50% | Primary Blue |

**Knob Specifications:**
- **Diameter:** 75px
- **Arc Range:** 270° (start: 225°, end: 495°)
- **Track Color:** #2A2A3E (dark blue-gray)
- **Active Arc:** #4A9EFF (primary blue)
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

### 5. Freeze Control
- **Type:** Toggle button with LED indicator
- **Position:** Grid (2,2) - spanning width
- **Button Size:** 150x50px
- **States:**
  - **OFF:** Gray background (#3A3A4E), white text
  - **ON:** Blue background (#4A9EFF), white text
- **LED Indicator:**
  - **Position:** Right of button
  - **Size:** 20px circle
  - **OFF State:** #2A2A3E (dark)
  - **ON State:** #FF4444 (red, pulsing glow effect)

### 6. Quality & Sample Mode Dropdowns
- **Position:** Bottom left section
- **Size:** 100x32px each
- **Style:** Flat dropdown with rounded corners (4px)
- **Background:** #2A2A3E
- **Text:** #A0A0B0, 12px
- **Options:**
  - **Quality:** "HiFi" | "Balanced" | "Extended" | "Maximum"
  - **Sample Mode:** "Vintage" | "Modern"

### 7. Input/Output Meters
- **Position:** Bottom right section
- **Type:** Vertical LED-style meters
- **Size:** 30px wide x 100px tall (each)
- **Segments:** 20 bars per meter
- **Colors:**
  - Green (#4CAF50): -60dB to -12dB
  - Yellow (#FFC107): -12dB to -3dB
  - Red (#FF4444): -3dB to 0dB
- **Labels:** "IN" and "OUT" below each meter

## Color Palette

### Primary Colors
- **Background:** `#1A1A2E` (deep blue-black)
- **Surface:** `#2A2A3E` (dark blue-gray)
- **Primary Blue:** `#4A9EFF` (bright sky blue)
- **Accent Cyan:** `#00D4FF` (cyan highlight)

### Text Colors
- **Primary Text:** `#FFFFFF` (white)
- **Secondary Text:** `#A0A0B0` (light gray)
- **Muted Text:** `#707085` (medium gray)

### Semantic Colors
- **Active/Hot:** `#4A9EFF` (primary blue)
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
  - Column gap: 20px
  - Row gap: 30px
  - Container padding: 30px
- **Section Margins:**
  - Header: 20px top/bottom
  - Visualization: 20px margin
  - Controls section: 30px padding
  - Footer: 20px bottom

## Interaction States

### Knob Interactions
- **Hover:** Slight glow on active arc (#4A9EFF with 20% opacity outer ring)
- **Active/Dragging:** Brighter glow, cursor changes to vertical resize
- **Double-click:** Reset to default value with subtle animation

### Button States
- **Hover:** 10% brightness increase
- **Active:** Pressed effect (2px translate-y)
- **Focus:** 2px outline in primary blue

### Tab States
- **Hover:** Background lightens to #3A3A4E
- **Active:** Background #4A9EFF, white text, bottom border accent
- **Transition:** 200ms ease-in-out

## Animation & Effects
- **Parameter Changes:** Smooth 50ms easing on value updates
- **Mode Switching:** 300ms fade transition on visualization
- **Freeze LED:** 1.5s pulsing animation (opacity 0.6 to 1.0)
- **Grain Visualization:** 60fps particle animation, throttled to CPU budget
- **Meter Updates:** Real-time (tied to audio processing callback), with 20ms smoothing

## Responsive Behavior
- **Minimum Size:** 600x400px (scaled layout)
- **Maximum Size:** 900x650px (increased spacing, larger knobs)
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
├── v1-ui-spec.md          (this file)
├── v1-style-guide.md      (detailed style reference)
└── v1-test.html           (working HTML preview)
```

## Implementation Notes
- Use HTML5 Canvas API for grain visualization
- CSS Grid for control layout
- CSS Custom Properties for theming
- JavaScript module system for parameter management
- JUCE WebView integration via `window.__JUCE__` bridge

## Next Steps
1. Review and approve this specification
2. Create v1-style-guide.md with detailed CSS specifications
3. Build v1-test.html working preview
4. Test in browser before JUCE integration
5. Iterate if needed (creates v2-*)
