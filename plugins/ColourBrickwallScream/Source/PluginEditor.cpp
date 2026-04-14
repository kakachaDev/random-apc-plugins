#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.hpp"

#include <unordered_map>

//==============================================================================
ColourBrickwallScreamAudioProcessorEditor::ColourBrickwallScreamAudioProcessorEditor
    (ColourBrickwallScreamAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (700, 500);

    // ── Create WebBrowserComponent ─────────────────────────────────────────────
    // CRITICAL: Relays must already exist (they are member variables).
    // Pass them via .withOptionsFrom() BEFORE constructing the component.
    webView = std::make_unique<juce::WebBrowserComponent> (
        juce::WebBrowserComponent::Options()
#if JUCE_WINDOWS
            .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options (
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder (juce::File::getSpecialLocation (
                        juce::File::SpecialLocationType::tempDirectory)))
#endif
            .withNativeIntegrationEnabled()
            .withOptionsFrom (driveRelay)
            .withOptionsFrom (characterRelay)
            .withOptionsFrom (colourRelay)
            .withOptionsFrom (toneRelay)
            .withOptionsFrom (attackCharacterRelay)
            .withOptionsFrom (ceilingRelay)
            .withOptionsFrom (mixRelay)
            .withOptionsFrom (outputGainRelay)
            .withResourceProvider ([this] (const auto& url) {
                return getResource (url);
            })
    );

    addAndMakeVisible (*webView);

    // ── Create parameter attachments AFTER webView ────────────────────────────
    auto& apvts = audioProcessor.apvts;
    driveAttachment           = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter (ParameterIDs::DRIVE),            driveRelay,           nullptr);
    characterAttachment       = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter (ParameterIDs::CHARACTER),        characterRelay,       nullptr);
    colourAttachment          = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter (ParameterIDs::COLOUR),           colourRelay,          nullptr);
    toneAttachment            = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter (ParameterIDs::TONE),             toneRelay,            nullptr);
    attackCharacterAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter (ParameterIDs::ATTACK_CHARACTER), attackCharacterRelay, nullptr);
    ceilingAttachment         = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter (ParameterIDs::CEILING),          ceilingRelay,         nullptr);
    mixAttachment             = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter (ParameterIDs::MIX),              mixRelay,             nullptr);
    outputGainAttachment      = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter (ParameterIDs::OUTPUT_GAIN),      outputGainRelay,      nullptr);

    // ── Load web content ───────────────────────────────────────────────────────
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // ── Meter polling timer: ~30fps ────────────────────────────────────────────
    startTimerHz (30);
}

ColourBrickwallScreamAudioProcessorEditor::~ColourBrickwallScreamAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void ColourBrickwallScreamAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF0D0A09));
}

void ColourBrickwallScreamAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}

//==============================================================================
// TIMER: Push meter values to JS
//==============================================================================
void ColourBrickwallScreamAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    const float mIn  = audioProcessor.meterInput.load();
    const float mGR  = audioProcessor.meterGR.load();
    const float mOut = audioProcessor.meterOutput.load();

    // Send as a JSON message to the WebView JS side
    juce::String msg;
    msg  = "{\"type\":\"meterUpdate\",\"input\":";
    msg += juce::String (mIn, 4);
    msg += ",\"gr\":";
    msg += juce::String (mGR, 4);
    msg += ",\"output\":";
    msg += juce::String (mOut, 4);
    msg += "}";

    webView->evaluateJavascript (
        "if(window.__juceMeterUpdate){window.__juceMeterUpdate(" + msg + ");}",
        [] (juce::WebBrowserComponent::EvaluationResult) {});
}

//==============================================================================
// RESOURCE PROVIDER
//==============================================================================
juce::ZipFile* ColourBrickwallScreamAudioProcessorEditor::getZipFile()
{
    static auto stream = juce::createAssetInputStream (
        "colourbrickwallscream_webui.zip", juce::AssertAssetExists::no);

    if (stream == nullptr)
        return nullptr;

    static juce::ZipFile f { *stream };
    return &f;
}

const char* ColourBrickwallScreamAudioProcessorEditor::getMimeForExtension (const juce::String& extension)
{
    static const std::unordered_map<juce::String, const char*> mimeMap =
    {
        { "htm",   "text/html"                },
        { "html",  "text/html"                },
        { "txt",   "text/plain"               },
        { "js",    "text/javascript"          },
        { "json",  "application/json"         },
        { "css",   "text/css"                 },
        { "png",   "image/png"                },
        { "svg",   "image/svg+xml"            },
        { "ico",   "image/vnd.microsoft.icon" },
        { "woff2", "font/woff2"               },
        { "map",   "application/json"         },
    };

    if (const auto it = mimeMap.find (extension.toLowerCase()); it != mimeMap.end())
        return it->second;

    jassertfalse;
    return "text/plain";
}

juce::String ColourBrickwallScreamAudioProcessorEditor::getExtension (juce::String filename)
{
    return filename.fromLastOccurrenceOf (".", false, false);
}

auto ColourBrickwallScreamAudioProcessorEditor::streamToVector (juce::InputStream& stream)
{
    std::vector<std::byte> result (static_cast<size_t> (stream.getTotalLength()));
    stream.setPosition (0);
    [[maybe_unused]] const auto bytesRead = stream.read (result.data(), result.size());
    jassert (bytesRead == static_cast<ssize_t> (result.size()));
    return result;
}

std::optional<juce::WebBrowserComponent::Resource>
ColourBrickwallScreamAudioProcessorEditor::getResource (const juce::String& url)
{
    const auto urlToRetrieve = (url == "/" || url.isEmpty())
        ? juce::String { "index.html" }
        : url.fromFirstOccurrenceOf ("/", false, false);

    if (auto* archive = getZipFile())
    {
        if (auto* entry = archive->getEntry (urlToRetrieve))
        {
            auto stream = juce::rawToUniquePtr (archive->createStreamForEntry (*entry));
            auto data   = streamToVector (*stream);
            auto mime   = getMimeForExtension (getExtension (entry->filename).toLowerCase());
            return juce::WebBrowserComponent::Resource { std::move (data), juce::String { mime } };
        }
    }

    return std::nullopt;
}
