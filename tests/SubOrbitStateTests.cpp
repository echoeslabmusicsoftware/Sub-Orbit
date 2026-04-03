#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <juce_audio_processors/juce_audio_processors.h>

#ifndef JucePlugin_Name
#define JucePlugin_Name "SUB ORBIT"
#endif

#include "PluginProcessor.h"
#include "SubOrbitParameters.h"

namespace
{
class DummyAudioProcessor final : public juce::AudioProcessor
{
  public:
    DummyAudioProcessor ()
        : juce::AudioProcessor (BusesProperties ()
                                    .withInput ("Input", juce::AudioChannelSet::stereo (), true)
                                    .withOutput ("Output", juce::AudioChannelSet::stereo (), true))
    {
    }

    void prepareToPlay (double, int) override
    {
    }
    void releaseResources () override
    {
    }
    bool isBusesLayoutSupported (const BusesLayout&) const override
    {
        return true;
    }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override
    {
    }

    [[nodiscard]] juce::AudioProcessorEditor* createEditor () override
    {
        return nullptr;
    }
    [[nodiscard]] bool hasEditor () const override
    {
        return false;
    }

    [[nodiscard]] const juce::String getName () const override
    {
        return "DummyAudioProcessor";
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
        return 0.0;
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

    void getStateInformation (juce::MemoryBlock&) override
    {
    }
    void setStateInformation (const void*, int) override
    {
    }
};

juce::AudioParameterFloat& requireFloatParameter (juce::AudioProcessorValueTreeState& state, std::string_view id)
{
    auto* parameter = state.getParameter (juce::String{std::string{id}});
    REQUIRE (parameter != nullptr);

    auto* floatParameter = dynamic_cast<juce::AudioParameterFloat*> (parameter);
    REQUIRE (floatParameter != nullptr);
    return *floatParameter;
}

juce::ValueTree requireStateTree (const juce::MemoryBlock& data)
{
    auto xml = juce::AudioProcessor::getXmlFromBinary (data.getData (), static_cast<int> (data.getSize ()));
    REQUIRE (xml != nullptr);

    auto tree = juce::ValueTree::fromXml (*xml);
    REQUIRE (tree.isValid ());
    return tree;
}

juce::MemoryBlock createStateData (const juce::ValueTree& tree)
{
    juce::MemoryBlock data;
    if (auto xml = tree.createXml ())
        juce::AudioProcessor::copyXmlToBinary (*xml, data);

    REQUIRE (data.getSize () > 0);
    return data;
}
} // namespace

TEST_CASE ("SubOrbit APVTS layout matches the six-parameter contract", "[state][params]")
{
    DummyAudioProcessor processor;
    juce::AudioProcessorValueTreeState state (
        processor, nullptr, "PARAMETERS", SubOrbitParameters::createParameterLayout ());
    const auto ids = SubOrbitParameters::getAutomatableParameterIDs ();
    const auto defaults = SubOrbitParameters::makeDefaultDspParameters ();

    REQUIRE (processor.getParameters ().size () == ids.size ());
    REQUIRE (ids.size () == 5);
    REQUIRE (ids[0] == SubOrbitParameters::kOrbitId);
    REQUIRE (ids[1] == SubOrbitParameters::kRangeHzId);
    REQUIRE (ids[2] == SubOrbitParameters::kSidechainAmountId);
    REQUIRE (ids[3] == SubOrbitParameters::kSidechainAttackMsId);
    REQUIRE (ids[4] == SubOrbitParameters::kSidechainReleaseMsId);

    const auto& orbit = requireFloatParameter (state, SubOrbitParameters::kOrbitId);
    REQUIRE (orbit.getName (32) == "Orbit");
    REQUIRE (orbit.getLabel () == "");
    REQUIRE (orbit.getVersionHint () == SubOrbitParameters::kParameterVersionHint);
    REQUIRE (orbit.range.start == 0.0f);
    REQUIRE (orbit.range.end == 1.0f);
    REQUIRE (orbit.range.interval == 0.001f);
    REQUIRE (orbit.get () == defaults.orbit);

    const auto& range = requireFloatParameter (state, SubOrbitParameters::kRangeHzId);
    REQUIRE (range.getName (32) == "Range");
    REQUIRE (range.getLabel () == "Hz");
    REQUIRE (range.getVersionHint () == SubOrbitParameters::kParameterVersionHint);
    REQUIRE (range.range.start == 60.0f);
    REQUIRE (range.range.end == 180.0f);
    REQUIRE (range.get () == defaults.rangeHz);
}

TEST_CASE ("Mono check and uiScale are not host parameters", "[state][params]")
{
    DummyAudioProcessor processor;
    juce::AudioProcessorValueTreeState state (
        processor, nullptr, "PARAMETERS", SubOrbitParameters::createParameterLayout ());
    const auto ids = SubOrbitParameters::getAutomatableParameterIDs ();
    const auto nonHostIds = SubOrbitParameters::getNonAutomatableControlIDs ();

    REQUIRE (std::find (ids.begin (), ids.end (), "monoCheck") == ids.end ());
    REQUIRE (std::find (ids.begin (), ids.end (), "uiScale") == ids.end ());
    REQUIRE (nonHostIds[0] == "monoCheck");
    REQUIRE (nonHostIds[1] == "uiScale");
    REQUIRE (state.getParameter ("monoCheck") == nullptr);
    REQUIRE (state.getParameter ("uiScale") == nullptr);
}

TEST_CASE ("SubOrbit processor state round-trips host parameters and uiScale without persisting monoCheck",
           "[state][processor]")
{
    SubOrbitAudioProcessor source;
    auto& sourceState = source.getValueTreeState ();

    requireFloatParameter (sourceState, SubOrbitParameters::kOrbitId) = 0.625f;
    requireFloatParameter (sourceState, SubOrbitParameters::kRangeHzId) = 140.0f;
    source.setUiScale (UiScale::large);
    source.setMonoCheckActive (true);

    juce::MemoryBlock data;
    source.getStateInformation (data);

    const auto tree = requireStateTree (data);
    REQUIRE (tree.getType ().toString () == "SubOrbitState");
    REQUIRE (static_cast<int> (tree.getProperty ("schemaVersion", -1)) == 1);
    REQUIRE (static_cast<int> (tree.getProperty ("uiScale", -1)) == static_cast<int> (UiScale::large));

    SubOrbitAudioProcessor restored;
    restored.setStateInformation (data.getData (), static_cast<int> (data.getSize ()));

    const auto restoredParams = SubOrbitParameters::readParameters (restored.getValueTreeState ());
    REQUIRE (restoredParams.orbit == Catch::Approx (0.625f).margin (0.0001f));
    REQUIRE (restoredParams.rangeHz == Catch::Approx (140.0f).margin (0.0001f));
    REQUIRE (restored.getUiScale () == UiScale::large);
    REQUIRE (restored.isMonoCheckActive () == false);
}

TEST_CASE ("SubOrbit processor repairs missing or invalid uiScale during restore", "[state][processor]")
{
    SubOrbitAudioProcessor source;
    requireFloatParameter (source.getValueTreeState (), SubOrbitParameters::kOrbitId) = 0.25f;

    juce::MemoryBlock originalData;
    source.getStateInformation (originalData);
    auto tree = requireStateTree (originalData);

    SECTION ("missing uiScale falls back to medium")
    {
        tree.removeProperty ("uiScale", nullptr);
    }

    SECTION ("invalid uiScale falls back to medium")
    {
        tree.setProperty ("uiScale", 99, nullptr);
    }

    const auto modifiedData = createStateData (tree);

    SubOrbitAudioProcessor restored;
    restored.setUiScale (UiScale::small);
    restored.setStateInformation (modifiedData.getData (), static_cast<int> (modifiedData.getSize ()));

    REQUIRE (restored.getUiScale () == UiScale::medium);

    const auto restoredParams = SubOrbitParameters::readParameters (restored.getValueTreeState ());
    REQUIRE (restoredParams.orbit == Catch::Approx (0.25f).margin (0.0001f));
}

TEST_CASE ("SubOrbit processor reset clears DSP state before further processing", "[state][processor]")
{
    SubOrbitAudioProcessor processor;
    auto& state = processor.getValueTreeState ();

    requireFloatParameter (state, SubOrbitParameters::kOrbitId) = 1.0f;
    requireFloatParameter (state, SubOrbitParameters::kRangeHzId) = 100.0f;

    processor.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> signal (2, 512);
    for (int sample = 0; sample < 512; ++sample)
    {
        const auto value =
            0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * 80.0f * static_cast<float> (sample) / 48000.0f);
        signal.setSample (0, sample, value);
        signal.setSample (1, sample, value);
    }

