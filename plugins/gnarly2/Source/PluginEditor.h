#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

//==============================================================================
class Gnarly2AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit Gnarly2AudioProcessorEditor (Gnarly2AudioProcessor&);
    ~Gnarly2AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Gnarly2AudioProcessor& processorRef;

    // ═══════════════════════════════════════════════════════════════
    // CRITICAL: Member Declaration Order (prevents DAW crashes)
    // Destruction happens in REVERSE order of declaration
    // Order: Relays → WebView → Attachments
    // ═══════════════════════════════════════════════════════════════

    // 1. RELAYS FIRST (destroyed last - no dependencies)
    std::unique_ptr<juce::WebSliderRelay> driveRelay;
    std::unique_ptr<juce::WebSliderRelay> cutoffRelay;
    std::unique_ptr<juce::WebSliderRelay> resonanceRelay;

    // 2. WEBVIEW SECOND (destroyed middle - depends on relays via withOptionsFrom)
    struct SinglePageBrowser : juce::WebBrowserComponent
    {
        using WebBrowserComponent::WebBrowserComponent;
        bool pageAboutToLoad (const juce::String& newURL) override
        {
            return newURL == getResourceProviderRoot();
        }
    };
    std::unique_ptr<SinglePageBrowser> webView;

    // 3. ATTACHMENTS LAST (destroyed first - depend on both relays and parameters)
    std::unique_ptr<juce::WebSliderParameterAttachment> driveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> cutoffAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> resonanceAttachment;

    //==============================================================================
    // Resource Provider
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    static juce::WebBrowserComponent::Options createWebOptions (Gnarly2AudioProcessorEditor& editor);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Gnarly2AudioProcessorEditor)
};
