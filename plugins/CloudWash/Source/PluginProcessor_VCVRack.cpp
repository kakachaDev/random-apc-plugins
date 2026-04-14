// DEPRECATED: VCV Rack-style implementation reference
//
// This file is kept for reference purposes only. It represents an alternative
// implementation approach that processes audio sample-by-sample like the original
// VCV Rack module, rather than block-based processing.
//
// The active implementation is in PluginProcessor.cpp which uses block-based
// processing with JUCE's LagrangeInterpolator for better performance.
//
// Source: https://github.com/VCVRack/AudibleInstruments/blob/v2/src/Clouds.cpp
// Original hardware: Mutable Instruments Clouds
//
// NOTE: This file is NOT compiled into the plugin. See CMakeLists.txt which
// only includes PluginProcessor.cpp in the target sources.

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CloudWashAudioProcessor::CloudWashAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#else
    :
#endif
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    // VCV Rack memory allocation: 118784 bytes main, 65536-128 bytes CCM
    const int memLen = 118784;
    const int ccmLen = 65536 - 128;

    block_mem.resize(memLen, 0);
    block_ccm.resize(ccmLen, 0);

    // Zero initialize processor (VCV Rack does this with memset)
    processor.Init(block_mem.data(), memLen, block_ccm.data(), ccmLen);

    // Initialize state
    freeze = false;
    playback = clouds::PLAYBACK_MODE_GRANULAR;
    quality = 0;

    // Initialize presets
    initializePresets();
}

CloudWashAudioProcessor::~CloudWashAudioProcessor()
{
}

//==============================================================================
const juce::String CloudWashAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CloudWashAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool CloudWashAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool CloudWashAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double CloudWashAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CloudWashAudioProcessor::getNumPrograms()
{
    return (int)presets.size();
}

int CloudWashAudioProcessor::getCurrentProgram()
{
    return currentPresetIndex;
}

void CloudWashAudioProcessor::setCurrentProgram (int index)
{
    if (index >= 0 && index < (int)presets.size())
    {
        currentPresetIndex = index;
        loadPreset(index);
    }
}

const juce::String CloudWashAudioProcessor::getProgramName (int index)
{
    if (index >= 0 && index < (int)presets.size())
        return presets[index].name;
    return "Invalid";
}

void CloudWashAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    if (index >= 0 && index < (int)presets.size())
        presets[index].name = newName;
}

//==============================================================================
void CloudWashAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hostSampleRate = sampleRate;

    // Initialize sample rate converters (VCV Rack style)
    for (int i = 0; i < 2; ++i) {
        inputResamplers[i].reset();
        outputResamplers[i].reset();
    }

    // Allocate buffers
    inputBuffer.setSize(2, samplesPerBlock * 4);
    outputBuffer.setSize(2, samplesPerBlock * 4);

    // Allocate frame buffers for Clouds (32 frames at a time, VCV Rack style)
    inputFrames.resize(32);
    outputFrames.resize(32);
}

void CloudWashAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CloudWashAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
   #endif

    return true;
  #endif
}
#endif

