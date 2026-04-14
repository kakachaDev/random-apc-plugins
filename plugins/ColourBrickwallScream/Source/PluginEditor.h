#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

//==============================================================================
/**
 * ColourBrickwallScream Plugin Editor — WebView UI
 *
 * CRITICAL: Member declaration order (prevents DAW crash on unload):
 *   1. Parameter relays    (destroyed LAST  — no dependencies)
 *   2. WebBrowserComponent (destroyed MIDDLE — references relays)
 *   3. Parameter attachments (destroyed FIRST — references both)
 */
class ColourBrickwallScreamAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      public juce::Timer
{
public:
    ColourBrickwallScreamAudioProcessorEditor (ColourBrickwallScreamAudioProcessor&);
    ~ColourBrickwallScreamAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;
    void timerCallback() override;

private:
    ColourBrickwallScreamAudioProcessor& audioProcessor;

    // ═══════════════════════════════════════════════════════════════════════════
    // 1. RELAYS FIRST (destroyed last — no dependencies)
    // ═══════════════════════════════════════════════════════════════════════════
    juce::WebSliderRelay driveRelay     { ParameterIDs::DRIVE     };
    juce::WebSliderRelay characterRelay { ParameterIDs::CHARACTER };
    juce::WebSliderRelay colourRelay    { ParameterIDs::COLOUR    };
    juce::WebSliderRelay toneRelay      { ParameterIDs::TONE      };
    juce::WebSliderRelay ceilingRelay   { ParameterIDs::CEILING   };
    juce::WebSliderRelay mixRelay       { ParameterIDs::MIX       };

    // ═══════════════════════════════════════════════════════════════════════════
    // 2. WEBVIEW SECOND (destroyed middle — references relays)
    // ═══════════════════════════════════════════════════════════════════════════
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // ═══════════════════════════════════════════════════════════════════════════
    // 3. ATTACHMENTS LAST (destroyed first — references relays + parameters)
    // ═══════════════════════════════════════════════════════════════════════════
    std::unique_ptr<juce::WebSliderParameterAttachment> driveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> characterAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> colourAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> toneAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> ceilingAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;

    // ═══════════════════════════════════════════════════════════════════════════
    // Resource provider
    // ═══════════════════════════════════════════════════════════════════════════
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    static const char*  getMimeForExtension (const juce::String& extension);
    static juce::String getExtension (juce::String filename);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColourBrickwallScreamAudioProcessorEditor)
};
