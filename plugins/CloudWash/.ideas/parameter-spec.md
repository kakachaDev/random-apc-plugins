# CloudWash - Parameter Specification

## Core Controls (Always Visible)

| ID | Name | Type | Range | Default | Unit | Description |
|:---|:---|:---|:---|:---|:---|:---|
| `position` | Position | Float | 0.0 - 1.0 | 0.5 | normalized | Grain playback position in buffer |
| `size` | Size | Float | 0.0 - 1.0 | 0.5 | normalized | Grain size/length |
| `pitch` | Pitch | Float | -2.0 - 2.0 | 0.0 | octaves | Grain pitch shift (±2 octaves) |
| `density` | Density | Float | 0.0 - 1.0 | 0.5 | normalized | Grain frequency/density |
| `texture` | Texture | Float | 0.0 - 1.0 | 0.5 | normalized | Grain shape and character |

## Input/Output Controls

| ID | Name | Type | Range | Default | Unit | Description |
|:---|:---|:---|:---|:---|:---|:---|
| `in_gain` | In Gain | Float | 0.0 - 1.0 | 0.8 | normalized | Input signal level |
| `blend` | Blend | Float | 0.0 - 1.0 | 0.5 | normalized | Wet/dry mix |
| `spread` | Stereo Spread | Float | 0.0 - 1.0 | 0.0 | normalized | Stereo width/panning |
| `feedback` | Feedback | Float | 0.0 - 1.0 | 0.0 | normalized | Feedback amount |
| `reverb` | Reverb | Float | 0.0 - 1.0 | 0.0 | normalized | Reverb amount |

## Mode & State Controls

| ID | Name | Type | Range | Default | Unit | Description |
|:---|:---|:---|:---|:---|:---|:---|
| `mode` | Mode | Choice | 0 - 3 | 0 | enum | Processing mode selector |
| `freeze` | Freeze | Bool | 0 - 1 | 0 | toggle | Freeze buffer for infinite sustain |
| `quality` | Quality | Choice | 0 - 3 | 2 | enum | Quality/buffer size preset |
| `sample_mode` | Sample Mode | Choice | 0 - 1 | 0 | enum | Processing rate (Vintage 32kHz vs Modern Native) |

## Mode Selector Values

| Value | Mode Name | Description |
|:---|:---|:---|
| 0 | Granular | Classic granular synthesis mode |
| 1 | Pitch Shifter | Pitch-shifting and time-stretching |
| 2 | Looping Delay | Textured delay with feedback |
| 3 | Spectral | Frequency-domain processing |

## Sample Mode Values

| Value | Mode Name | Processing Rate | Description |
|:---|:---|:---|:---|
| 0 | Vintage | 32 kHz | Hardware-faithful 32kHz internal processing (classic lo-fi character) |
| 1 | Modern | Native (44.1-192 kHz) | Runs at host sample rate for maximum transparency and quality |

**Key Differences:**
- **Vintage (32kHz)**: Faithful to original Mutable Instruments Clouds hardware. Adds characteristic warmth and slight aliasing. Lower CPU usage.
- **Modern (Native)**: Transparent, high-fidelity processing at host sample rate. Better high-frequency response, no aliasing. Higher CPU usage at 96kHz+.

## Quality Settings

**Note:** Quality settings work in conjunction with Sample Mode. Buffer durations listed below apply to Vintage mode (32kHz). In Modern mode, buffer durations scale proportionally with sample rate.

| Value | Name | Buffer Duration (32kHz) | Bit Depth | Lo-Fi Processing | Description |
|:---|:---|:---|:---|:---|:---|
| 0 | High Fidelity | 1 second | 16-bit stereo | None | Shortest buffer, pristine quality |
| 1 | Balanced | 2 seconds | 16-bit stereo | None | Good balance of buffer time and quality |
| 2 | Extended | 4 seconds | 8-bit µ-law stereo | Downsampling + compression | Longer buffer with lo-fi character |
| 3 | Maximum | 8 seconds | 8-bit µ-law mono | Aggressive downsampling + compression | Longest buffer, vintage lo-fi mono |

**Quality + Sample Mode Interaction:**
- **Vintage + Quality 0-1**: Hardware-accurate with full-bandwidth processing
- **Vintage + Quality 2-3**: Hardware-accurate with lo-fi bit reduction and downsampling
- **Modern + Quality 0-1**: Full transparency at native sample rate
- **Modern + Quality 2-3**: Lo-fi bit reduction applied to native-rate signal for creative effect

## Parameter Behavior Notes

### Position
- In **Granular mode**: Selects playback position in the frozen/recorded buffer
- In **Pitch mode**: Controls time-stretching anchor point
- In **Delay mode**: Sets delay tap position
- In **Spectral mode**: Shifts frequency analysis window

### Size
- Controls grain window length in milliseconds
- Smaller values = glitchy, granular
- Larger values = smooth, pad-like

### Density
- Low values: Sparse, individual grains audible
- High values: Dense cloud, continuous texture

### Texture
- Mode-dependent parameter that shapes grain characteristics
- **Granular**: Grain envelope shape
- **Pitch**: Formant preservation
- **Delay**: Diffusion amount
- **Spectral**: Frequency smearing

### Blend Behavior (Mode-Dependent)
The blend parameter changes function based on mode:
- **Mode 0 (Granular)**: Wet/dry mix
- **Mode 1 (Pitch)**: Stereo spread
- **Mode 2 (Delay)**: Feedback amount
- **Mode 3 (Spectral)**: Reverb amount

### Freeze
- When enabled: Stops recording new input, loops current buffer
- Allows infinite sustain and buffer manipulation
- LED indicator for visual feedback

## DSP Implementation Notes

- **Internal processing rate:**
  - **Vintage mode**: 32 kHz (hardware-faithful)
  - **Modern mode**: Native host sample rate (44.1kHz, 48kHz, 96kHz, 192kHz)
- **Block size**: 32 samples per block (at internal rate)
- **Input buffer**: 256 stereo frames (duration varies with sample rate)
- **Stereo input** with mono fallback (right channel mirrors left if mono)
- **Schmitt trigger** for freeze functionality
- **Grain engine** processes in blocks for efficiency
- **Sample rate conversion**: When in Vintage mode, input is downsampled to 32kHz and output is upsampled back to host rate
- **Quality modes 2-3**: Apply additional downsampling (16kHz) and bit reduction on top of sample mode selection

## Automation & CV Mapping Priority

**High Priority** (most expressive):
1. Position
2. Size
3. Texture
4. Density
5. Pitch

**Medium Priority**:
6. Blend
7. Feedback
8. Reverb
9. Spread

**Low Priority**:
10. In Gain
11. Mode (stepped parameter)
12. Freeze (toggle)
13. Quality (global setting)
14. Sample Mode (global setting)

## Total Parameter Count
- **14 parameters** total
- 10 continuous (float)
- 3 discrete (mode, quality, sample_mode)
- 1 boolean (freeze)