    juce::MidiBuffer midi;
    processor.processBlock (signal, midi);

    processor.reset ();

    const auto resetAnalysis = processor.getAnalysisState ();
    REQUIRE (resetAnalysis.effectiveOrbit == Catch::Approx (0.0f).margin (1.0e-6f));

    juce::AudioBuffer<float> silence (2, 512);
    silence.clear ();
    processor.processBlock (silence, midi);

    for (int channel = 0; channel < 2; ++channel)
        for (int sample = 0; sample < silence.getNumSamples (); ++sample)
            REQUIRE (silence.getSample (channel, sample) == Catch::Approx (0.0f).margin (1.0e-6f));
}

TEST_CASE ("processBlockBypassed passes audio through and feeds scope data", "[state][processor]")
{
    SubOrbitAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    for (int sample = 0; sample < 512; ++sample)
    {
        const auto value =
            0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * 80.0f * static_cast<float> (sample) / 48000.0f);
        buffer.setSample (0, sample, value);
        buffer.setSample (1, sample, value);
    }

    auto expected = buffer;
    juce::MidiBuffer midi;
    processor.processBlockBypassed (buffer, midi);

    for (int channel = 0; channel < 2; ++channel)
        for (int sample = 0; sample < 512; ++sample)
            REQUIRE (buffer.getSample (channel, sample) == expected.getSample (channel, sample));

    std::array<StereoScopeSample, 1024> scopeOut{};
    const auto pulled = processor.getScopeDataFifo ().pull (scopeOut.data (), static_cast<int> (scopeOut.size ()));
    REQUIRE (pulled == 512);
}