//==============================================================================
// VCV RACK PROCESSING ALGORITHM - EXACT PORT
//==============================================================================
void CloudWashAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    //==============================================================================
    // VCV RACK ALGORITHM: Process in 32-sample chunks at 32kHz
    //==============================================================================

    int numSamples = buffer.getNumSamples();

    // Get parameters (VCV Rack reads these fresh every cycle)
    float position = apvts.getRawParameterValue("position")->load();
    float size = apvts.getRawParameterValue("size")->load();
    float pitch = apvts.getRawParameterValue("pitch")->load();
    float in_gain = apvts.getRawParameterValue("in_gain")->load();
    float density = apvts.getRawParameterValue("density")->load();
    float texture = apvts.getRawParameterValue("texture")->load();
    float blend = apvts.getRawParameterValue("blend")->load();
    float spread = apvts.getRawParameterValue("spread")->load();
    float feedback_val = apvts.getRawParameterValue("feedback")->load();
    float reverb = apvts.getRawParameterValue("reverb")->load();

    auto freezeParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("freeze"));
    auto modeParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("mode"));
    auto qualityParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("quality"));

    if (freezeParam) {
        freeze = freezeParam->get();
    }

    if (modeParam) {
        playback = static_cast<clouds::PlaybackMode>(modeParam->getIndex());
    }

    if (qualityParam) {
        quality = qualityParam->getIndex();
    }

    // Process sample by sample (VCV Rack processes per sample)
    for (int i = 0; i < numSamples; ++i)
    {
        // Get input frame (VCV Rack: input with gain, /5.0 for voltage scaling)
        float inputL = buffer.getSample(0, i) * in_gain;
        float inputR = (totalNumInputChannels > 1) ? buffer.getSample(1, i) * in_gain : inputL;

        // Add to input buffer (VCV Rack uses ring buffer)
        if (inputBufferWritePos < inputBuffer.getNumSamples()) {
            inputBuffer.setSample(0, inputBufferWritePos, inputL);
            inputBuffer.setSample(1, inputBufferWritePos, inputR);
            inputBufferWritePos++;
        }

        // Check if we need to render frames (VCV Rack: when output buffer empty)
        if (outputBufferReadPos >= outputBufferSize) {
            // Convert input buffer to 32kHz (VCV Rack uses SRC)
            double ratioDown = hostSampleRate / 32000.0;
            int num32kSamples = 32;  // VCV Rack always processes 32 frames

            // Resample input
            for (int ch = 0; ch < 2; ++ch) {
                inputResamplers[ch].process(
                    ratioDown,
                    inputBuffer.getReadPointer(ch),
                    resampledInput[ch],
                    num32kSamples
                );
            }

            // Convert to ShortFrame (VCV Rack format)
            for (int j = 0; j < 32; ++j) {
                inputFrames[j].l = juce::jlimit(-32768, 32767,
                    (int)(resampledInput[0][j] * 32767.0f));
                inputFrames[j].r = juce::jlimit(-32768, 32767,
                    (int)(resampledInput[1][j] * 32767.0f));
            }

            // VCV RACK CRITICAL: Set up processor before EVERY Process() call
            processor.set_playback_mode(playback);
            processor.set_quality(quality);
            processor.Prepare();  // VCV Rack calls this every time!

            // Set parameters (VCV Rack clamps everything)
            clouds::Parameters* p = processor.mutable_parameters();
            p->trigger = false;  // No trigger in plugin version
            p->gate = false;
            p->freeze = freeze;
            p->position = juce::jlimit(0.0f, 1.0f, position);
            p->size = juce::jlimit(0.0f, 1.0f, size);
            p->pitch = juce::jlimit(-48.0f, 48.0f, pitch * 12.0f);  // -2 to +2 octaves = -24 to +24 semitones
            p->density = juce::jlimit(0.0f, 1.0f, density);
            p->texture = juce::jlimit(0.0f, 1.0f, texture);
            p->dry_wet = juce::jlimit(0.0f, 1.0f, blend);
            p->stereo_spread = juce::jlimit(0.0f, 1.0f, spread);
            p->feedback = juce::jlimit(0.0f, 1.0f, feedback_val);
            p->reverb = juce::jlimit(0.0f, 1.0f, reverb);

            // Process (VCV Rack: exactly 32 frames)
            processor.Process(inputFrames.data(), outputFrames.data(), 32);

            // Convert output to float
            for (int j = 0; j < 32; ++j) {
                resampledOutput[0][j] = outputFrames[j].l / 32768.0f;
                resampledOutput[1][j] = outputFrames[j].r / 32768.0f;
            }

            // Resample output back to host rate
            double ratioUp = 32000.0 / hostSampleRate;
            for (int ch = 0; ch < 2; ++ch) {
                outputResamplers[ch].process(
                    ratioUp,
                    resampledOutput[ch],
                    outputBuffer.getWritePointer(ch),
                    numSamples
                );
            }

            outputBufferSize = numSamples;
            outputBufferReadPos = 0;
            inputBufferWritePos = 0;
        }

        // Write output (VCV Rack: shift from output buffer)
        if (outputBufferReadPos < outputBufferSize) {
            buffer.setSample(0, i, outputBuffer.getSample(0, outputBufferReadPos));
            if (totalNumOutputChannels > 1) {
                buffer.setSample(1, i, outputBuffer.getSample(1, outputBufferReadPos));
            }
            outputBufferReadPos++;
        }
    }

    // METERING: Update meter values
    float inputPeak = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    float outputPeak = buffer.getRMSLevel(0, 0, buffer.getNumSamples());

    if (inputPeak > inputPeakHold)
        inputPeakHold = inputPeak;
    else
        inputPeakHold *= 0.97f;

    if (outputPeak > outputPeakHold)
        outputPeakHold = outputPeak;
    else
        outputPeakHold *= 0.97f;

    inputPeakLevel.store(inputPeakHold);
    outputPeakLevel.store(outputPeakHold);
}

