#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace
{
constexpr auto kStateRootType = "SubOrbitState";
constexpr auto kStateSchemaVersionProperty = "schemaVersion";
constexpr int kCurrentStateSchemaVersion = 1;
constexpr auto kUiScaleProperty = "uiScale";

bool isValidUiScaleValue (int value) noexcept
{
    return value >= static_cast<int> (UiScale::small) && value <= static_cast<int> (UiScale::large);
}

int sanitiseUiScaleValue (int value) noexcept
{
    return isValidUiScaleValue (value) ? value : static_cast<int> (UiScale::medium);
}
} // namespace

SubOrbitAudioProcessor::SubOrbitAudioProcessor ()
    : juce::AudioProcessor (BusesProperties ()
                                .withInput ("Input", juce::AudioChannelSet::stereo (), true)
                                .withInput ("Sidechain", juce::AudioChannelSet::mono (), false)
                                .withOutput ("Output", juce::AudioChannelSet::stereo (), true)),
      parameters (*this, nullptr, "PARAMETERS", SubOrbitParameters::createParameterLayout ())
{
    cachedParamPointers.orbit = parameters.getRawParameterValue (std::string{SubOrbitParameters::kOrbitId});
    cachedParamPointers.rangeHz = parameters.getRawParameterValue (std::string{SubOrbitParameters::kRangeHzId});
    cachedParamPointers.sidechainAmount =
        parameters.getRawParameterValue (std::string{SubOrbitParameters::kSidechainAmountId});
    cachedParamPointers.sidechainAttackMs =
        parameters.getRawParameterValue (std::string{SubOrbitParameters::kSidechainAttackMsId});
    cachedParamPointers.sidechainReleaseMs =
        parameters.getRawParameterValue (std::string{SubOrbitParameters::kSidechainReleaseMsId});
}

void SubOrbitAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 2;
    engine.prepare (spec);
    engine.setParameters (readCachedParameters ());
    engine.reset ();
    setLatencySamples (engine.getLatencySamples ());

    monoCheckSmoothed.reset (sampleRate, kMonoRampSeconds);
    monoCheckSmoothed.setCurrentAndTargetValue (monoCheckActive.load (std::memory_order_relaxed) ? 1.0f : 0.0f);
}

void SubOrbitAudioProcessor::releaseResources ()
{
}

void SubOrbitAudioProcessor::reset ()
{
    engine.setParameters (readCachedParameters ());
    engine.reset ();
}

bool SubOrbitAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getMainInputChannelSet ();
    const auto mainOut = layouts.getMainOutputChannelSet ();

    if (mainOut != juce::AudioChannelSet::stereo ())
        return false;
    if (mainIn != juce::AudioChannelSet::stereo () && !mainIn.isDisabled ())
        return false;

    if (layouts.inputBuses.size () > 1)
    {
        const auto sc = layouts.inputBuses[1];
        if (!sc.isDisabled () && sc != juce::AudioChannelSet::mono ())
            return false;
    }

    return true;
}

void SubOrbitAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto channel = getTotalNumInputChannels (); channel < getTotalNumOutputChannels (); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples ());

    auto sidechainBuffer = getBusBuffer (buffer, true, 1);
    if (sidechainBuffer.getNumChannels () > 0 && sidechainBuffer.getNumSamples () > 0)
        engine.processSidechainBlock (sidechainBuffer.getReadPointer (0), sidechainBuffer.getNumSamples ());

    engine.setParameters (readCachedParameters ());

    auto mainBuffer = getBusBuffer (buffer, true, 0);
    engine.processBlock (mainBuffer);

    applyMonoCheck (buffer);

    pushScopeData (buffer);
}

void SubOrbitAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    applyMonoCheck (buffer);

    pushScopeData (buffer);
}

juce::AudioProcessorEditor* SubOrbitAudioProcessor::createEditor ()
{
    return new SubOrbitAudioProcessorEditor (*this);
}

void SubOrbitAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto root = juce::ValueTree (kStateRootType);
    root.setProperty (kStateSchemaVersionProperty, kCurrentStateSchemaVersion, nullptr);
    root.setProperty (kUiScaleProperty, sanitiseUiScaleValue (uiScale.load (std::memory_order_relaxed)), nullptr);
    root.addChild (parameters.copyState (), -1, nullptr);

    if (auto xml = root.createXml ())
        copyXmlToBinary (*xml, destData);
}

void SubOrbitAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto root = juce::ValueTree::fromXml (*xml);
        if (!root.isValid ())
            return;

        if (root.getType () == parameters.state.getType ())
        {
            uiScale.store (static_cast<int> (UiScale::medium), std::memory_order_relaxed);
            parameters.replaceState (root);
            return;
        }

        const auto storedUiScale =
            static_cast<int> (root.getProperty (kUiScaleProperty, static_cast<int> (UiScale::medium)));
        uiScale.store (sanitiseUiScaleValue (storedUiScale), std::memory_order_relaxed);

        if (auto state = root.getChildWithName (parameters.state.getType ()); state.isValid ())
            parameters.replaceState (state);
    }
}

void SubOrbitAudioProcessor::pushScopeData (const juce::AudioBuffer<float>& buffer) noexcept
{
    if (buffer.getNumChannels () < 2)
        return;

    scopeDataFifo.push (buffer.getReadPointer (0), buffer.getReadPointer (1), buffer.getNumSamples ());
}

void SubOrbitAudioProcessor::applyMonoCheck (juce::AudioBuffer<float>& buffer) noexcept
{
    monoCheckSmoothed.setTargetValue (monoCheckActive.load (std::memory_order_relaxed) ? 1.0f : 0.0f);

    if (!monoCheckSmoothed.isSmoothing () && monoCheckSmoothed.getTargetValue () == 0.0f)
        return;

    for (int sample = 0; sample < buffer.getNumSamples (); ++sample)
    {
        const auto blend = monoCheckSmoothed.getNextValue ();
        const auto left = buffer.getSample (0, sample);
        const auto right = buffer.getSample (1, sample);
        const auto mono = 0.5f * (left + right);
        buffer.setSample (0, sample, left + blend * (mono - left));
        buffer.setSample (1, sample, right + blend * (mono - right));
    }
}

SubOrbitDspParameters SubOrbitAudioProcessor::readCachedParameters () const noexcept
{
    auto params = SubOrbitParameters::makeDefaultDspParameters ();
    if (cachedParamPointers.orbit != nullptr)
        params.orbit = cachedParamPointers.orbit->load (std::memory_order_relaxed);
    if (cachedParamPointers.rangeHz != nullptr)
        params.rangeHz = cachedParamPointers.rangeHz->load (std::memory_order_relaxed);
    if (cachedParamPointers.sidechainAmount != nullptr)
        params.sidechainAmount = cachedParamPointers.sidechainAmount->load (std::memory_order_relaxed);
    if (cachedParamPointers.sidechainAttackMs != nullptr)
        params.sidechainAttackMs = cachedParamPointers.sidechainAttackMs->load (std::memory_order_relaxed);
    if (cachedParamPointers.sidechainReleaseMs != nullptr)
        params.sidechainReleaseMs = cachedParamPointers.sidechainReleaseMs->load (std::memory_order_relaxed);
    return params;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter ()
{
    return new SubOrbitAudioProcessor ();
}
