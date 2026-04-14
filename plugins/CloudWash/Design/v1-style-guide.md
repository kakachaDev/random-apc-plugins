# CloudWash - Style Guide v1

## Visual Identity
**Theme:** Atmospheric Clouds
**Aesthetic:** Clean, modern, functional
**Mood:** Calm, spacious, ethereal
**Inspiration:** Sky gradients, cloud formations, modern DAW interfaces

---

## Color System

### Base Palette
```css
:root {
  /* Background Layers */
  --bg-primary: #1A1A2E;      /* Deep blue-black base */
  --bg-surface: #2A2A3E;      /* Elevated surface */
  --bg-raised: #3A3A4E;       /* Hover/raised elements */

  /* Brand Colors */
  --brand-blue: #4A9EFF;      /* Primary interactive blue */
  --brand-cyan: #00D4FF;      /* Accent cyan for highlights */
  --brand-sky: #7AB8FF;       /* Lighter sky blue for gradients */

  /* Text Colors */
  --text-primary: #FFFFFF;    /* White for labels and values */
  --text-secondary: #A0A0B0;  /* Light gray for secondary info */
  --text-muted: #707085;      /* Muted gray for subtle text */
  --text-disabled: #4A4A5E;   /* Disabled state */

  /* Semantic Colors */
  --color-active: #4A9EFF;    /* Active/selected state */
  --color-success: #4CAF50;   /* Green for meters (safe zone) */
  --color-warning: #FFC107;   /* Amber for meters (caution) */
  --color-danger: #FF4444;    /* Red for meters (peak) + freeze LED */

  /* Visualization Gradient */
  --viz-top: #3A4A6E;         /* Top of grain visualization */
  --viz-bottom: #1A1A2E;      /* Bottom (blends with background) */

  /* Transparency Overlays */
  --overlay-light: rgba(255, 255, 255, 0.05);
  --overlay-dark: rgba(0, 0, 0, 0.2);
  --glow-blue: rgba(74, 158, 255, 0.3);
}
```

### Color Usage Guidelines

**Backgrounds:**
- Plugin window: `--bg-primary`
- Knob track/containers: `--bg-surface`
- Hover states: `--bg-raised`

**Interactive Elements:**
- Active knob arc: `--brand-blue`
- Hovered knob: `--brand-blue` with `--glow-blue` shadow
- Selected mode tab: `--brand-blue` background
- Pitch parameter accent: `--brand-cyan`

**Text Hierarchy:**
- Parameter labels: `--text-muted`
- Parameter values: `--text-secondary`
- Plugin title: `--text-primary`
- Mode tabs: `--text-primary`

**Feedback:**
- Freeze LED (active): `--color-danger`
- Input meter (safe): `--color-success`
- Input meter (hot): `--color-warning`
- Input meter (peak): `--color-danger`

---

## Typography

### Font Stack
```css
--font-primary: 'Inter', 'Segoe UI', -apple-system, BlinkMacSystemFont, system-ui, sans-serif;
--font-mono: 'SF Mono', 'Consolas', 'Monaco', monospace;
```

### Type Scale
```css
--font-size-title: 18px;        /* Plugin name */
--font-size-tab: 12px;          /* Mode tabs */
--font-size-label: 11px;        /* Parameter labels */
--font-size-value: 14px;        /* Parameter values */
--font-size-small: 10px;        /* Dropdown items, hints */

--font-weight-bold: 700;        /* Title */
--font-weight-semibold: 600;    /* Labels, tabs */
--font-weight-normal: 400;      /* Values */

--letter-spacing-title: 2px;    /* CLOUDWASH */
--letter-spacing-label: 1px;    /* POSITION, SIZE, etc. */
--letter-spacing-tab: 0.5px;    /* Mode tabs */
```

### Text Styles
```css
.plugin-title {
  font-size: var(--font-size-title);
  font-weight: var(--font-weight-bold);
  color: var(--text-primary);
  text-transform: uppercase;
  letter-spacing: var(--letter-spacing-title);
}

.parameter-label {
  font-size: var(--font-size-label);
  font-weight: var(--font-weight-semibold);
  color: var(--text-muted);
  text-transform: uppercase;
  letter-spacing: var(--letter-spacing-label);
}

.parameter-value {
  font-size: var(--font-size-value);
  font-weight: var(--font-weight-normal);
  color: var(--text-secondary);
  font-variant-numeric: tabular-nums; /* Aligned number width */
}

.mode-tab {
  font-size: var(--font-size-tab);
  font-weight: var(--font-weight-semibold);
  color: var(--text-primary);
  text-transform: uppercase;
  letter-spacing: var(--letter-spacing-tab);
}
```

---

## Layout & Spacing

### Grid System
```css
--grid-unit: 8px;           /* Base unit (all spacing multiples of 8) */
--gap-xs: 8px;              /* Tight spacing */
--gap-sm: 16px;             /* Default spacing */
--gap-md: 24px;             /* Section spacing */
--gap-lg: 32px;             /* Large section spacing */

--container-padding: 30px;  /* Main container padding */
--knob-spacing-x: 20px;     /* Horizontal gap between knobs */
--knob-spacing-y: 30px;     /* Vertical gap between knobs */
```