juce::String CloudWashAudioProcessor::getQualityModeName(int index)
{
    // VCV Rack quality labels
    switch (index) {
        case 0:  return "1s 32kHz 16-bit stereo";
        case 1:  return "2s 32kHz 16-bit mono";
        case 2:  return "4s 16kHz 8-bit mu-law stereo";
        case 3:  return "8s 16kHz 8-bit mu-law mono";
        default: return "Unknown";
    }
}

//==============================================================================
juce::AudioProcessorEditor* CloudWashAudioProcessor::createEditor()
{
    return new CloudWashAudioProcessorEditor (*this);
}

bool CloudWashAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================
void CloudWashAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CloudWashAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout CloudWashAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // VCV Rack parameter ranges (exact match)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "position", "Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "size", "Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "pitch", "Pitch",
        juce::NormalisableRange<float>(-2.0f, 2.0f, 0.01f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "in_gain", "In Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "density", "Density",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "texture", "Texture",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "blend", "Blend",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "spread", "Stereo Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "feedback", "Feedback",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "reverb", "Reverb",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "mode", "Mode",
        juce::StringArray{"Granular", "Pitch-shifter", "Looping delay", "Spectral"}, 0));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "freeze", "Freeze", false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "quality", "Quality",
        juce::StringArray{
            getQualityModeName(0),
            getQualityModeName(1),
            getQualityModeName(2),
            getQualityModeName(3)
        }, 0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "sample_mode", "Sample Mode",
        juce::StringArray{"Normal", "Reverse"}, 0));

    return layout;
}

//==============================================================================
// PRESET MANAGEMENT
//==============================================================================

void CloudWashAudioProcessor::initializePresets()
{
    presets.clear();

    // Preset 1: Init (Default)
    presets.push_back({"01 - Init", {
        {"position", 0.5f}, {"size", 0.5f}, {"pitch", 0.0f}, {"density", 0.5f}, {"texture", 0.5f},
        {"in_gain", 0.5f}, {"blend", 0.5f}, {"spread", 0.0f}, {"feedback", 0.0f}, {"reverb", 0.0f},
        {"mode", 0.0f}, {"quality", 0.0f}, {"freeze", 0.0f}, {"sample_mode", 0.0f}
    }});

    // Add more presets...
    currentPresetIndex = 0;
}

void CloudWashAudioProcessor::loadPreset(int index)
{
    if (index < 0 || index >= (int)presets.size())
        return;

    const auto& preset = presets[index];

    for (const auto& [paramName, value] : preset.parameters)
    {
        auto* param = apvts.getParameter(paramName);
        if (param)
        {
            auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param);
            if (choiceParam)
            {
                int numChoices = choiceParam->choices.size();
                int targetIndex = juce::jlimit(0, numChoices - 1, (int)(value * numChoices + 0.5f));
                choiceParam->setValueNotifyingHost(choiceParam->convertTo0to1(targetIndex));
            }
            else
            {
                param->setValueNotifyingHost(value);
            }
        }
    }

    currentPresetIndex = index;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CloudWashAudioProcessor();
}
