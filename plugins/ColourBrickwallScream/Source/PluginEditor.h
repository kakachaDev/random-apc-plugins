#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"

//==============================================================================
// Colour palette
//==============================================================================
namespace CBS
{
    static const juce::Colour BG      { 0xFF0E1B26 };
    static const juce::Colour PANEL   { 0xFF122030 };
    static const juce::Colour RAISED  { 0xFF1A2D3C };
    static const juce::Colour ACCENT  { 0xFF3EB8E0 };
    static const juce::Colour ACCENT2 { 0xFF65CBF0 };
    static const juce::Colour ACCD    { 0xFF2490B8 };
    static const juce::Colour TEXT    { 0xFFC8E4EF };
    static const juce::Colour DIM     { 0xFF4A7A90 };
    static const juce::Colour LABEL   { 0xFF6699AA };
    static const juce::Colour BORDER  { 0xFF223344 };
    static const juce::Colour SEP     { 0xFF1C2E3C };
    static const juce::Colour TRACK   { 0xFF1E3040 };
}

//==============================================================================
// Custom LookAndFeel
//==============================================================================
class CBSLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CBSLookAndFeel()
    {
        setColour (juce::Slider::rotarySliderFillColourId,    CBS::ACCENT);
        setColour (juce::Slider::rotarySliderOutlineColourId, CBS::TRACK);
        setColour (juce::Slider::thumbColourId,               CBS::ACCENT2);
        setColour (juce::TextButton::buttonColourId,          CBS::RAISED);
        setColour (juce::TextButton::buttonOnColourId,        CBS::ACCENT.withAlpha (0.18f));
        setColour (juce::TextButton::textColourOffId,         CBS::DIM);
        setColour (juce::TextButton::textColourOnId,          CBS::ACCENT);
        setColour (juce::Label::textColourId,                 CBS::LABEL);
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& b,
                               const juce::Colour&,
                               bool isMouseOver, bool isButtonDown) override;

    void drawButtonText (juce::Graphics& g, juce::TextButton& b,
                         bool isMouseOver, bool isButtonDown) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return juce::Font (static_cast<float> (buttonHeight) * 0.42f,
                           juce::Font::plain);
    }

    juce::Font getLabelFont (juce::Label& l) override
    {
        return juce::Font (l.getFont().getHeight(), juce::Font::plain);
    }
};

//==============================================================================
// Three-channel level meter component
//==============================================================================
class MeterBar : public juce::Component
{
public:
    void setLevel (float lin) { level = juce::jlimit (0.0f, 1.0f, lin); }
    void setReversed (bool r) { reversed = r; }

    void paint (juce::Graphics& g) override
    {
        const int w = getWidth();
        const int h = getHeight();
        const int numSegs = w / 5;
        const int filled  = juce::roundToInt (level * numSegs);

        g.fillAll (CBS::BG.darker (0.3f));

        for (int i = 0; i < numSegs; ++i)
        {
            const float ratio = reversed ? (1.0f - (float)i / numSegs) : ((float)i / numSegs);
            const int   idx   = reversed ? (numSegs - 1 - i) : i;
            const bool  lit   = idx < filled;
            const int   px    = (reversed ? (w - (i + 1) * 5 + 1) : (i * 5 + 1));

            juce::Colour col;
            if      (ratio < 0.60f) col = lit ? juce::Colour (0xFF22CC22) : juce::Colour (0xFF1A5C1A);
            else if (ratio < 0.75f) col = lit ? juce::Colour (0xFF88CC00) : juce::Colour (0xFF2A5A10);
            else if (ratio < 0.85f) col = lit ? juce::Colour (0xFFFFAA00) : juce::Colour (0xFF7A5500);
            else if (ratio < 0.95f) col = lit ? juce::Colour (0xFFFF4400) : juce::Colour (0xFF8A2A00);
            else                    col = lit ? juce::Colour (0xFFFF1100) : juce::Colour (0xFF6A0000);

            g.setColour (col);
            g.fillRect (px, 1, 4, h - 2);
        }
    }

private:
    float level    = 0.0f;
    bool  reversed = false;
};

//==============================================================================
// Main editor
//==============================================================================
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
    CBSLookAndFeel laf;

    // ── Knobs ──────────────────────────────────────────────────────────────────
    juce::Slider driveKnob   { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider toneKnob    { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider colourKnob  { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider ceilingKnob { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider mixKnob     { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };

    // ── Value labels ───────────────────────────────────────────────────────────
    juce::Label driveVal, toneVal, colourVal, ceilingVal, mixVal;

    // ── Knob name labels ───────────────────────────────────────────────────────
    juce::Label driveNameLabel, toneNameLabel, colourNameLabel,
                ceilingNameLabel, mixNameLabel;

    // ── Character selector ─────────────────────────────────────────────────────
    static constexpr int NUM_CHARS = 6;
    static const char* CHAR_NAMES[NUM_CHARS];
    juce::TextButton charButtons[NUM_CHARS];

    // ── Meters ─────────────────────────────────────────────────────────────────
    MeterBar meterIn, meterGR, meterOut;
    juce::Label mLabelIn, mLabelGR, mLabelOut;
    juce::Label mValIn, mValGR, mValOut;

    // ── Parameter attachments ──────────────────────────────────────────────────
    std::unique_ptr<juce::SliderParameterAttachment> driveAtt;
    std::unique_ptr<juce::SliderParameterAttachment> toneAtt;
    std::unique_ptr<juce::SliderParameterAttachment> colourAtt;
    std::unique_ptr<juce::SliderParameterAttachment> ceilingAtt;
    std::unique_ptr<juce::SliderParameterAttachment> mixAtt;

    // ── Helpers ────────────────────────────────────────────────────────────────
    void setupKnob   (juce::Slider& s, juce::Label& nameLabel, const char* name,
                      juce::Label& valLabel);
    void setCharacter (int idx);
    void updateCharButtons();

    // Value formatters
    static juce::String formatDrive   (float norm);
    static juce::String formatTone    (float v);
    static juce::String formatColour  (float norm);
    static juce::String formatCeiling (float v);
    static juce::String formatMix     (float norm);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColourBrickwallScreamAudioProcessorEditor)
};
