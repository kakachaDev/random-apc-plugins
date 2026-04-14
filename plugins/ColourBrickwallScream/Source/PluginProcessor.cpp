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

    // Drive/Scream: 0.0–1.0 (mapped to 0–40dB in processor)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::DRIVE, "Drive / Scream",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f));

    // Character: 0–5 enum (Tube/Tape/Transformer/Diode/Bitcrush/Full Scream)
    layout.add (std::make_unique<juce::AudioParameterInt> (
        ParameterIDs::CHARACTER, "Character", 0, 5, 0));

    // Tone: -1.0–1.0 (tilt EQ ±6dB)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::TONE, "Tone",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));

    // Colour: 0.0–1.0 (per-variant harmonic blend)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::COLOUR, "Colour",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    // Attack Character: 0.0–1.0 (mapped log to 0.1ms–50ms)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::ATTACK_CHARACTER, "Attack Character",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    // Ceiling: -12.0–0.0 dBFS
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::CEILING, "Ceiling",
        juce::NormalisableRange<float> (-12.0f, 0.0f, 0.01f), -0.3f));

    // Mix: 0.0–1.0 (dry/wet)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::MIX, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));

    // Output Gain: -12.0–12.0 dB
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParameterIDs::OUTPUT_GAIN, "Output Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f));

    return layout;
}

//==============================================================================
// CONSTRUCTOR / DESTRUCTOR
//==============================================================================
ColourBrickwallScreamAudioProcessor::ColourBrickwallScreamAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ColourBrickwallScream", createParameterLayout()),
      oversampler (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true)
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

    // Lookahead: 2ms
    lookaheadSamples = static_cast<int> (std::ceil (0.002 * sampleRate));
    setLatencySamples (lookaheadSamples);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32> (numChannels_);

    // ── Drive smoothing (20ms) ─────────────────────────────────────────────────
    smoothDrive.reset (sampleRate, 0.020);
    smoothDrive.setCurrentAndTargetValue (
        apvts.getRawParameterValue (ParameterIDs::DRIVE)->load());

    // ── Tape LPF (simple 1-pole, ~14kHz) ──────────────────────────────────────
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

    // ── Tone filter ────────────────────────────────────────────────────────────
    lastToneValue = 2.0f;  // force recalculation
    for (int ch = 0; ch < numChannels_; ++ch)
        toneFilter[ch].reset();
    smoothTone.reset (sampleRate, 0.020);
    smoothTone.setCurrentAndTargetValue (
        apvts.getRawParameterValue (ParameterIDs::TONE)->load());

    // ── Brickwall limiter ──────────────────────────────────────────────────────
    lookaheadBuf.setSize (numChannels_, lookaheadSamples + samplesPerBlock + 16);
    lookaheadBuf.clear();
    lookaheadWritePos = 0;
    grEnvelope = 1.0f;

    oversampler.initProcessing (static_cast<size_t> (samplesPerBlock));
    oversampler.reset();

    smoothAttack.reset (sampleRate, 0.020);
    smoothCeiling.reset (sampleRate, 0.050);
    {
        float atkNorm  = apvts.getRawParameterValue (ParameterIDs::ATTACK_CHARACTER)->load();
        float ceilDb   = apvts.getRawParameterValue (ParameterIDs::CEILING)->load();
        smoothAttack.setCurrentAndTargetValue (atkNorm);
        smoothCeiling.setCurrentAndTargetValue (ceilDb);
        updateLimiterCoeffs (atkNorm, ceilDb, sampleRate);
    }
    releaseCoeff = static_cast<float> (
        std::exp (-1.0 / (0.200 * sampleRate)));  // 200ms fixed release

    // ── Dry buffer ─────────────────────────────────────────────────────────────
    dryBuffer.setSize (numChannels_, lookaheadSamples + samplesPerBlock + 16);
    dryBuffer.clear();
    dryWritePos = 0;

    // ── Output / mix smoothing ──────────────────────────────────────────────────
    smoothMix.reset (sampleRate, 0.020);
    smoothMix.setCurrentAndTargetValue (
        apvts.getRawParameterValue (ParameterIDs::MIX)->load());

    smoothOutputGain.reset (sampleRate, 0.020);
    smoothOutputGain.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (
            apvts.getRawParameterValue (ParameterIDs::OUTPUT_GAIN)->load()));

    // ── Temper crossfade ───────────────────────────────────────────────────────
    currentChar  = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::CHARACTER)->load());
    targetChar   = currentChar;
    crossfadePos = 1.0f;
    crossfadeInc = 0.0f;

    // Crossfade increment: full transition in 4ms
    const float crossfadeSamples = static_cast<float> (0.004 * sampleRate);
    crossfadeInc = (crossfadeSamples > 0.0f) ? (1.0f / crossfadeSamples) : 1.0f;

    // ── Meter smoothing state ───────────────────────────────────────────────────
    meterInSmoothed  = 0.0f;
    meterOutSmoothed = 0.0f;
    grMeterSmoothed  = 0.0f;
}

