#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NfGnarlyAudioProcessor::NfGnarlyAudioProcessor()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier ("NfGnarly"), createParameterLayout())
{
}

NfGnarlyAudioProcessor::~NfGnarlyAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout NfGnarlyAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Drive parameter: -24dB to +24dB, linear
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 },
        "Drive",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 1) + " dB"; }
    ));

    // Cutoff parameter: 20Hz to 20kHz, logarithmic (0.25 skew = smoother feel)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cutoff", 1 },
        "Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f),  // 0.25 instead of 0.3 = smoother
        1000.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) {
            if (value >= 1000.0f)
                return juce::String (value / 1000.0f, 1) + " kHz";
            else
                return juce::String (value, 0) + " Hz";
        }
    ));

    // Resonance parameter: 0.0 to 1.0, linear
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "resonance", 1 },
        "Resonance",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 2); }
    ));

    return layout;
}

//==============================================================================
const juce::String NfGnarlyAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NfGnarlyAudioProcessor::acceptsMidi() const
{
    return false;
}

bool NfGnarlyAudioProcessor::producesMidi() const
{
    return false;
}

bool NfGnarlyAudioProcessor::isMidiEffect() const
{
    return false;
}

double NfGnarlyAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NfGnarlyAudioProcessor::getNumPrograms()
{
    return 1;
}

int NfGnarlyAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NfGnarlyAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String NfGnarlyAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void NfGnarlyAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void NfGnarlyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());

    driveGain.prepare (spec);
    filter.prepare (spec);

    // Set minimal ramping for smooth audio (no zipper noise)
    driveGain.setRampDurationSeconds (0.005);  // 5ms - very fast
}

void NfGnarlyAudioProcessor::releaseResources()
{
}

bool NfGnarlyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void NfGnarlyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear extra output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Handle zero-length buffers
    if (buffer.getNumSamples() == 0)
        return;

    // Get parameter values - DIRECT, NO SMOOTHING for instant response
    const float drive = parameters.getRawParameterValue ("drive")->load();
    const float cutoff = parameters.getRawParameterValue ("cutoff")->load();
    const float resonance = parameters.getRawParameterValue ("resonance")->load();

    // Apply drive gain directly (has internal ramping to prevent zipper noise)
    driveGain.setGainDecibels (drive);

    // Map resonance (0.0-1.0) to Q (0.7-50)
    const float q = 0.7f + resonance * 49.3f;

    // Update filter coefficients DIRECTLY for instant response
    *filter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (
        currentSampleRate,
        juce::jlimit (20.0f, 20000.0f, cutoff),
        juce::jlimit (0.1f, 50.0f, q)
    );

    // Process audio
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    driveGain.process (context);
    filter.process (context);
}

//==============================================================================
bool NfGnarlyAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* NfGnarlyAudioProcessor::createEditor()
{
    return new NfGnarlyAudioProcessorEditor (*this);
}

//==============================================================================
void NfGnarlyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void NfGnarlyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NfGnarlyAudioProcessor();
}
