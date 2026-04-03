#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "CRTLookAndFeel.h"

class TipsOverlay final : public juce::Component
{
  public:
    TipsOverlay ();

    void paint (juce::Graphics& g) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent& event) override;
    bool keyPressed (const juce::KeyPress& key) override;

  private:
    class TipsPanel final : public juce::Component
    {
      public:
        TipsPanel ();

        void paint (juce::Graphics& g) override;
        void resized () override;

      private:
        juce::Label titleLabel;
        juce::TextButton closeButton;
        juce::HyperlinkButton linkButton;
        juce::Label versionLabel;
    };

    std::unique_ptr<TipsPanel> panel;
};
