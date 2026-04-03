#pragma once

#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/SubOrbitEngine.h"
#include "ScopeDataFifo.h"
#include "SubOrbitParameters.h"
#include "SubOrbitTypes.h"

class SubOrbitAudioProcessor final : public juce::AudioProcessor
{
  public:
    SubOrbitAudioProcessor ();
    ~SubOrbitAudioProcessor () override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources () override;
    void reset () override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    [[nodiscard]] juce::AudioProcessorEditor* createEditor () override;
    [[nodiscard]] bool hasEditor () const override
    {
        return true;
    }

    [[nodiscard]] const juce::String getName () const override
    {
        return JucePlugin_Name;
    }
    [[nodiscard]] bool acceptsMidi () const override
    {
        return false;
    }
    [[nodiscard]] bool producesMidi () const override
    {
        return false;
    }
    [[nodiscard]] bool isMidiEffect () const override
    {
        return false;
    }
    [[nodiscard]] double getTailLengthSeconds () const override
    {
        return 0.02;
    }
    [[nodiscard]] int getNumPrograms () override
    {
        return 1;
    }
    [[nodiscard]] int getCurrentProgram () override
    {
        return 0;
    }
    void setCurrentProgram (int) override
    {
    }
    [[nodiscard]] const juce::String getProgramName (int) override
    {
        return {};
    }
    void changeProgramName (int, const juce::String&) override
    {
    }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState () noexcept
    {
        return parameters;
    }
    ScopeDataFifo& getScopeDataFifo () noexcept
    {
        return scopeDataFifo;
    }
    SubOrbitAnalysisState getAnalysisState () const noexcept
    {
        return engine.getAnalysisState ();
    }

    void setMonoCheckActive (bool shouldBeActive) noexcept
    {
        monoCheckActive.store (shouldBeActive, std::memory_order_relaxed);
    }
    [[nodiscard]] bool isMonoCheckActive () const noexcept
    {
        return monoCheckActive.load (std::memory_order_relaxed);
    }

    void setUiScale (UiScale newScale) noexcept
    {
        uiScale.store (static_cast<int> (newScale), std::memory_order_relaxed);
    }
    [[nodiscard]] UiScale getUiScale () const noexcept
    {
        return static_cast<UiScale> (uiScale.load (std::memory_order_relaxed));
    }

  private:
    void pushScopeData (const juce::AudioBuffer<float>& buffer) noexcept;
    void applyMonoCheck (juce::AudioBuffer<float>& buffer) noexcept;
    [[nodiscard]] SubOrbitDspParameters readCachedParameters () const noexcept;

    struct CachedParameterPointers
    {
        std::atomic<float>* orbit = nullptr;
        std::atomic<float>* rangeHz = nullptr;
        std::atomic<float>* sidechainAmount = nullptr;
        std::atomic<float>* sidechainAttackMs = nullptr;
        std::atomic<float>* sidechainReleaseMs = nullptr;
    };

    static constexpr double kMonoRampSeconds = 0.005;

    juce::AudioProcessorValueTreeState parameters;
    CachedParameterPointers cachedParamPointers;
    ScopeDataFifo scopeDataFifo;
    SubOrbit::DSP::SubOrbitEngine engine;
    std::atomic<bool> monoCheckActive{false};
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> monoCheckSmoothed;
    std::atomic<int> uiScale{static_cast<int> (UiScale::medium)};
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter ();