TEST_CASE ("SubOrbit supports stereo in + mono sidechain + stereo out bus layout", "[state][buses]")
{
    SubOrbitAudioProcessor processor;

    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add (juce::AudioChannelSet::stereo ());
    layout.inputBuses.add (juce::AudioChannelSet::mono ());
    layout.outputBuses.add (juce::AudioChannelSet::stereo ());

    REQUIRE (processor.isBusesLayoutSupported (layout));
}

TEST_CASE ("SubOrbit rejects stereo sidechain bus", "[state][buses]")
{
    SubOrbitAudioProcessor processor;

    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add (juce::AudioChannelSet::stereo ());
    layout.inputBuses.add (juce::AudioChannelSet::stereo ());
    layout.outputBuses.add (juce::AudioChannelSet::stereo ());

    REQUIRE (!processor.isBusesLayoutSupported (layout));
}

TEST_CASE ("SubOrbit supports stereo in + no sidechain + stereo out bus layout", "[state][buses]")
{
    SubOrbitAudioProcessor processor;

    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add (juce::AudioChannelSet::stereo ());
    layout.outputBuses.add (juce::AudioChannelSet::stereo ());

    REQUIRE (processor.isBusesLayoutSupported (layout));
}

TEST_CASE ("SubOrbit supports stereo in + disabled sidechain + stereo out", "[state][buses]")
{
    SubOrbitAudioProcessor processor;

    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add (juce::AudioChannelSet::stereo ());
    layout.inputBuses.add (juce::AudioChannelSet::disabled ());
    layout.outputBuses.add (juce::AudioChannelSet::stereo ());

    REQUIRE (processor.isBusesLayoutSupported (layout));
}

TEST_CASE ("SubOrbit APVTS layout includes sidechainAmount parameter", "[state][params]")
{
    DummyAudioProcessor processor;
    juce::AudioProcessorValueTreeState state (
        processor, nullptr, "PARAMETERS", SubOrbitParameters::createParameterLayout ());
    const auto ids = SubOrbitParameters::getAutomatableParameterIDs ();

    REQUIRE (ids.size () == 5);
    REQUIRE (ids[2] == SubOrbitParameters::kSidechainAmountId);

    const auto& scAmount = requireFloatParameter (state, SubOrbitParameters::kSidechainAmountId);
    REQUIRE (scAmount.getName (32) == "SC Amount");
    REQUIRE (scAmount.getVersionHint () == SubOrbitParameters::kParameterVersionHint);
    REQUIRE (scAmount.range.start == 0.0f);
    REQUIRE (scAmount.range.end == 1.0f);
    REQUIRE (scAmount.get () == Catch::Approx (1.0f));
}
