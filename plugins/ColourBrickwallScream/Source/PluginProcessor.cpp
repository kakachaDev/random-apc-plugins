#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <algorithm>

//==============================================================================
// PARAMETER LAYOUT
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
ColourBrickwallScreamAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Drive: 0.0–1.0 (mapped to 0–40dB in processor)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::DRIVE, "Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f));

    // Character: 0–5 enum (Tube/Tape/Transformer/Diode/Bitcrush/Full Scream)
    layout.add (std::make_unique<juce::AudioParameterInt> (
        ParameterIDs::CHARACTER, "Character", 0, 5, 0));

    // Tone: -1.0–1.0 (dual-shelf tilt EQ)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::TONE, "Tone",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));

    // Colour: 0.0–1.0 (per-variant harmonic texture)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::COLOUR, "Colour",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    // Ceiling: -12.0–0.0 dBFS
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::CEILING, "Ceiling",
        juce::NormalisableRange<float> (-12.0f, 0.0f, 0.01f), -0.3f));

    // Mix: 0.0–1.0 (dry/wet)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::MIX, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));

    return layout;
}

//==============================================================================
// CONSTRUCTOR / DESTRUCTOR
//==============================================================================
ColourBrickwallScreamAudioProcessor::ColourBrickwallScreamAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ColourBrickwallScream", createParameterLayout())
{
}

ColourBrickwallScreamAudioProcessor::~ColourBrickwallScreamAudioProcessor()
{
}

//==============================================================================
// PREPARE TO PLAY
//==============================================================================
void ColourBrickwallScreamAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRate_  = sampleRate;
    blockSize_   = samplesPerBlock;
    numChannels_ = std::min (getTotalNumOutputChannels(), 2);

    setLatencySamples (0);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32> (numChannels_);

    // ── Drive smoothing (20ms) ────────────────────────────────────────────────
    smoothDrive.reset (sampleRate, 0.020);
    smoothDrive.setCurrentAndTargetValue (
        apvts.getRawParameterValue (ParameterIDs::DRIVE)->load());

    // ── Tape LPF (1-pole, ~14kHz) ─────────────────────────────────────────────
    const double tapeCutoff = 14000.0;
    tapeLPFCoeff = static_cast<float> (
        std::exp (-2.0 * juce::MathConstants<double>::pi * tapeCutoff / sampleRate));
    tapeLPFState.fill (0.0f);

    // ── Colour filters ─────────────────────────────────────────────────────────
    for (int v = 0; v < 6; ++v)
    {
        updateColourFilter (v, sampleRate);
        colourFilters[v].prepare (spec);
        colourFilters[v].reset();
    }
    smoothColour.reset (sampleRate, 0.020);
    smoothColour.setCurrentAndTargetValue (
        apvts.getRawParameterValue (ParameterIDs::COLOUR)->load());

    // ── Tone filters ───────────────────────────────────────────────────────────
    lastToneValue = 2.0f;   // force recalculation
    for (int ch = 0; ch < numChannels_; ++ch)
    {
        toneFilterHigh[ch].reset();
        toneFilterLow[ch].reset();
    }
    updateToneFilter (apvts.getRawParameterValue (ParameterIDs::TONE)->load(), sampleRate);

    // ── Brickwall limiter ──────────────────────────────────────────────────────
    grEnvelope = 1.0f;
    attackCoeff  = static_cast<float> (std::exp (-1.0 / (0.0005 * sampleRate)));  // 0.5ms
    releaseCoeff = static_cast<float> (std::exp (-1.0 / (0.200  * sampleRate)));  // 200ms

    // ── Dry buffer ─────────────────────────────────────────────────────────────
    dryBuffer.setSize (numChannels_, samplesPerBlock);
    dryBuffer.clear();

    // ── Mix smoothing ──────────────────────────────────────────────────────────
    smoothMix.reset (sampleRate, 0.020);
    smoothMix.setCurrentAndTargetValue (
        apvts.getRawParameterValue (ParameterIDs::MIX)->load());

    // ── Temper crossfade ───────────────────────────────────────────────────────
    currentChar  = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::CHARACTER)->load());
    targetChar   = currentChar;
    crossfadePos = 1.0f;
    crossfadeInc = 0.0f;

    const float crossfadeSamples = static_cast<float> (0.004 * sampleRate);
    crossfadeInc = (crossfadeSamples > 0.0f) ? (1.0f / crossfadeSamples) : 1.0f;

    // ── Meter state ───────────────────────────────────────────────────────────
    meterInSmoothed  = 0.0f;
    meterOutSmoothed = 0.0f;
    grMeterSmoothed  = 0.0f;
}