void ColourBrickwallScreamAudioProcessor::releaseResources()
{
    oversampler.reset();
}

//==============================================================================
// SATURATION TRANSFER FUNCTIONS
//==============================================================================

float ColourBrickwallScreamAudioProcessor::saturateTube (float x, float /*drive*/) noexcept
{
    // Devereux soft-clip: y = (3x/2)(1 - x²/3), clamped to [-1,+1]
    const float xc = juce::jlimit (-1.0f, 1.0f, x);
    return (1.5f * xc) * (1.0f - (xc * xc) / 3.0f);
}

float ColourBrickwallScreamAudioProcessor::saturateTape (float x, float /*drive*/) noexcept
{
    return std::tanh (x);
}

float ColourBrickwallScreamAudioProcessor::saturateTransformer (float x, float /*drive*/) noexcept
{
    // y = x / (1 + |x|^0.7) — asymmetric iron-core feel
    const float absX = std::abs (x);
    const float denom = 1.0f + std::pow (absX, 0.7f);
    return x / denom;
}

float ColourBrickwallScreamAudioProcessor::saturateDiode (float x, float /*drive*/) noexcept
{
    // Asymmetric hard clip: upper at 0.9, lower at -0.75
    return juce::jlimit (-0.75f, 0.9f, x);
}

float ColourBrickwallScreamAudioProcessor::getBitDepthFromDrive (float normDrive) noexcept
{
    // Map 0–1 drive to 16–4 bits (higher drive = lower bit depth = more grit)
    return 16.0f - normDrive * 12.0f;  // 16 at drive=0, 4 at drive=1
}

float ColourBrickwallScreamAudioProcessor::saturateBitcrush (float x, float drive) noexcept
{
    const float bits  = getBitDepthFromDrive (drive);
    const float steps = std::pow (2.0f, bits);
    return std::round (x * steps) / steps;
}

float ColourBrickwallScreamAudioProcessor::saturateFullScream (float x, float drive) noexcept
{
    // Wavefolding: y = sin(π * x * (1 + drive * 3))
    const float period = 1.0f + drive * 3.0f;
    return std::sin (juce::MathConstants<float>::pi * x * period);
}

//==============================================================================
// COLOUR FILTER SETUP
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
        case 0:  // Tube: low-mid warmth boost (300Hz, +4dB, Q=0.7)
            c = BiquadCoeffs::makePeakFilter (sr, 300.0, 0.7, 4.0f);
            break;
        case 1:  // Tape: presence air high shelf (10kHz, +3dB)
            c = BiquadCoeffs::makeHighShelf (sr, 10000.0, 0.7, 3.0f);
            break;
        case 2:  // Transformer: mid bloom peak (900Hz, +5dB, Q=1.2)
            c = BiquadCoeffs::makePeakFilter (sr, 900.0, 1.2, 5.0f);
            break;
        case 3:  // Diode: upper-mid bite (4kHz, +5dB, Q=1.5)
            c = BiquadCoeffs::makePeakFilter (sr, 4000.0, 1.5, 5.0f);
            break;
        case 4:  // Bitcrush: high-end grit (8kHz, +6dB, Q=2.0)
            c = BiquadCoeffs::makePeakFilter (sr, 8000.0, 2.0, 6.0f);
            break;
        case 5:  // Full Scream: subharmonic low shelf (80Hz, +6dB)
            c = BiquadCoeffs::makeLowShelf (sr, 80.0, 0.7, 6.0f);
            break;
        default:
            c = BiquadCoeffs::makeAllPass (sr, 1000.0, 0.7);
            break;
    }

    colourFilters[variant].setCoeffs (c);
}

//==============================================================================
// TONE FILTER UPDATE
//==============================================================================
void ColourBrickwallScreamAudioProcessor::updateToneFilter (float toneValue, double sr)
{
    // Tilt EQ: bipolar tone control ±1.0 → ±6dB at pivot ~1kHz
    // Positive = high shelf boost + low shelf cut (brighter)
    // Negative = low shelf boost + high shelf cut (darker)
    // We implement as a single shelving filter approximation:
    // use a high shelf centred at 1kHz, scaled by toneValue

    const float gainDb = toneValue * 6.0f;   // ±6dB

    BiquadCoeffs::Ptr c;
    if (std::abs (gainDb) < 0.05f)
    {
        c = BiquadCoeffs::makeAllPass (sr, 1000.0, 0.7);
    }
    else if (gainDb > 0.0f)
    {
        // Brighter: high shelf boost
        c = BiquadCoeffs::makeHighShelf (sr, 1000.0, 0.7, static_cast<double> (gainDb));
    }
    else
    {
        // Darker: low shelf boost (equivalent to cutting highs)
        c = BiquadCoeffs::makeLowShelf (sr, 1000.0, 0.7, static_cast<double> (-gainDb));
    }

    for (int ch = 0; ch < numChannels_; ++ch)
        *toneFilter[ch].coefficients = *c;
}