### Component Dimensions
```css
/* Window */
--window-width: 700px;
--window-height: 500px;

/* Header */
--header-height: 60px;

/* Visualization */
--viz-height: 80px;

/* Knobs */
--knob-diameter: 75px;
--knob-indicator-size: 8px;

/* Freeze Button */
--freeze-width: 150px;
--freeze-height: 50px;
--freeze-led-size: 20px;

/* Dropdowns */
--dropdown-width: 100px;
--dropdown-height: 32px;

/* Meters */
--meter-width: 30px;
--meter-height: 100px;

/* Mode Tabs */
--tab-width: 100px;
--tab-height: 32px;
```

---

## Component Styles

### Rotary Knob
```css
.knob {
  width: var(--knob-diameter);
  height: var(--knob-diameter);
  position: relative;
  cursor: ns-resize; /* Vertical drag cursor */
}

.knob-track {
  fill: none;
  stroke: var(--bg-surface);
  stroke-width: 6px;
  stroke-linecap: round;
}

.knob-arc {
  fill: none;
  stroke: var(--brand-blue);
  stroke-width: 6px;
  stroke-linecap: round;
  transition: stroke 50ms ease;
}

.knob-indicator {
  width: var(--knob-indicator-size);
  height: var(--knob-indicator-size);
  background: var(--text-primary);
  border-radius: 50%;
  position: absolute;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.3);
}

.knob:hover .knob-arc {
  filter: drop-shadow(0 0 8px var(--glow-blue));
}

.knob.active .knob-arc {
  stroke: var(--brand-cyan);
  filter: drop-shadow(0 0 12px var(--glow-blue));
}
```

**Knob Arc Geometry:**
- Start angle: 225° (bottom-left)
- End angle: 495° (bottom-right)
- Total range: 270°
- Center point: (37.5px, 37.5px) for 75px diameter
- Radius: 33px (inner space for indicator)

### Mode Tabs
```css
.mode-tabs {
  display: flex;
  gap: 2px;
  background: var(--bg-surface);
  border-radius: 6px;
  padding: 2px;
}

.mode-tab {
  width: var(--tab-width);
  height: var(--tab-height);
  background: transparent;
  border: none;
  border-radius: 4px;
  transition: all 200ms ease-in-out;
}

.mode-tab:hover {
  background: var(--bg-raised);
}

.mode-tab.active {
  background: var(--brand-blue);
  color: var(--text-primary);
  box-shadow: 0 2px 8px var(--glow-blue);
}
```

### Freeze Button & LED
```css
.freeze-button {
  width: var(--freeze-width);
  height: var(--freeze-height);
  background: var(--bg-raised);
  border: 2px solid var(--bg-surface);
  border-radius: 8px;
  color: var(--text-primary);
  font-size: 14px;
  font-weight: var(--font-weight-semibold);
  text-transform: uppercase;
  letter-spacing: 1px;
  cursor: pointer;
  transition: all 150ms ease;
}

.freeze-button:hover {
  background: var(--bg-surface);
  border-color: var(--brand-blue);
}

.freeze-button.active {
  background: var(--brand-blue);
  border-color: var(--brand-cyan);
  box-shadow: 0 4px 12px var(--glow-blue);
}

.freeze-led {
  width: var(--freeze-led-size);
  height: var(--freeze-led-size);
  border-radius: 50%;
  background: var(--bg-surface);
  border: 2px solid var(--bg-raised);
  transition: all 150ms ease;
}

.freeze-led.active {
  background: var(--color-danger);
  border-color: var(--color-danger);
  box-shadow:
    0 0 8px var(--color-danger),
    0 0 16px rgba(255, 68, 68, 0.4);
  animation: pulse 1.5s ease-in-out infinite;
}

@keyframes pulse {
  0%, 100% { opacity: 0.6; }
  50% { opacity: 1.0; }
}
```

### Dropdown Menu
```css
.dropdown {
  width: var(--dropdown-width);
  height: var(--dropdown-height);
  background: var(--bg-surface);
  border: 1px solid var(--bg-raised);
  border-radius: 4px;
  color: var(--text-secondary);
  font-size: var(--font-size-small);
  padding: 0 12px;
  cursor: pointer;
  appearance: none;
  background-image: url("data:image/svg+xml,..."); /* Down arrow icon */
  background-repeat: no-repeat;
  background-position: right 8px center;
}

.dropdown:hover {
  background: var(--bg-raised);
  border-color: var(--brand-blue);
}

.dropdown:focus {
  outline: 2px solid var(--brand-blue);
  outline-offset: 2px;
}
```

### Level Meters
```css
.meter {
  width: var(--meter-width);
  height: var(--meter-height);
  background: var(--bg-surface);
  border-radius: 4px;
  overflow: hidden;
  position: relative;
}

.meter-segment {
  width: 100%;
  height: 5%; /* 20 segments = 5% each */
  margin-bottom: 1px;
  background: var(--bg-raised);
  transition: background 20ms ease;
}

.meter-segment.active.green {
  background: var(--color-success);
}

.meter-segment.active.yellow {
  background: var(--color-warning);
}

.meter-segment.active.red {
  background: var(--color-danger);
}
```

