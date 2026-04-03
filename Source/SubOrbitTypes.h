#pragma once

#include "SubOrbitParameterSpecs.h"

namespace SubOrbit
{

enum class MonitorState
{
    open,
    limit,
};

enum class UiScale
{
    small = 0,
    medium,
    large,
};

struct DspParameters
{
    float orbit = SubOrbitParameterSpecs::kOrbit.defaultValue;
    float rangeHz = SubOrbitParameterSpecs::kRangeHz.defaultValue;
    float sidechainAmount = SubOrbitParameterSpecs::kSidechainAmount.defaultValue;
    float sidechainAttackMs = SubOrbitParameterSpecs::kSidechainAttackMs.defaultValue;
    float sidechainReleaseMs = SubOrbitParameterSpecs::kSidechainReleaseMs.defaultValue;
};

struct AnalysisState
{
    float lowBandCorrelation = 1.0f;
    float effectiveOrbit = 0.0f;
    bool sampleRateSupported = true;
    float sidechainEnvelopeLevel = 0.0f;
};

} // namespace SubOrbit

using SubOrbitDspParameters = SubOrbit::DspParameters;
using SubOrbitAnalysisState = SubOrbit::AnalysisState;
using MonitorState = SubOrbit::MonitorState;
using UiScale = SubOrbit::UiScale;