void ColourBrickwallScreamAudioProcessor::releaseResources()
{
}

//==============================================================================
// SATURATION TRANSFER FUNCTIONS
//
// All functions receive x = input * driveGain where driveGain = 10^(drive*40/20).
// At drive=0.25: driveGain ≈ 3.2x  (x can reach ±3 for 0dBFS in)
// At drive=0.5:  driveGain ≈ 10x   (x can reach ±10)
// Each function must produce distinct harmonic character in the ±1 to ±10 range.
//==============================================================================

// Tube: asymmetric x/(1+|x|) — positive half clips harder than negative.
// Heavy asymmetry → dominant 2nd harmonic (even: warm, musical).
float ColourBrickwallScreamAudioProcessor::saturateTube (float x, float /*drive*/) noexcept
{
    if (x >= 0.0f)
        return x / (1.0f + x);            // clips to 1.0 (positive asymptote)
    else
        return x / (1.0f - x * 0.55f);   // softer negative side (clips to −1/0.55 ≈ −1.82)
}

// Tape: symmetric tanh core + 3rd-harmonic flavour injection.
// Soft & even, with a slight "woolly" presence from harmonic stacking.
float ColourBrickwallScreamAudioProcessor::saturateTape (float x, float /*drive*/) noexcept
{
    const float t1 = std::tanh (x);                         // fundamental saturation
    const float t3 = std::tanh (x * 2.5f) / 2.5f;          // compressed 3rd harmonic layer
    return t1 * 0.80f + t3 * 0.20f;
}

// Transformer: linear → sharp knee at ±0.7, then quick hard saturation.
// Punchy "slam": transients pass clean then hit a wall.
float ColourBrickwallScreamAudioProcessor::saturateTransformer (float x, float /*drive*/) noexcept
{
    const float absX = std::abs (x);
    const float threshold = 0.70f;
    if (absX <= threshold)
        return x;
    const float excess = absX - threshold;
    // Hard saturation beyond knee: approaches threshold + 0.30 max
    const float sat = threshold + std::tanh (excess * 3.0f) * 0.30f;
    return std::copysign (sat, x);
}

// Diode: hard asymmetric clip — guitar-pedal silicon diode character.
// Positive half clips at 0.60 (very hard), negative at 0.85 (slightly softer).
// Large asymmetry → strong even + odd harmonics, bright and aggressive.
float ColourBrickwallScreamAudioProcessor::saturateDiode (float x, float /*drive*/) noexcept
{
    if (x > 0.60f)
        return 0.60f + (x - 0.60f) * 0.08f;   // 8% ratio past threshold: nearly flat
    if (x < -0.85f)
        return -0.85f + (x + 0.85f) * 0.15f;  // slightly softer negative clip
    return x;
}

float ColourBrickwallScreamAudioProcessor::getBitDepthFromDrive (float normDrive) noexcept
{
    // Drive 0→1 maps to 16→4 bits (higher drive = more quantisation)
    return 16.0f - normDrive * 12.0f;
}

// Bitcrush: pre-clip via tanh then quantise to drive-dependent bit depth.
// Creates staircase distortion — instantly recognisable digital texture.
float ColourBrickwallScreamAudioProcessor::saturateBitcrush (float x, float drive) noexcept
{
    const float bits  = getBitDepthFromDrive (drive);
    const float steps = std::pow (2.0f, std::max (2.0f, bits - 1.0f));
    const float clamped = std::tanh (x);   // soft-clip to ±1 before quantising
    return std::round (clamped * steps) / steps;
}

// Full Scream: wavefolding via sin — wildly different harmonic structure.
// At drive=0.25: foldGain=3.25 (≈3 folds); at drive=1.0: foldGain=8 (≈8 folds).
// Each additional fold inverts the waveform, cascading harmonics.
float ColourBrickwallScreamAudioProcessor::saturateFullScream (float x, float drive) noexcept
{
    const float foldGain = 3.0f + drive * 5.0f;
    return std::sin (juce::MathConstants<float>::pi * x * foldGain);
}