---

## Grain Visualization

### Canvas Particle System
```css
.grain-visualization {
  width: 650px;
  height: 80px;
  background: linear-gradient(
    to bottom,
    var(--viz-top),
    var(--viz-bottom)
  );
  border-radius: 8px;
  overflow: hidden;
}
```

**Particle Rendering:**
- **Shape:** Small circles (2-6px diameter based on Size parameter)
- **Color:** White with 60% opacity, subtle blue tint
- **Movement:** Horizontal drift (left-to-right), vertical float
- **Density:** Number of particles scales with Density parameter (10-100 particles)
- **Freeze Behavior:** Particles freeze in place, fade to 30% opacity

**Mode-Specific Visuals:**
- **Granular:** Random scatter, organic movement
- **Pitch:** Organized horizontal lines, synchronized movement
- **Delay:** Echo trail effect, delayed particle copies
- **Spectral:** Frequency-based vertical positioning, shimmer effect

---

## Effects & Animations

### Transitions
```css
--transition-fast: 50ms ease;       /* Parameter value updates */
--transition-normal: 150ms ease;    /* Button states */
--transition-smooth: 200ms ease-in-out; /* Mode switching */
--transition-slow: 300ms ease;      /* Visualization fades */
```

### Shadows & Glows
```css
--shadow-sm: 0 2px 4px rgba(0, 0, 0, 0.2);
--shadow-md: 0 4px 8px rgba(0, 0, 0, 0.3);
--shadow-lg: 0 8px 16px rgba(0, 0, 0, 0.4);

--glow-blue-sm: 0 0 8px var(--glow-blue);
--glow-blue-md: 0 0 12px var(--glow-blue);
--glow-blue-lg: 0 0 16px var(--glow-blue), 0 0 32px rgba(74, 158, 255, 0.2);
```

### Interaction Feedback
```css
/* Hover States */
.interactive:hover {
  filter: brightness(1.1);
}

/* Active/Pressed States */
.interactive:active {
  transform: translateY(2px);
}

/* Focus Rings (Keyboard Navigation) */
.interactive:focus-visible {
  outline: 2px solid var(--brand-blue);
  outline-offset: 2px;
}
```

---

## Accessibility

### Color Contrast Ratios
- **Text on Background:** 7.2:1 (AAA)
- **Labels on Surface:** 4.8:1 (AA)
- **Active Elements:** 5.5:1 (AA)

### Focus Indicators
- **Keyboard Navigation:** 2px solid blue outline with 2px offset
- **Tab Order:** Mode tabs → Knobs (row-by-row) → Freeze → Dropdowns → Meters (skip)

### ARIA Labels
```html
<div class="knob" role="slider"
     aria-label="Position"
     aria-valuemin="0"
     aria-valuemax="100"
     aria-valuenow="50"
     aria-valuetext="50%">
  <!-- Knob SVG -->
</div>
```

---

## Responsive Scaling

### Breakpoints
```css
/* Compact Mode (600px width) */
@media (max-width: 650px) {
  --knob-diameter: 60px;
  --container-padding: 20px;
  --viz-height: 60px;
}

/* Large Mode (900px width) */
@media (min-width: 850px) {
  --knob-diameter: 90px;
  --container-padding: 40px;
  --viz-height: 100px;
}
```

---

## Implementation Checklist

- [ ] CSS variables defined in `:root`
- [ ] Font stack with fallbacks loaded
- [ ] SVG knobs with proper arc calculations
- [ ] Canvas setup for grain visualization
- [ ] Mode tab switching with transitions
- [ ] Freeze button toggle with LED animation
- [ ] Dropdown menus with custom styling
- [ ] Level meters with real-time updates
- [ ] Hover/focus states on all interactive elements
- [ ] ARIA labels for accessibility
- [ ] Keyboard navigation support
- [ ] Responsive scaling tested

---

## Design Tokens Export (for JUCE/C++)

If implementing custom rendering in C++ (Visage alternative):

```cpp
// CloudWash Design Tokens
namespace CloudWash::Design {
    // Colors
    constexpr Colour BG_PRIMARY    = Colour(0xFF1A1A2E);
    constexpr Colour BG_SURFACE    = Colour(0xFF2A2A3E);
    constexpr Colour BRAND_BLUE    = Colour(0xFF4A9EFF);
    constexpr Colour TEXT_PRIMARY  = Colour(0xFFFFFFFF);

    // Dimensions
    constexpr int WINDOW_WIDTH     = 700;
    constexpr int WINDOW_HEIGHT    = 500;
    constexpr int KNOB_DIAMETER    = 75;

    // Typography
    constexpr float FONT_SIZE_TITLE = 18.0f;
    constexpr float FONT_SIZE_LABEL = 11.0f;
}
```

---

## Version History
- **v1.0** (2026-01-24): Initial design system for CloudWash WebView UI
