#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class SidechainPanel : public juce::Component
{
  public:
    SidechainPanel ();

    void resized () override;
    void paint (juce::Graphics& g) override;

    juce::Slider& getAttackSlider () noexcept
    {
        return attackSlider;
    }

    juce::Slider& getReleaseSlider () noexcept
    {
        return releaseSlider;
    }

  private:
    void updateAttackValueLabel (float value);
    void updateReleaseValueLabel (float value);

    juce::Label titleLabel;
    juce::Label attackLabel;
    juce::Label attackValueLabel;
    juce::Label releaseLabel;
    juce::Label releaseValueLabel;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
};