//==============================================================================
// COLOUR FILTER SETUP
// Each character gets a high-gain filter that pushes a signature frequency
// band hard into the post-saturation tanh, creating distinct harmonic textures.
//==============================================================================
void ColourBrickwallScreamAudioProcessor::ColourFilter::prepare (const juce::dsp::ProcessSpec& spec)
{
    for (int ch = 0; ch < 2; ++ch)
        filter[ch].prepare (spec);
}

void ColourBrickwallScreamAudioProcessor::ColourFilter::reset()
{
    for (int ch = 0; ch < 2; ++ch)
        filter[ch].reset();
}

float ColourBrickwallScreamAudioProcessor::ColourFilter::processSample (int ch, float x)
{
    return filter[ch].processSample (x);
}

void ColourBrickwallScreamAudioProcessor::ColourFilter::setCoeffs (BiquadCoeffs::Ptr c)
{
    for (int ch = 0; ch < 2; ++ch)
        *filter[ch].coefficients = *c;
}

void ColourBrickwallScreamAudioProcessor::updateColourFilter (int variant, double sr)
{
    BiquadCoeffs::Ptr c;

    switch (variant)
    {
        case 0:  // Tube: fat low-mid warmth (+14dB at 250Hz, Q=0.5)
            c = BiquadCoeffs::makePeakFilter (sr, 250.0, 0.5, 14.0f);
            break;
        case 1:  // Tape: presence + air (+14dB high shelf at 3.5kHz)
            c = BiquadCoeffs::makeHighShelf (sr, 3500.0, 0.7, 14.0f);
            break;
        case 2:  // Transformer: aggressive mid honk (+15dB at 1.5kHz, Q=2.5)
            c = BiquadCoeffs::makePeakFilter (sr, 1500.0, 2.5, 15.0f);
            break;
        case 3:  // Diode: sibilant bite (+15dB at 5kHz, Q=3.0)
            c = BiquadCoeffs::makePeakFilter (sr, 5000.0, 3.0, 15.0f);
            break;
        case 4:  // Bitcrush: harsh presence (+15dB at 10kHz, Q=4.0)
            c = BiquadCoeffs::makePeakFilter (sr, 10000.0, 4.0, 15.0f);
            break;
        case 5:  // Full Scream: subharmonic rumble (+16dB low shelf at 100Hz)
            c = BiquadCoeffs::makeLowShelf (sr, 100.0, 0.5, 16.0f);
            break;
        default:
            c = BiquadCoeffs::makeAllPass (sr, 1000.0, 0.7);
            break;
    }

    colourFilters[variant].setCoeffs (c);
}

//==============================================================================
// TONE FILTER UPDATE — dual-shelf tilt EQ
//   Positive: +high shelf (3.5kHz) + -low shelf (250Hz) → brighter
//   Negative: -high shelf (3.5kHz) + +low shelf (250Hz) → warmer
//   Zero: both shelves at 0dB → transparent
//==============================================================================
void ColourBrickwallScreamAudioProcessor::updateToneFilter (float toneValue, double sr)
{
    lastToneValue = toneValue;

    const float gainHigh = toneValue * 9.0f;   // ±9dB high shelf
    const float gainLow  = -toneValue * 6.0f;  // ±6dB low shelf (opposite polarity)

    auto cHigh = BiquadCoeffs::makeHighShelf (sr, 3500.0, 0.7, static_cast<double> (gainHigh));
    auto cLow  = BiquadCoeffs::makeLowShelf  (sr, 250.0,  0.7, static_cast<double> (gainLow));

    for (int ch = 0; ch < numChannels_; ++ch)
    {
        *toneFilterHigh[ch].coefficients = *cHigh;
        *toneFilterLow[ch].coefficients  = *cLow;
    }
}

//==============================================================================
// BUS LAYOUT
//==============================================================================
#ifndef JucePlugin_PreferredChannelConfigurations
bool ColourBrickwallScreamAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

