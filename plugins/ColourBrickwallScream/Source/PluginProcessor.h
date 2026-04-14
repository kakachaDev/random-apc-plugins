#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>

#include "ParameterIDs.hpp"

//==============================================================================
/**
 * ColourBrickwallScream — Harmonic Saturator & Brickwall Limiter
 *
 * Signal chain:
 *   DryBuffer tap → DriveGain → Saturation (Temper) → Colour → Tone →
 *   Brickwall Limiter → OutputGain → MixBlend → Output
 */
class ColourBrickwallScreamAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    ColourBrickwallScreamAudioProcessor();
    ~ColourBrickwallScreamAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Public APVTS
    juce::AudioProcessorValueTreeState apvts;

    //==============================================================================
    // Thread-safe metering (read from GUI thread at ~30fps)
    std::atomic<float> meterInput  { 0.0f };   // 0.0–1.0 linear
    std::atomic<float> meterGR     { 0.0f };   // 0.0–1.0 (0=no reduction, 1=full)
    std::atomic<float> meterOutput { 0.0f };   // 0.0–1.0 linear

    //==============================================================================
    // Latency (lookahead)
    int getLatencySamples() const { return lookaheadSamples; }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // ── DSP STATE ────────────────────────────────────────────────────────────────

    double sampleRate_  = 44100.0;
    int    blockSize_   = 512;
    int    numChannels_ = 2;

    // Lookahead (2ms, scaled to sample rate)
    int lookaheadSamples = 0;

    // ── DRIVE ─────────────────────────────────────────────────────────────────────
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothDrive;

    // ── SATURATION (Temper) ───────────────────────────────────────────────────────
    // Per-sample transfer function selection
    int   currentChar  = 0;
    int   targetChar   = 0;
    float crossfadePos = 1.0f;   // 1.0 = fully on targetChar
    float crossfadeInc = 0.0f;   // per-sample increment for 4ms fade

    // Inline transfer functions
    static float saturateTube       (float x, float drive) noexcept;
    static float saturateTape       (float x, float drive) noexcept;
    static float saturateTransformer(float x, float drive) noexcept;
    static float saturateDiode      (float x, float drive) noexcept;
    static float saturateBitcrush   (float x, float drive) noexcept;
    static float saturateFullScream (float x, float drive) noexcept;

    // Tape: post-saturation LPF per channel (1 pole, 14kHz)
    std::array<float, 2> tapeLPFState { 0.0f, 0.0f };
    float tapeLPFCoeff = 0.0f;   // updated in prepareToPlay

    // Bitcrush: bit-depth mapped from drive
    static float getBitDepthFromDrive (float normDrive) noexcept;

    // ── COLOUR ────────────────────────────────────────────────────────────────────
    // Per-variant biquad (one filter pair per channel, 2 channels max)
    using BiquadCoeffs = juce::dsp::IIR::Coefficients<float>;

    struct ColourFilter
    {
        juce::dsp::IIR::Filter<float> filter[2];
        void prepare (const juce::dsp::ProcessSpec& spec);
        void reset();
        float processSample (int ch, float x);
        void setCoeffs (BiquadCoeffs::Ptr c);
    };

    std::array<ColourFilter, 6> colourFilters;   // one per Temper variant
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothColour;

    void updateColourFilter (int variant, double sr);

    // ── TONE (tilt EQ) ────────────────────────────────────────────────────────────
    juce::dsp::IIR::Filter<float> toneFilter[2];  // per channel
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothTone;
    float lastToneValue = 2.0f;  // sentinel to force first recalculation
    void updateToneFilter (float toneValue, double sr);

    // ── BRICKWALL LIMITER ──────────────────────────────────────────────────────────
    // Lookahead ring buffer (per channel)
    juce::AudioBuffer<float> lookaheadBuf;
    int lookaheadWritePos = 0;

    // Gain reduction envelope (single value, applied to all channels)
    float grEnvelope = 1.0f;    // 1.0 = no reduction
    float attackCoeff  = 0.0f;  // per-sample, updated from attack_character
    float releaseCoeff = 0.0f;  // fixed 200ms

    // 4x oversampling for true-peak detection
    juce::dsp::Oversampling<float> oversampler;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothAttack;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothCeiling;

    void updateLimiterCoeffs (float attackNorm, float ceilingDb, double sr);

    // GR meter (smoothed, for atomic write)
    float grMeterSmoothed = 0.0f;

    // ── DRY BUFFER & MIX ──────────────────────────────────────────────────────────
    juce::AudioBuffer<float> dryBuffer;   // latency-compensated dry signal
    int dryWritePos = 0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothOutputGain;

    // ── METERING ─────────────────────────────────────────────────────────────────
    float meterInSmoothed  = 0.0f;
    float meterOutSmoothed = 0.0f;
    static constexpr float kMeterAttack  = 0.70f;
    static constexpr float kMeterRelease = 0.09f;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColourBrickwallScreamAudioProcessor)
};
