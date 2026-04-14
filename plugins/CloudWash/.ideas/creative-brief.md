# CloudWash - Creative Brief

## Hook
**"The legendary Mutable Instruments Clouds eurorack module, reimagined as a modern VST plugin."**

CloudWash brings the iconic granular texture synthesizer from the eurorack world into your DAW. Transform any audio source into lush ambient clouds, shimmering textures, glitchy rhythms, or otherworldly soundscapes with four distinct processing modes.

## Vision
A faithful yet enhanced VST adaptation of the Mutable Instruments Clouds granular processor, featuring:
- **4 Processing Modes:** Granular, Pitch-shifter/Time-stretcher, Looping Delay, Spectral Processing
- **Complete Parameter Set:** All original controls including Position, Size, Density, Texture, Pitch, and more
- **Modern Interface:** Clean, spacious layout optimized for DAW workflow with large, precise controls
- **Studio-Ready:** Professional quality settings with adjustable buffer sizes and sample rates

## Sonic Character
- **Granular Mode:** Dense clouds of micro-grains, from subtle textures to complete sound transformation
- **Pitch Mode:** Pristine time-stretching and pitch-shifting without the typical artifacts
- **Looping Delay Mode:** Evolving, textured delays with feedback and reverb
- **Spectral Mode:** Frequency-domain madness for experimental sound design

## Target Use Cases
1. **Ambient Production:** Lush, evolving pads and textures
2. **Sound Design:** Transform recordings into unique sonic materials
3. **Live Performance:** Real-time granular processing with freeze functionality
4. **Experimental Music:** Spectral processing and glitch generation
5. **Post-Production:** Creative texture layering and atmosphere creation

## Unique Selling Points
- Based on open-source VCV Rack implementation of the original Clouds DSP
- All 4 processing modes from the hardware unit
- **Vintage/Modern Processing:** Choose between hardware-faithful 32kHz or transparent native sample rate
- Modern UI with improved visibility and control precision
- Studio-quality audio path with flexible quality settings
- Freeze function for infinite sustain and looping

## Technical Foundation
- Source reference: https://github.com/VCVRack/AudibleInstruments/blob/v2/src/Clouds.cpp
- **Dual processing modes:**
  - **Vintage:** 32kHz internal rate (hardware-faithful character)
  - **Modern:** Native host sample rate (44.1-192kHz, transparent quality)
- Stereo input/output
- Block-based granular engine with 256-frame buffer
- Four quality presets balancing buffer time vs. fidelity

## Design Philosophy
**Clean, modern, and inviting** - while the eurorack original has a dark, minimal aesthetic, CloudWash embraces a spacious, easy-to-read interface with:
- Large, smooth controls for precise parameter adjustments
- Clear visual hierarchy separating core controls from modulation
- Soft color palette evoking the "cloud" theme
- Mode selector prominently displayed
- Real-time visual feedback for grain activity