//==============================================================================
// PROCESS BLOCK
//==============================================================================
void ColourBrickwallScreamAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                         juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = std::min (buffer.getNumChannels(), numChannels_);

    if (numSamples == 0 || numCh == 0)
        return;

    // Clear extra output channels
    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    // ── Read parameters ────────────────────────────────────────────────────────
    const float driveNorm  = apvts.getRawParameterValue (ParameterIDs::DRIVE)->load();
    const int   charParam  = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::CHARACTER)->load());
    const float colourNorm = apvts.getRawParameterValue (ParameterIDs::COLOUR)->load();
    const float toneNorm   = apvts.getRawParameterValue (ParameterIDs::TONE)->load();
    const float ceilDb     = apvts.getRawParameterValue (ParameterIDs::CEILING)->load();
    const float mixNorm    = apvts.getRawParameterValue (ParameterIDs::MIX)->load();

    // Update smoother targets
    smoothDrive.setTargetValue  (driveNorm);
    smoothColour.setTargetValue (colourNorm);
    smoothMix.setTargetValue    (mixNorm);

    // Handle character change → crossfade
    if (charParam != targetChar)
    {
        currentChar  = targetChar;
        targetChar   = charParam;
        crossfadePos = 0.0f;
    }

    // Update tone filter at block rate when value changes
    if (std::abs (toneNorm - lastToneValue) > 0.005f)
        updateToneFilter (toneNorm, sampleRate_);

    // ── Input metering ─────────────────────────────────────────────────────────
    float inputPeak = 0.0f;
    for (int ch = 0; ch < numCh; ++ch)
        inputPeak = std::max (inputPeak, buffer.getMagnitude (ch, 0, numSamples));

    meterInSmoothed = (inputPeak > meterInSmoothed)
        ? meterInSmoothed + (inputPeak - meterInSmoothed) * kMeterAttack
        : meterInSmoothed + (inputPeak - meterInSmoothed) * kMeterRelease;
    meterInput.store (meterInSmoothed);

    // ── Store dry copy ─────────────────────────────────────────────────────────
    for (int ch = 0; ch < numCh; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    // ── Per-sample processing ──────────────────────────────────────────────────
    for (int i = 0; i < numSamples; ++i)
    {
        const float driveSample  = smoothDrive.getNextValue();
        const float colourSample = smoothColour.getNextValue();

        // Drive: 0–1 → 0–40dB pre-gain
        const float driveGain = juce::Decibels::decibelsToGain (driveSample * 40.0f);

        for (int ch = 0; ch < numCh; ++ch)
        {
            float x = buffer.getSample (ch, i) * driveGain;

            // ── Saturation (Temper + crossfade) ────────────────────────────────
            auto applySat = [&] (int variant, float samp) -> float
            {
                switch (variant)
                {
                    case 0: return saturateTube        (samp, driveSample);
                    case 1:
                    {
                        float s = saturateTape (samp, driveSample);
                        // 1-pole tape roll-off (~14kHz)
                        s = s * (1.0f - tapeLPFCoeff) + tapeLPFState[ch] * tapeLPFCoeff;
                        tapeLPFState[ch] = s;
                        return s;
                    }
                    case 2: return saturateTransformer (samp, driveSample);
                    case 3: return saturateDiode       (samp, driveSample);
                    case 4: return saturateBitcrush    (samp, driveSample);
                    case 5: return saturateFullScream  (samp, driveSample);
                    default: return samp;
                }
            };

            float satOut;
            if (crossfadePos >= 1.0f)
            {
                satOut = applySat (targetChar, x);
            }
            else
            {
                const float s1 = applySat (currentChar, x);
                const float s2 = applySat (targetChar,  x);
                satOut = s1 + crossfadePos * (s2 - s1);
            }

            // ── Colour: harmonic texture blend ─────────────────────────────────
            // The character filter strongly boosts a signature frequency band.
            // Passing that through tanh creates harmonics from those boosted freqs.
            // Squared blend curve: even at 50% colour the effect is clearly audible.
            {
                const float filtered = colourFilters[targetChar].processSample (ch, satOut);
                const float resat    = std::tanh (filtered);
                const float blend    = colourSample * colourSample;
                x = satOut * (1.0f - blend) + resat * blend;
            }

            // ── Tone: dual-shelf tilt EQ ───────────────────────────────────────
            x = toneFilterHigh[ch].processSample (x);
            x = toneFilterLow[ch].processSample  (x);

            buffer.setSample (ch, i, x);
        }

        // Advance crossfade
        if (crossfadePos < 1.0f)
            crossfadePos = std::min (1.0f, crossfadePos + crossfadeInc);
    }

    // ── Brickwall Limiter ──────────────────────────────────────────────────────
    // Simple per-sample peak limiter. Uses raw ceilDb parameter directly
    // (no smoother — fixes the bug where smoothCeiling.getCurrentValue()
    // was never advanced and stayed at the initial value forever).
    {
        const float ceilingLinear = juce::Decibels::decibelsToGain (ceilDb);

        for (int i = 0; i < numSamples; ++i)
        {
            // Peak across all channels at this sample
            float peak = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
                peak = std::max (peak, std::abs (buffer.getSample (ch, i)));

            const float targetGR = (peak > ceilingLinear && peak > 1e-6f)
                                   ? (ceilingLinear / peak) : 1.0f;

            if (targetGR < grEnvelope)
                grEnvelope = grEnvelope * attackCoeff  + targetGR * (1.0f - attackCoeff);
            else
                grEnvelope = grEnvelope * releaseCoeff + targetGR * (1.0f - releaseCoeff);

            grEnvelope = juce::jlimit (0.0f, 1.0f, grEnvelope);

            for (int ch = 0; ch < numCh; ++ch)
                buffer.setSample (ch, i, buffer.getSample (ch, i) * grEnvelope);
        }

        // GR meter
        const float grReduction = 1.0f - grEnvelope;
        grMeterSmoothed = (grReduction > grMeterSmoothed)
            ? grMeterSmoothed + (grReduction - grMeterSmoothed) * kMeterAttack
            : grMeterSmoothed + (grReduction - grMeterSmoothed) * kMeterRelease;
        meterGR.store (grMeterSmoothed);
    }

    // ── Mix blend (dry / wet) ──────────────────────────────────────────────────
    for (int i = 0; i < numSamples; ++i)
    {
        const float wet = smoothMix.getNextValue();
        const float dry = 1.0f - wet;

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float dryS = dryBuffer.getSample (ch, i);
            const float wetS = buffer.getSample (ch, i);
            buffer.setSample (ch, i, dry * dryS + wet * wetS);
        }
    }

    // ── Output metering ────────────────────────────────────────────────────────
    float outputPeak = 0.0f;
    for (int ch = 0; ch < numCh; ++ch)
        outputPeak = std::max (outputPeak, buffer.getMagnitude (ch, 0, numSamples));

    meterOutSmoothed = (outputPeak > meterOutSmoothed)
        ? meterOutSmoothed + (outputPeak - meterOutSmoothed) * kMeterAttack
        : meterOutSmoothed + (outputPeak - meterOutSmoothed) * kMeterRelease;
    meterOutput.store (meterOutSmoothed);
}

