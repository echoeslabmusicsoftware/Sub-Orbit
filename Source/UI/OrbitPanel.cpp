#include "OrbitPanel.h"

#include "CRTLookAndFeel.h"

OrbitPanel::OrbitPanel ()
{
    addAndMakeVisible (titleLabel);
    titleLabel.setText ("ORBIT", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::FontOptions (11.0f));

    addAndMakeVisible (orbitSlider);
    orbitSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    orbitSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    orbitSlider.setRotaryParameters (
        juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    orbitSlider.setDoubleClickReturnValue (true, 0.0);
    orbitSlider.setTooltip ("Set how wide the low band feels.");

    addAndMakeVisible (rangeSlider);
    rangeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    rangeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    rangeSlider.setDoubleClickReturnValue (true, 100.0);
    rangeSlider.setTooltip ("Choose the highest frequency that receives widening.");

    addAndMakeVisible (orbitValueLabel);
    orbitValueLabel.setText ("0%", juce::dontSendNotification);
    orbitValueLabel.setJustificationType (juce::Justification::centred);
    orbitValueLabel.setFont (juce::FontOptions (12.0f));

    addAndMakeVisible (crossoverLabel);
    crossoverLabel.setText ("CROSSOVER", juce::dontSendNotification);
    crossoverLabel.setJustificationType (juce::Justification::centred);
    crossoverLabel.setFont (juce::FontOptions (9.0f));

    addAndMakeVisible (rangeValueLabel);
    rangeValueLabel.setText ("100 Hz", juce::dontSendNotification);
    rangeValueLabel.setJustificationType (juce::Justification::centred);
    rangeValueLabel.setFont (juce::FontOptions (12.0f));
}

void OrbitPanel::resized ()
{
    auto bounds = getLocalBounds ().reduced (14);

    // Title at top (centered)
    titleLabel.setBounds (bounds.removeFromTop (16));
    bounds.removeFromTop (4);

    // Reserve crossover section at bottom: label + slider + hz label
    auto crossoverSection = bounds.removeFromBottom (52);
    bounds.removeFromBottom (6);

    // Orbit knob centered in remaining area
    const auto orbitDiameter = juce::jlimit (100, 150, juce::jmin (bounds.getWidth () - 20, bounds.getHeight () - 4));
    auto knobCentre = bounds.getCentre ();
    auto knobBounds = juce::Rectangle<int> (orbitDiameter, orbitDiameter).withCentre (knobCentre);
    orbitSlider.setBounds (knobBounds);

    // Value label inside the lower third of the knob arc
    const auto valueLabelY = knobCentre.y + orbitDiameter / 4;
    orbitValueLabel.setBounds (knobBounds.getX (), valueLabelY, orbitDiameter, 16);

    // Crossover: label centered, slider centered below, Hz centered below slider
    crossoverLabel.setBounds (crossoverSection.removeFromTop (14));
    crossoverSection.removeFromTop (2);

    const auto sliderInset = 24;
    auto sliderArea = crossoverSection.removeFromTop (20);
    rangeSlider.setBounds (sliderArea.reduced (sliderInset, 0));

    rangeValueLabel.setBounds (crossoverSection);
}

void OrbitPanel::paint (juce::Graphics& g)
{
    CRTLookAndFeel::drawTerminalBorder (g, getLocalBounds ().toFloat ().reduced (0.5f), 8.0f);
}

void OrbitPanel::setOrbitStateLabel (float orbitValue)
{
    orbitValueLabel.setText (juce::String (juce::roundToInt (orbitValue * 100.0f)) + "%", juce::dontSendNotification);
}

void OrbitPanel::setRangeValueLabel (float rangeHz)
{
    const auto text =
        rangeHz < 100.0f ? juce::String (rangeHz, 1) + " Hz" : juce::String (juce::roundToInt (rangeHz)) + " Hz";
    rangeValueLabel.setText (text, juce::dontSendNotification);
}