//==============================================================================
// LIMITER COEFFICIENT UPDATE
//==============================================================================
void ColourBrickwallScreamAudioProcessor::updateLimiterCoeffs (float attackNorm, float /*ceilingDb*/, double sr)
{
    // Map attack_character [0,1] → attack time [0.1ms, 50ms] log scale
    const double minAtk = 0.0001;   // 0.1ms
    const double maxAtk = 0.050;    // 50ms
    const double atkTime = minAtk * std::pow (maxAtk / minAtk, static_cast<double> (attackNorm));

    attackCoeff = static_cast<float> (std::exp (-1.0 / (atkTime * sr)));
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

    const int numSamples  = buffer.getNumSamples();
    const int numCh       = std::min (buffer.getNumChannels(), numChannels_);

    if (numSamples == 0 || numCh == 0)
        return;

    // Clear any extra output channels
    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    // ── Read parameters ────────────────────────────────────────────────────────
    const float driveNorm  = apvts.getRawParameterValue (ParameterIDs::DRIVE)->load();
    const int   charParam  = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::CHARACTER)->load());
    const float colourNorm = apvts.getRawParameterValue (ParameterIDs::COLOUR)->load();
    const float toneNorm   = apvts.getRawParameterValue (ParameterIDs::TONE)->load();
    const float atkNorm    = apvts.getRawParameterValue (ParameterIDs::ATTACK_CHARACTER)->load();
    const float ceilDb     = apvts.getRawParameterValue (ParameterIDs::CEILING)->load();
    const float mixNorm    = apvts.getRawParameterValue (ParameterIDs::MIX)->load();
    const float outGainDb  = apvts.getRawParameterValue (ParameterIDs::OUTPUT_GAIN)->load();

    // Update targets for smoothers
    smoothDrive.setTargetValue (driveNorm);
    smoothColour.setTargetValue (colourNorm);
    smoothTone.setTargetValue (toneNorm);
    smoothAttack.setTargetValue (atkNorm);
    smoothCeiling.setTargetValue (ceilDb);
    smoothMix.setTargetValue (mixNorm);
    smoothOutputGain.setTargetValue (juce::Decibels::decibelsToGain (outGainDb));

    // Handle character change → trigger crossfade
    if (charParam != targetChar)
    {
        currentChar  = targetChar;
        targetChar   = charParam;
        crossfadePos = 0.0f;
    }

    // Update limiter and tone coefficients when parameters change
    // (block-rate check, cheap enough)
    {
        const float atkSnap  = smoothAttack.getTargetValue();
        const float ceilSnap = smoothCeiling.getTargetValue();
        updateLimiterCoeffs (atkSnap, ceilSnap, sampleRate_);
    }
    if (std::abs (smoothTone.getTargetValue() - lastToneValue) > 0.005f)
    {
        updateToneFilter (smoothTone.getTargetValue(), sampleRate_);
        lastToneValue = smoothTone.getTargetValue();
    }

    // ── Input metering tap ─────────────────────────────────────────────────────
    float inputPeak = 0.0f;
    for (int ch = 0; ch < numCh; ++ch)
        inputPeak = std::max (inputPeak, buffer.getMagnitude (ch, 0, numSamples));

    const float meterAtkFactor = kMeterAttack;
    const float meterRelFactor = kMeterRelease;
    meterInSmoothed = (inputPeak > meterInSmoothed)
        ? meterInSmoothed + (inputPeak - meterInSmoothed) * meterAtkFactor
        : meterInSmoothed + (inputPeak - meterInSmoothed) * meterRelFactor;
    meterInput.store (meterInSmoothed);

    // ── Dry buffer write (latency-compensated) ─────────────────────────────────
    {
        const int bufSize = dryBuffer.getNumSamples();
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float* src = buffer.getReadPointer (ch);
            float*       dst = dryBuffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                dst[(dryWritePos + i) % bufSize] = src[i];
        }
        dryWritePos = (dryWritePos + numSamples) % bufSize;
    }

    // ── Per-sample processing ──────────────────────────────────────────────────
    for (int i = 0; i < numSamples; ++i)
    {
        const float driveSample  = smoothDrive.getNextValue();
        const float colourSample = smoothColour.getNextValue();
        // (tone, attack, ceiling, mix, output consumed below)

        // Map drive 0–1 → linear pre-gain
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
                        // Tape LPF (1-pole per channel)
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

            // ── Colour blend ───────────────────────────────────────────────────
            const float coloured = colourFilters[targetChar].processSample (ch, satOut);
            x = satOut + colourSample * (coloured - satOut);

            // ── Tone ───────────────────────────────────────────────────────────
            x = toneFilter[ch].processSample (x);

            buffer.setSample (ch, i, x);
        }

        // Advance crossfade (channel-independent)
        if (crossfadePos < 1.0f)
            crossfadePos = std::min (1.0f, crossfadePos + crossfadeInc);
    }

    // ── Brickwall Limiter ──────────────────────────────────────────────────────
    // Write to lookahead ring buffer, read back delayed version
    {
        const float ceilingLinear = juce::Decibels::decibelsToGain (
            smoothCeiling.getCurrentValue());

        const int bufSize = lookaheadBuf.getNumSamples();
        int   readPos  = (lookaheadWritePos - lookaheadSamples + bufSize) % bufSize;

        // 4x oversampled peak detection
        // We'll do detection on the non-oversampled signal for simplicity
        // (true-peak detection via oversampler happens on the write path)
        juce::dsp::AudioBlock<float> inputBlock (buffer);
        auto& oversampledBlock = oversampler.processSamplesUp (inputBlock);

        const int osNumSamples = static_cast<int> (oversampledBlock.getNumSamples());
        float peakOS = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            for (int i = 0; i < osNumSamples; ++i)
                peakOS = std::max (peakOS, std::abs (oversampledBlock.getSample (ch, i)));

        juce::dsp::AudioBlock<float> outputBlock (buffer);
        oversampler.processSamplesDown (outputBlock);

        // Write current samples to lookahead buffer
        for (int ch = 0; ch < numCh; ++ch)
        {
            float* dst = lookaheadBuf.getWritePointer (ch);
            const float* src = buffer.getReadPointer (ch);
            for (int i = 0; i < numSamples; ++i)
                dst[(lookaheadWritePos + i) % bufSize] = src[i];
        }

        // Read delayed samples and apply gain reduction
        for (int i = 0; i < numSamples; ++i)
        {
            const int rPos = (readPos + i) % bufSize;

            // Gain reduction: peak from this-plus-lookahead position
            // Use block-level peak from oversampler for true-peak ceiling
            const float targetGR = (peakOS > ceilingLinear && peakOS > 1e-6f)
                                   ? (ceilingLinear / peakOS)
                                   : 1.0f;

            // Smooth with attack/release
            if (targetGR < grEnvelope)
                grEnvelope = grEnvelope * attackCoeff + targetGR * (1.0f - attackCoeff);
            else
                grEnvelope = grEnvelope * releaseCoeff + targetGR * (1.0f - releaseCoeff);

            grEnvelope = juce::jlimit (0.0f, 1.0f, grEnvelope);

            for (int ch = 0; ch < numCh; ++ch)
                buffer.setSample (ch, i, lookaheadBuf.getSample (ch, rPos) * grEnvelope);
        }

        lookaheadWritePos = (lookaheadWritePos + numSamples) % bufSize;

        // GR meter smoothing
        const float grReduction = 1.0f - grEnvelope;
        grMeterSmoothed = (grReduction > grMeterSmoothed)
            ? grMeterSmoothed + (grReduction - grMeterSmoothed) * kMeterAttack
            : grMeterSmoothed + (grReduction - grMeterSmoothed) * kMeterRelease;
        meterGR.store (grMeterSmoothed);
    }

    // ── Output gain ────────────────────────────────────────────────────────────
    for (int i = 0; i < numSamples; ++i)
    {
        const float outGain = smoothOutputGain.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
            buffer.setSample (ch, i, buffer.getSample (ch, i) * outGain);
    }

    // ── Mix blend (dry/wet) ────────────────────────────────────────────────────
    // Read dry buffer (latency-compensated)
    {
        const int bufSize = dryBuffer.getNumSamples();
        const int dryReadBase = (dryWritePos - lookaheadSamples - numSamples + bufSize * 2) % bufSize;

        for (int i = 0; i < numSamples; ++i)
        {
            const float wet = smoothMix.getNextValue();
            const float dry = 1.0f - wet;
            const int   rPos = (dryReadBase + i) % bufSize;

            for (int ch = 0; ch < numCh; ++ch)
            {
                const float dryS = dryBuffer.getSample (ch, rPos);
                const float wetS = buffer.getSample (ch, i);
                buffer.setSample (ch, i, dry * dryS + wet * wetS);
            }
        }
    }

    // ── Output metering tap ────────────────────────────────────────────────────
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

int ColourBrickwallScreamAudioProcessor::getNumPrograms()   { return 1; }
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