//==============================================================================
// EDITOR
//==============================================================================
juce::AudioProcessorEditor* ColourBrickwallScreamAudioProcessor::createEditor()
{
    return new ColourBrickwallScreamAudioProcessorEditor (*this);
}

bool ColourBrickwallScreamAudioProcessor::hasEditor() const { return true; }

//==============================================================================
// PLUGIN INFO
//==============================================================================
const juce::String ColourBrickwallScreamAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ColourBrickwallScreamAudioProcessor::acceptsMidi()  const { return false; }
bool ColourBrickwallScreamAudioProcessor::producesMidi() const { return false; }
bool ColourBrickwallScreamAudioProcessor::isMidiEffect() const { return false; }
double ColourBrickwallScreamAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int ColourBrickwallScreamAudioProcessor::getNumPrograms()    { return 1; }
int ColourBrickwallScreamAudioProcessor::getCurrentProgram() { return 0; }
void ColourBrickwallScreamAudioProcessor::setCurrentProgram (int) {}
const juce::String ColourBrickwallScreamAudioProcessor::getProgramName (int) { return {}; }
void ColourBrickwallScreamAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
// STATE
//==============================================================================
void ColourBrickwallScreamAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ColourBrickwallScreamAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// PLUGIN ENTRY POINT
//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ColourBrickwallScreamAudioProcessor();
}
