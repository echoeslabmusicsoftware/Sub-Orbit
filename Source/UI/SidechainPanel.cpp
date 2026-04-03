#include "SidechainPanel.h"

#include "CRTLookAndFeel.h"

SidechainPanel::SidechainPanel ()
{
    addAndMakeVisible (titleLabel);
    titleLabel.setText ("SIDECHAIN", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::FontOptions (14.0f));

    addAndMakeVisible (attackSlider);
    attackSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    attackSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    attackSlider.setRotaryParameters (
        juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    attackSlider.setDoubleClickReturnValue (true, 10.0);
    attackSlider.setTooltip ("Set sidechain envelope attack time (rise speed).");
    attackSlider.onValueChange = [this] { updateAttackValueLabel (static_cast<float> (attackSlider.getValue ())); };

    addAndMakeVisible (releaseSlider);
    releaseSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    releaseSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    releaseSlider.setRotaryParameters (
        juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    releaseSlider.setDoubleClickReturnValue (true, 300.0);
    releaseSlider.setTooltip ("Set sidechain envelope release time (fall speed).");
    releaseSlider.onValueChange = [this] { updateReleaseValueLabel (static_cast<float> (releaseSlider.getValue ())); };

    addAndMakeVisible (attackLabel);
    attackLabel.setText ("ATTACK", juce::dontSendNotification);
    attackLabel.setJustificationType (juce::Justification::centred);
    attackLabel.setFont (juce::FontOptions (11.0f));

    addAndMakeVisible (attackValueLabel);
    attackValueLabel.setText ("10.0 ms", juce::dontSendNotification);
    attackValueLabel.setJustificationType (juce::Justification::centred);
    attackValueLabel.setFont (juce::FontOptions (10.0f));

    addAndMakeVisible (releaseLabel);
    releaseLabel.setText ("RELEASE", juce::dontSendNotification);
    releaseLabel.setJustificationType (juce::Justification::centred);
    releaseLabel.setFont (juce::FontOptions (11.0f));

    addAndMakeVisible (releaseValueLabel);
    releaseValueLabel.setText ("300 ms", juce::dontSendNotification);
    releaseValueLabel.setJustificationType (juce::Justification::centred);
    releaseValueLabel.setFont (juce::FontOptions (10.0f));
}

void SidechainPanel::resized ()
{
    auto bounds = getLocalBounds ().reduced (12);

    // Title at the top
    auto titleArea = bounds.removeFromTop (22);
    titleLabel.setBounds (titleArea);
    bounds.removeFromTop (12); // gap below title

    // Center the two knobs horizontally, constrain for smaller panel height
    const auto knobSize = juce::jlimit (60, 80, juce::jmin (bounds.getWidth () / 3, bounds.getHeight () / 3));
    const auto knobsAreaWidth = knobSize * 2 + 20; // two knobs + gap between
    const auto knobsStartX = bounds.getCentreX () - knobsAreaWidth / 2;

    auto attackArea = bounds.withX (knobsStartX).withWidth (knobSize);
    attackArea.removeFromTop ((bounds.getHeight () - knobSize - 50) / 2); // vertical centering

    auto releaseArea = bounds.withX (knobsStartX + knobSize + 24).withWidth (knobSize);
    releaseArea.removeFromTop ((bounds.getHeight () - knobSize - 50) / 2); // vertical centering

    // Attack: name label → knob → value label
    auto attackNameArea = attackArea.removeFromTop (14);
    attackLabel.setBounds (attackNameArea);
    attackArea.removeFromTop (4);

    auto attackKnobArea = attackArea.removeFromTop (knobSize);
    attackSlider.setBounds (attackKnobArea.withSizeKeepingCentre (knobSize, knobSize));

    attackArea.removeFromTop (4);
    auto attackValueArea = attackArea.removeFromTop (14);
    attackValueLabel.setBounds (attackValueArea);

    // Release: name label → knob → value label
    auto releaseNameArea = releaseArea.removeFromTop (14);
    releaseLabel.setBounds (releaseNameArea);
    releaseArea.removeFromTop (4);

    auto releaseKnobArea = releaseArea.removeFromTop (knobSize);
    releaseSlider.setBounds (releaseKnobArea.withSizeKeepingCentre (knobSize, knobSize));

    releaseArea.removeFromTop (4);
    auto releaseValueArea = releaseArea.removeFromTop (14);
    releaseValueLabel.setBounds (releaseValueArea);
}

void SidechainPanel::updateAttackValueLabel (float value)
{
    // Format: 1 decimal place, e.g. "10.0 ms", "0.5 ms"
    attackValueLabel.setText (juce::String (value, 1) + " ms", juce::dontSendNotification);
}

void SidechainPanel::updateReleaseValueLabel (float value)
{
    // Format: integers for < 1000 ms, seconds with 1 decimal for >= 1000 ms
    juce::String text;
    if (value < 1000.0f)
    {
        text = juce::String (juce::roundToInt (value)) + " ms";
    }
    else
    {
        text = juce::String (value / 1000.0f, 1) + " s";
    }
    releaseValueLabel.setText (text, juce::dontSendNotification);
}

void SidechainPanel::paint (juce::Graphics& g)
{
    CRTLookAndFeel::drawTerminalBorder (g, getLocalBounds ().toFloat ().reduced (0.5f), 8.0f);
}
