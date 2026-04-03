#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../SubOrbitTypes.h"

class OrbitPanel : public juce::Component
{
  public:
    OrbitPanel ();

    void resized () override;
    void paint (juce::Graphics& g) override;

    juce::Slider& getOrbitSlider () noexcept
    {
        return orbitSlider;
    }
    juce::Slider& getRangeSlider () noexcept
    {
        return rangeSlider;
    }

    void setOrbitStateLabel (float orbitValue);
    void setRangeValueLabel (float rangeHz);

  private:
    juce::Label titleLabel;
    juce::Label orbitValueLabel;
    juce::Label crossoverLabel;
    juce::Label rangeValueLabel;
    juce::Slider orbitSlider;
    juce::Slider rangeSlider;
};
