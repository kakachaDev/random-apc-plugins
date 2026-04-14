#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

//==============================================================================
// Static data
//==============================================================================
const char* ColourBrickwallScreamAudioProcessorEditor::CHAR_NAMES[NUM_CHARS] =
    { "TUBE", "TAPE", "XFMR", "DIODE", "BITCR", "SCREAM" };

//==============================================================================
// LookAndFeel — rotary knob
//==============================================================================
void CBSLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                       int x, int y, int width, int height,
                                       float sliderPos,
                                       float rotaryStartAngle, float rotaryEndAngle,
                                       juce::Slider& slider)
{
    const float cx     = x + width  * 0.5f;
    const float cy     = y + height * 0.5f;
    const float radius = juce::jmin (width, height) * 0.5f - 5.0f;
    const float inR    = radius * 0.70f;
    const float trR    = radius * 0.84f;
    const float trW    = (width >= 160) ? 4.0f : 3.0f;
    const float angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const bool  big    = (width >= 160);

    // High-drive glow on big knob
    if (big && sliderPos > 0.78f)
    {
        const float alpha = juce::jmin (1.0f, (sliderPos - 0.78f) * 4.0f);
        g.setColour (CBS::ACCENT.withAlpha (alpha * 0.45f));
        g.drawEllipse (cx - radius - 3, cy - radius - 3,
                       (radius + 3) * 2.0f, (radius + 3) * 2.0f, 4.0f);
    }

    // Bezel
    juce::ColourGradient bzG (CBS::RAISED.brighter (0.1f),
                               cx - radius * 0.28f, cy - radius * 0.28f,
                               CBS::BG,
                               cx + radius, cy + radius, true);
    g.setGradientFill (bzG);
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    g.setColour (CBS::BORDER);
    g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.5f);

    // Body
    juce::ColourGradient bdG (CBS::PANEL.brighter (0.05f),
                               cx - inR * 0.22f, cy - inR * 0.22f,
                               CBS::BG.darker (0.2f),
                               cx + inR, cy + inR, true);
    g.setGradientFill (bdG);
    g.fillEllipse (cx - inR, cy - inR, inR * 2.0f, inR * 2.0f);

    // Track background arc
    juce::Path trackPath;
    trackPath.addCentredArc (cx, cy, trR, trR, 0.0f,
                              rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (CBS::TRACK);
    g.strokePath (trackPath, juce::PathStrokeType (trW, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

    // Active arc
    const float midAngle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * 0.5f;
    const bool  bipolar  = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0
                           && std::abs (slider.getMinimum()) > 0.5;

    float arcStart = rotaryStartAngle;
    float arcEnd   = angle;

    if (bipolar)
    {
        arcStart = midAngle;
        arcEnd   = angle;
        if (arcEnd < arcStart) std::swap (arcStart, arcEnd);
    }

    if (std::abs (arcEnd - arcStart) > 0.01f)
    {
        juce::Path activePath;
        activePath.addCentredArc (cx, cy, trR, trR, 0.0f, arcStart, arcEnd, true);
        g.setColour (CBS::ACCENT);
        g.strokePath (activePath, juce::PathStrokeType (trW, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
    }

    // Indicator line
    const float iS = inR * 0.18f;
    const float iE = inR * 0.70f;
    g.setColour (CBS::ACCENT2);
    g.drawLine (cx + std::cos (angle) * iS, cy + std::sin (angle) * iS,
                cx + std::cos (angle) * iE, cy + std::sin (angle) * iE,
                big ? 2.5f : 2.0f);

    // Tip dot
    const float tipX = cx + std::cos (angle) * iE;
    const float tipY = cy + std::sin (angle) * iE;
    const float tipR = big ? 3.5f : 2.5f;
    g.setColour (CBS::ACCENT2);
    g.fillEllipse (tipX - tipR, tipY - tipR, tipR * 2.0f, tipR * 2.0f);

    // Center dot
    const float cdR = big ? 4.0f : 3.0f;
    g.setColour (CBS::BORDER);
    g.fillEllipse (cx - cdR, cy - cdR, cdR * 2.0f, cdR * 2.0f);

    // Tick marks on big knob
    if (big)
    {
        for (float tickAngle : { rotaryStartAngle, midAngle, rotaryEndAngle })
        {
            g.setColour (CBS::DIM.withAlpha (0.5f));
            g.drawLine (cx + std::cos (tickAngle) * (radius - 7),
                        cy + std::sin (tickAngle) * (radius - 7),
                        cx + std::cos (tickAngle) * (radius - 2),
                        cy + std::sin (tickAngle) * (radius - 2), 1.0f);
        }
    }
}

//==============================================================================
// LookAndFeel — character button
//==============================================================================
void CBSLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                           const juce::Colour&,
                                           bool isMouseOver, bool isButtonDown)
{
    const auto bounds = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on     = b.getToggleState();

    if (on)
    {
        g.setColour (CBS::ACCENT.withAlpha (0.18f));
        g.fillRoundedRectangle (bounds, 2.0f);
    }
    else if (isMouseOver || isButtonDown)
    {
        g.setColour (CBS::RAISED.brighter (0.1f));
        g.fillRoundedRectangle (bounds, 2.0f);
    }
    else
    {
        g.setColour (CBS::RAISED);
        g.fillRoundedRectangle (bounds, 2.0f);
    }

    g.setColour (on ? CBS::ACCENT : CBS::BORDER);
    g.drawRoundedRectangle (bounds, 2.0f, 1.0f);
}

void CBSLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b,
                                     bool, bool)
{
    const bool on = b.getToggleState();
    g.setColour (on ? CBS::ACCENT : CBS::DIM);
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.drawText (b.getButtonText(), b.getLocalBounds(),
                juce::Justification::centred, false);
}

//==============================================================================
// Value formatters
//==============================================================================
juce::String ColourBrickwallScreamAudioProcessorEditor::formatDrive (float norm)
{
    return juce::String (norm * 40.0f, 1) + " dB";
}

juce::String ColourBrickwallScreamAudioProcessorEditor::formatTone (float v)
{
    return (v >= 0 ? "+" : "") + juce::String (v, 2);
}

juce::String ColourBrickwallScreamAudioProcessorEditor::formatColour (float norm)
{
    return juce::String (juce::roundToInt (norm * 100)) + "%";
}

juce::String ColourBrickwallScreamAudioProcessorEditor::formatCeiling (float v)
{
    return juce::String (v, 1) + " dBFS";
}

juce::String ColourBrickwallScreamAudioProcessorEditor::formatMix (float norm)
{
    return juce::String (juce::roundToInt (norm * 100)) + "%";
}

//==============================================================================
// Setup helper
//==============================================================================
void ColourBrickwallScreamAudioProcessorEditor::setupKnob (
    juce::Slider& s, juce::Label& nameLabel, const char* name, juce::Label& valLabel)
{
    s.setLookAndFeel (&laf);
    addAndMakeVisible (s);

    nameLabel.setText (name, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setFont (juce::Font (9.0f, juce::Font::plain));
    nameLabel.setColour (juce::Label::textColourId, CBS::LABEL);
    addAndMakeVisible (nameLabel);

    valLabel.setJustificationType (juce::Justification::centred);
    valLabel.setFont (juce::Font (9.0f, juce::Font::plain));
    valLabel.setColour (juce::Label::textColourId, CBS::ACCENT2);
    addAndMakeVisible (valLabel);
}

//==============================================================================
// Constructor
//==============================================================================
ColourBrickwallScreamAudioProcessorEditor::ColourBrickwallScreamAudioProcessorEditor
    (ColourBrickwallScreamAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&laf);

    // ── Knobs ──────────────────────────────────────────────────────────────────
    setupKnob (driveKnob,   driveNameLabel,   "DRIVE",   driveVal);
    setupKnob (toneKnob,    toneNameLabel,    "TONE",    toneVal);
    setupKnob (colourKnob,  colourNameLabel,  "COLOUR",  colourVal);
    setupKnob (ceilingKnob, ceilingNameLabel, "CEILING", ceilingVal);
    setupKnob (mixKnob,     mixNameLabel,     "MIX",     mixVal);

    // ── Character buttons ──────────────────────────────────────────────────────
    for (int i = 0; i < NUM_CHARS; ++i)
    {
        charButtons[i].setButtonText (CHAR_NAMES[i]);
        charButtons[i].setClickingTogglesState (false);
        charButtons[i].setLookAndFeel (&laf);
        charButtons[i].onClick = [this, i] { setCharacter (i); };
        addAndMakeVisible (charButtons[i]);
    }

    // ── Meters ─────────────────────────────────────────────────────────────────
    for (auto* c : { &meterIn, &meterGR, &meterOut })
        addAndMakeVisible (c);
    meterGR.setReversed (true);

    auto setupMLabel = [this] (juce::Label& l, const char* t) {
        l.setText (t, juce::dontSendNotification);
        l.setFont (juce::Font (8.0f, juce::Font::plain));
        l.setColour (juce::Label::textColourId, CBS::LABEL);
        l.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (l);
    };
    setupMLabel (mLabelIn,  "IN");
    setupMLabel (mLabelGR,  "GR");
    setupMLabel (mLabelOut, "OUT");

    auto setupMVal = [this] (juce::Label& l) {
        l.setText ("— dB", juce::dontSendNotification);
        l.setFont (juce::Font (8.0f, juce::Font::plain));
        l.setColour (juce::Label::textColourId, CBS::DIM);
        l.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (l);
    };
    setupMVal (mValIn);
    setupMVal (mValGR);
    setupMVal (mValOut);

    // ── Parameter attachments ──────────────────────────────────────────────────
    auto& apvts = audioProcessor.apvts;
    driveAtt   = std::make_unique<juce::SliderParameterAttachment> (*apvts.getParameter (ParameterIDs::DRIVE),   driveKnob,   nullptr);
    toneAtt    = std::make_unique<juce::SliderParameterAttachment> (*apvts.getParameter (ParameterIDs::TONE),    toneKnob,    nullptr);
    colourAtt  = std::make_unique<juce::SliderParameterAttachment> (*apvts.getParameter (ParameterIDs::COLOUR),  colourKnob,  nullptr);
    ceilingAtt = std::make_unique<juce::SliderParameterAttachment> (*apvts.getParameter (ParameterIDs::CEILING), ceilingKnob, nullptr);
    mixAtt     = std::make_unique<juce::SliderParameterAttachment> (*apvts.getParameter (ParameterIDs::MIX),     mixKnob,     nullptr);

    // Knob value callbacks
    driveKnob.onValueChange = [this] {
        driveVal.setText (formatDrive ((float)((driveKnob.getValue() - driveKnob.getMinimum()) /
                          (driveKnob.getMaximum() - driveKnob.getMinimum()))),
                          juce::dontSendNotification);
    };
    toneKnob.onValueChange = [this] {
        toneVal.setText (formatTone ((float)toneKnob.getValue()), juce::dontSendNotification);
    };
    colourKnob.onValueChange = [this] {
        colourVal.setText (formatColour ((float)((colourKnob.getValue() - colourKnob.getMinimum()) /
                           (colourKnob.getMaximum() - colourKnob.getMinimum()))),
                           juce::dontSendNotification);
    };
    ceilingKnob.onValueChange = [this] {
        ceilingVal.setText (formatCeiling ((float)ceilingKnob.getValue()), juce::dontSendNotification);
    };
    mixKnob.onValueChange = [this] {
        mixVal.setText (formatMix ((float)((mixKnob.getValue() - mixKnob.getMinimum()) /
                        (mixKnob.getMaximum() - mixKnob.getMinimum()))),
                        juce::dontSendNotification);
    };

    // Seed initial value labels
    driveKnob.onValueChange();
    toneKnob.onValueChange();
    colourKnob.onValueChange();
    ceilingKnob.onValueChange();
    mixKnob.onValueChange();

    // ── Initial char button state ──────────────────────────────────────────────
    updateCharButtons();

    // ── Window ────────────────────────────────────────────────────────────────
    setResizable (true, true);
    setResizeLimits (420, 300, 1400, 1000);
    getConstrainer()->setFixedAspectRatio (800.0 / 570.0);
    setSize (800, 570);

    startTimerHz (30);
}

ColourBrickwallScreamAudioProcessorEditor::~ColourBrickwallScreamAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
// Character helpers
//==============================================================================
void ColourBrickwallScreamAudioProcessorEditor::setCharacter (int idx)
{
    if (auto* p = dynamic_cast<juce::AudioParameterInt*> (
            audioProcessor.apvts.getParameter (ParameterIDs::CHARACTER)))
        *p = idx;

    updateCharButtons();
}

void ColourBrickwallScreamAudioProcessorEditor::updateCharButtons()
{
    const int cur = (int) std::round (
        audioProcessor.apvts.getRawParameterValue (ParameterIDs::CHARACTER)->load());

    for (int i = 0; i < NUM_CHARS; ++i)
        charButtons[i].setToggleState (i == cur, juce::dontSendNotification);
}

//==============================================================================
// Paint
//==============================================================================
void ColourBrickwallScreamAudioProcessorEditor::paint (juce::Graphics& g)
{
    const int W = getWidth();
    const int H = getHeight();

    // Background
    g.fillAll (CBS::BG);

    // Header background
    const int headerH = juce::roundToInt (H * 0.12f);
    g.setColour (CBS::PANEL);
    g.fillRect (0, 0, W, headerH);

    // Header separator
    g.setColour (CBS::SEP);
    g.fillRect (0, headerH, W, 1);

    // Bottom panel background
    const int bottomH = juce::roundToInt (H * 0.245f);
    g.setColour (CBS::PANEL);
    g.fillRect (0, H - bottomH, W, bottomH);

    // Bottom separator
    g.setColour (CBS::SEP);
    g.fillRect (0, H - bottomH, W, 1);

    // Plugin title
    const float scale = W / 800.0f;
    g.setColour (CBS::TEXT);
    g.setFont (juce::Font (juce::roundToInt (15.0f * scale), juce::Font::plain));
    g.drawText ("skr.colour_breakwall",
                juce::roundToInt (16 * scale), juce::roundToInt (12 * scale),
                juce::roundToInt (220 * scale), juce::roundToInt (18 * scale),
                juce::Justification::centredLeft, false);

    // Subtitle
    g.setColour (CBS::ACCENT);
    g.setFont (juce::Font (juce::roundToInt (7.5f * scale), juce::Font::plain));
    g.drawText ("HARMONIC SATURATOR  \xc2\xb7  BRICKWALL LIMITER",
                juce::roundToInt (16 * scale), juce::roundToInt (33 * scale),
                juce::roundToInt (280 * scale), juce::roundToInt (14 * scale),
                juce::Justification::centredLeft, false);
}

//==============================================================================
// Resized
//==============================================================================
void ColourBrickwallScreamAudioProcessorEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();
    const float s = W / 800.0f;   // scale factor

    auto sc = [&] (float v) { return juce::roundToInt (v * s); };

    // ── Header layout ──────────────────────────────────────────────────────────
    const int headerH  = sc (68);
    const int meterX   = sc (260);
    const int meterW   = W - meterX - sc (12);
    const int mLabelW  = sc (22);
    const int mValW    = sc (52);
    const int mBarW    = meterW - mLabelW - mValW - sc (12);
    const int mRowH    = sc (12);
    const int mTopY    = sc (14);

    for (int i = 0; i < 3; ++i)
    {
        juce::Label* lbl  = (i == 0 ? &mLabelIn  : (i == 1 ? &mLabelGR  : &mLabelOut));
        MeterBar*    bar  = (i == 0 ? &meterIn    : (i == 1 ? &meterGR   : &meterOut));
        juce::Label* val  = (i == 0 ? &mValIn     : (i == 1 ? &mValGR    : &mValOut));
        const int    rowY = mTopY + i * sc (17);

        lbl->setBounds (meterX, rowY, mLabelW, mRowH);
        bar->setBounds (meterX + mLabelW + sc (4), rowY, mBarW, mRowH);
        val->setBounds (meterX + mLabelW + sc (4) + mBarW + sc (4), rowY, mValW, mRowH);
    }

    // ── Bottom row ─────────────────────────────────────────────────────────────
    const int bottomH  = sc (140);
    const int bottomY  = H - bottomH;
    const int smKnobSz = sc (80);
    const int smLblH   = sc (12);

    auto layoutSmallKnob = [&] (juce::Slider& k, juce::Label& nl, juce::Label& vl, int cx) {
        k.setBounds  (cx - smKnobSz / 2, bottomY + sc (12), smKnobSz, smKnobSz);
        nl.setBounds (cx - sc (50), bottomY + sc (12) + smKnobSz + sc (2), sc (100), smLblH);
        vl.setBounds (cx - sc (50), bottomY + sc (12) + smKnobSz + sc (14), sc (100), smLblH);
    };

    layoutSmallKnob (ceilingKnob, ceilingNameLabel, ceilingVal, sc (250));
    layoutSmallKnob (mixKnob,     mixNameLabel,     mixVal,     sc (560));

    // ── Main area ──────────────────────────────────────────────────────────────
    const int mainY = headerH + 1;
    const int mainH = H - headerH - 1 - bottomH;

    const int lgKnobSz = sc (180);
    const int smSideKnobSz = sc (100);
    const int knobLblH = sc (14);
    const int knobValH = sc (14);

    const int centerX = W / 2;
    const int knobCY  = mainY + (mainH - lgKnobSz - knobLblH * 2 - sc (32)) / 2 + lgKnobSz / 2;

    // Drive (center)
    driveKnob.setBounds  (centerX - lgKnobSz / 2, knobCY - lgKnobSz / 2, lgKnobSz, lgKnobSz);
    driveNameLabel.setBounds (centerX - sc (60), knobCY + lgKnobSz / 2 + sc (2), sc (120), knobLblH);
    driveVal.setBounds       (centerX - sc (60), knobCY + lgKnobSz / 2 + sc (14), sc (120), knobValH);

    // Character buttons below drive label
    {
        const int btnY  = knobCY + lgKnobSz / 2 + knobLblH * 2 + sc (20);
        const int btnW  = sc (56);
        const int btnH  = sc (22);
        const int gap   = sc (4);
        const int totalW = NUM_CHARS * btnW + (NUM_CHARS - 1) * gap;
        int bx = centerX - totalW / 2;
        for (int i = 0; i < NUM_CHARS; ++i)
        {
            charButtons[i].setBounds (bx, btnY, btnW, btnH);
            bx += btnW + gap;
        }
    }

    // Tone (left side)
    const int sideKnobCY = mainY + mainH / 2 - sc (8);
    toneKnob.setBounds  (sc (40), sideKnobCY - smSideKnobSz / 2, smSideKnobSz, smSideKnobSz);
    toneNameLabel.setBounds (sc (40), sideKnobCY + smSideKnobSz / 2 + sc (2), smSideKnobSz, knobLblH);
    toneVal.setBounds       (sc (40), sideKnobCY + smSideKnobSz / 2 + sc (14), smSideKnobSz, knobValH);

    // Colour (right side)
    const int rightSideX = W - sc (40) - smSideKnobSz;
    colourKnob.setBounds  (rightSideX, sideKnobCY - smSideKnobSz / 2, smSideKnobSz, smSideKnobSz);
    colourNameLabel.setBounds (rightSideX, sideKnobCY + smSideKnobSz / 2 + sc (2), smSideKnobSz, knobLblH);
    colourVal.setBounds       (rightSideX, sideKnobCY + smSideKnobSz / 2 + sc (14), smSideKnobSz, knobValH);
}

//==============================================================================
// Timer: poll meters and char state
//==============================================================================
void ColourBrickwallScreamAudioProcessorEditor::timerCallback()
{
    // Update meters
    const float mIn  = audioProcessor.meterInput.load();
    const float mGR  = audioProcessor.meterGR.load();
    const float mOut = audioProcessor.meterOutput.load();

    meterIn.setLevel  (mIn);
    meterGR.setLevel  (mGR);
    meterOut.setLevel (mOut);
    meterIn.repaint();
    meterGR.repaint();
    meterOut.repaint();

    // dB labels
    auto toDb = [] (float lin) -> juce::String {
        if (lin < 0.001f) return "— dB";
        return juce::String (20.0f * std::log10 (lin), 1) + " dB";
    };
    mValIn.setText  (toDb (mIn),  juce::dontSendNotification);
    mValOut.setText (toDb (mOut), juce::dontSendNotification);

    // GR: invert direction
    const float grDb = (mGR < 0.001f) ? 0.0f : -20.0f * std::log10 (juce::jmax (0.001f, 1.0f - mGR * 0.6f));
    mValGR.setText ((grDb > 0.05f ? "-" : " ") + juce::String (std::abs (grDb), 1) + " dB",
                    juce::dontSendNotification);

    // Sync character buttons (handles automation)
    updateCharButtons();
}
