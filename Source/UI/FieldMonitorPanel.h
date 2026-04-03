#pragma once

#include <array>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../ScopeDataFifo.h"
#include "../SubOrbitTypes.h"
#include "CRTLookAndFeel.h"

class FieldMonitorPanel : public juce::Component
{
  public:
    explicit FieldMonitorPanel (ScopeDataFifo& fifo);

    void resized () override;
    void paint (juce::Graphics& g) override;

    void setAnalysisState (const SubOrbitAnalysisState& newState);

    juce::TextButton& getMonoCheckButton ()
    {
        return monoCheckButton;
    }

  private:
    class GoniometerComponent : public juce::Component
    {
      public:
        explicit GoniometerComponent (ScopeDataFifo& fifo);

        void paint (juce::Graphics& g) override;
        void resized () override;

      private:
        void vblankCallback ();
        void rebuildDotMask ();

        ScopeDataFifo& scopeFifo;
        std::array<StereoScopeSample, 2048> samples{};
        int sampleCount = 0;

        juce::Image historyBuffer;
        juce::Image dotMask;

        juce::VBlankAttachment vblankAttachment{this, [this] { vblankCallback (); }};
    };

    class CorrelationMeter : public juce::Component
    {
      public:
        void setCorrelation (float newCorrelation);
        void paint (juce::Graphics& g) override;

      private:
        float correlation = 1.0f;
    };

    ScopeDataFifo& scopeFifo;
    GoniometerComponent goniometer;
    CorrelationMeter correlationMeter;
    juce::Label statusLabel;
    juce::Label correlationRiskLabel;
    juce::Label correlationSafeLabel;
    juce::TextButton monoCheckButton;
};
