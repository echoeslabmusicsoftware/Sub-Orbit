#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "UI/CRTLookAndFeel.h"
#include "UI/FieldMonitorPanel.h"
#include "UI/OrbitPanel.h"
#include "UI/SidechainPanel.h"
#include "UI/TipsOverlay.h"

class SubOrbitAudioProcessorEditor final : public juce::AudioProcessorEditor
{
  public:
    explicit SubOrbitAudioProcessorEditor (SubOrbitAudioProcessor&);
    ~SubOrbitAudioProcessorEditor () override;

    void paint (juce::Graphics& g) override;
    void resized () override;
    bool keyPressed (const juce::KeyPress& key) override;
    void mouseUp (const juce::MouseEvent& event) override;

  private:
    void vblankCallback ();
    void rebuildBackground ();
    void applyScale (UiScale scale);

    SubOrbitAudioProcessor& processorRef;
    CRTLookAndFeel lookAndFeel;
    juce::Image cachedBackground;

    std::unique_ptr<juce::Drawable> logoGreen;
    juce::Rectangle<int> logoBounds;

    juce::Label titleLabel;
    juce::TextButton tipsButton;
    std::unique_ptr<TipsOverlay> tipsOverlay;

    FieldMonitorPanel fieldMonitorPanel;
    OrbitPanel orbitPanel;
    SidechainPanel sidechainPanel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> orbitAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rangeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sidechainAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sidechainReleaseAttachment;
    juce::VBlankAttachment vblankAttachment{this, [this] { vblankCallback (); }};
};
