#pragma once

#include <cmath>

#include <juce_core/juce_core.h>

#include "../SubOrbitTypes.h"

namespace SubOrbit::DSP
{

class LowBandAnalyzer
{
  public:
    void reset () noexcept
    {
        smoothedCorrelation = 1.0f;
        state = {};
    }

    SubOrbitAnalysisState update (double sumLL,
                                  double sumRR,
                                  double sumLR,
                                  float effectiveOrbit,
                                  bool active,
                                  int numSamples,
                                  double sampleRate) noexcept
    {
        if (!active)
        {
            state.lowBandCorrelation = 1.0f;
            state.effectiveOrbit = 0.0f;
            smoothedCorrelation = 1.0f;
            return state;
        }

        const auto denom = std::sqrt (sumLL * sumRR + 1.0e-12);
        const auto rawCorrelation = denom > 0.0 ? static_cast<float> (sumLR / denom) : 1.0f;

        const auto blockSeconds = static_cast<float> (numSamples) / static_cast<float> (sampleRate);
        const auto alpha = 1.0f - std::exp (-blockSeconds / kSmoothingTimeSeconds);
        smoothedCorrelation = (1.0f - alpha) * smoothedCorrelation + alpha * juce::jlimit (-1.0f, 1.0f, rawCorrelation);

        state.lowBandCorrelation = smoothedCorrelation;
        state.effectiveOrbit = effectiveOrbit;

        return state;
    }

  private:
    static constexpr float kSmoothingTimeSeconds = 0.065f;
    float smoothedCorrelation = 1.0f;
    SubOrbitAnalysisState state{};
};

} // namespace SubOrbit::DSP
