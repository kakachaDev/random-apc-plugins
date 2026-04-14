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
 *   Brickwall Limiter → MixBlend → Output
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

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // ── DSP STATE ────────────────────────────────────────────────────────────────

    double sampleRate_  = 44100.0;
    int    blockSize_   = 512;
    int    numChannels_ = 2;

    // ── DRIVE ─────────────────────────────────────────────────────────────────────
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothDrive;

    // ── SATURATION (Temper) ───────────────────────────────────────────────────────
    int   currentChar  = 0;
    int   targetChar   = 0;
    float crossfadePos = 1.0f;
    float crossfadeInc = 0.0f;

    static float saturateTube       (float x, float drive) noexcept;
    static float saturateTape       (float x, float drive) noexcept;
    static float saturateTransformer(float x, float drive) noexcept;
    static float saturateDiode      (float x, float drive) noexcept;
    static float saturateBitcrush   (float x, float drive) noexcept;
    static float saturateFullScream (float x, float drive) noexcept;

    // Tape: post-saturation LPF per channel (1-pole, ~14kHz)
    std::array<float, 2> tapeLPFState { 0.0f, 0.0f };
    float tapeLPFCoeff = 0.0f;

    static float getBitDepthFromDrive (float normDrive) noexcept;

    // ── COLOUR ────────────────────────────────────────────────────────────────────
    using BiquadCoeffs = juce::dsp::IIR::Coefficients<float>;

    struct ColourFilter
    {
        juce::dsp::IIR::Filter<float> filter[2];
        void prepare (const juce::dsp::ProcessSpec& spec);
        void reset();
        float processSample (int ch, float x);
        void setCoeffs (BiquadCoeffs::Ptr c);
    };

    std::array<ColourFilter, 6> colourFilters;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothColour;
    void updateColourFilter (int variant, double sr);

    // ── TONE (dual-shelf tilt EQ) ─────────────────────────────────────────────────
    juce::dsp::IIR::Filter<float> toneFilterHigh[2];  // high shelf per channel
    juce::dsp::IIR::Filter<float> toneFilterLow[2];   // low shelf per channel
    float lastToneValue = 2.0f;   // sentinel to force first recalculation
    void updateToneFilter (float toneValue, double sr);

    // ── BRICKWALL LIMITER ──────────────────────────────────────────────────────────
    float grEnvelope   = 1.0f;
    float attackCoeff  = 0.0f;   // fixed 0.5ms, set in prepareToPlay
    float releaseCoeff = 0.0f;   // fixed 200ms

    // GR meter (smoothed, for atomic write)
    float grMeterSmoothed = 0.0f;

    // ── DRY BUFFER & MIX ──────────────────────────────────────────────────────────
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMix;

    // ── METERING ─────────────────────────────────────────────────────────────────
    float meterInSmoothed  = 0.0f;
    float meterOutSmoothed = 0.0f;
    static constexpr float kMeterAttack  = 0.70f;
    static constexpr float kMeterRelease = 0.09f;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColourBrickwallScreamAudioProcessor)
};
