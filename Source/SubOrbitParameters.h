#pragma once

#include <array>
#include <cmath>
#include <string_view>

#include <juce_audio_processors/juce_audio_processors.h>

#include "SubOrbitParameterSpecs.h"
#include "SubOrbitTypes.h"

namespace SubOrbitParameters
{
inline constexpr std::string_view kOrbitId = SubOrbitParameterSpecs::kOrbit.id;
inline constexpr std::string_view kRangeHzId = SubOrbitParameterSpecs::kRangeHz.id;
inline constexpr std::string_view kSidechainAmountId = SubOrbitParameterSpecs::kSidechainAmount.id;
inline constexpr std::string_view kSidechainAttackMsId = SubOrbitParameterSpecs::kSidechainAttackMs.id;
inline constexpr std::string_view kSidechainReleaseMsId = SubOrbitParameterSpecs::kSidechainReleaseMs.id;
inline constexpr int kParameterVersionHint = SubOrbitParameterSpecs::kParameterVersionHint;

inline constexpr std::array<std::string_view, 5> getAutomatableParameterIDs ()
{
    return SubOrbitParameterSpecs::kAutomatableParameterIDs;
}

inline constexpr SubOrbitDspParameters makeDefaultDspParameters () noexcept
{
    return {SubOrbitParameterSpecs::kOrbit.defaultValue,
            SubOrbitParameterSpecs::kRangeHz.defaultValue,
            SubOrbitParameterSpecs::kSidechainAmount.defaultValue,
            SubOrbitParameterSpecs::kSidechainAttackMs.defaultValue,
            SubOrbitParameterSpecs::kSidechainReleaseMs.defaultValue};
}

inline juce::NormalisableRange<float> makeLogNormalisableRange (float minValue, float maxValue)
{
    return {minValue,
            maxValue,
            [] (float start, float end, float proportion)
            {
                const auto logStart = std::log (start);
                const auto logEnd = std::log (end);
                return std::exp (logStart + (logEnd - logStart) * proportion);
            },
            [] (float start, float end, float value)
            {
                const auto logStart = std::log (start);
                const auto logEnd = std::log (end);
                return (std::log (value) - logStart) / (logEnd - logStart);
            }};
}

inline juce::ParameterID makeParameterID (std::string_view id)
{
    return {std::string{id}, kParameterVersionHint};
}

inline std::unique_ptr<juce::AudioParameterFloat>
makeFloatParameter (const SubOrbitParameterSpecs::FloatParameterSpec& spec)
{
    auto range = juce::NormalisableRange<float>{spec.minValue, spec.maxValue, spec.step};

    if (spec.scaling == SubOrbitParameterSpecs::FloatScaling::logarithmic)
        range = makeLogNormalisableRange (spec.minValue, spec.maxValue);

    return std::make_unique<juce::AudioParameterFloat> (
        makeParameterID (spec.id),
        juce::String{std::string{spec.name}},
        std::move (range),
        spec.defaultValue,
        juce::AudioParameterFloatAttributes ().withLabel (juce::String{std::string{spec.label}}));
}

inline std::unique_ptr<juce::AudioParameterBool>
makeBoolParameter (const SubOrbitParameterSpecs::BoolParameterSpec& spec)
{
    return std::make_unique<juce::AudioParameterBool> (makeParameterID (spec.id),
                                                       juce::String{std::string{spec.name}},
                                                       spec.defaultValue,
                                                       juce::AudioParameterBoolAttributes ());
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout ()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (makeFloatParameter (SubOrbitParameterSpecs::kOrbit));
    layout.add (makeFloatParameter (SubOrbitParameterSpecs::kRangeHz));
    layout.add (makeFloatParameter (SubOrbitParameterSpecs::kSidechainAmount));
    layout.add (makeFloatParameter (SubOrbitParameterSpecs::kSidechainAttackMs));
    layout.add (makeFloatParameter (SubOrbitParameterSpecs::kSidechainReleaseMs));

    return layout;
}

inline constexpr std::array<std::string_view, 2> getNonAutomatableControlIDs ()
{
    return {"monoCheck", "uiScale"};
}

inline SubOrbitDspParameters readParameters (const juce::AudioProcessorValueTreeState& state)
{
    const auto load = [&] (std::string_view id) { return state.getRawParameterValue (std::string{id})->load (); };

    return {load (kOrbitId),
            load (kRangeHzId),
            load (kSidechainAmountId),
            load (kSidechainAttackMsId),
            load (kSidechainReleaseMsId)};
}

} // namespace SubOrbitParameters
