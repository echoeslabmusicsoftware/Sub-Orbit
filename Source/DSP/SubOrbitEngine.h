#pragma once

#include <array>
#include <atomic>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "../SubOrbitTypes.h"
#include "LowBandAnalyzer.h"
#include "QuadratureAllpassPair.h"
#include "SidechainEnvelopeFollower.h"

namespace SubOrbit::DSP
{

class SubOrbitEngine
{
  public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset ();
    void setParameters (const SubOrbitDspParameters& newParameters);
    void processBlock (juce::AudioBuffer<float>& buffer);

    void processSidechainBlock (const float* data, int numSamples);
    [[nodiscard]] float getSidechainEnvelopeLevel () const noexcept;

    [[nodiscard]] SubOrbitAnalysisState getAnalysisState () const noexcept
    {
        return analysisBuffers[static_cast<size_t> (latestIndex.load (std::memory_order_acquire))];
    }

    /** Returns the latency in samples introduced by internal resampling.
        Zero for native sample rates (44.1/48 kHz). */
    [[nodiscard]] int getLatencySamples () const noexcept;

  private:
    struct QuadratureConfig
    {
        bool supported = false;
        bool usesInternalResampling = false;
        double internalSampleRate = 0.0;
        int resampleFactor = 1;
    };

    void updateSplitFilterCutoff (double newCutoffHz);
    static QuadratureConfig getQuadratureConfig (double hostSampleRate) noexcept;
    static float angleToOrbit (float alphaRadians) noexcept;

    juce::dsp::LinkwitzRileyFilter<double> splitFilter;
    QuadratureAllpassPair quadraturePair;
    LowBandAnalyzer analyzer;
    juce::LagrangeInterpolator quadratureUpsamplerI;
    juce::LagrangeInterpolator quadratureUpsamplerQ;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> alphaRadians;
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Multiplicative> rangeHz;

    void publishAnalysisState (const SubOrbitAnalysisState& state) noexcept;

    SubOrbitDspParameters parameters{};
    std::array<SubOrbitAnalysisState, 3> analysisBuffers{};
    std::atomic<int> latestIndex{0};
    int writeIndex = 1;
    double sampleRate = 44100.0;
    int blockSize = 0;
    double lastRangeHz = 100.0;
    double quadratureInternalSampleRate = 44100.0;
    int quadratureResampleFactor = 1;
    int quadratureResamplePhase = 0;
    double quadratureResampleAccumulator = 0.0;
    double quadratureAlphaAccumulator = 0.0;
    double heldQuadratureI = 0.0;
    double heldQuadratureQ = 0.0;
    std::vector<float> lowMidBlock;
    std::vector<float> highLeftBlock;
    std::vector<float> highRightBlock;
    std::vector<float> quadratureIBlock;
    std::vector<float> quadratureQBlock;
    std::vector<float> fullRateIBlock;
    std::vector<float> fullRateQBlock;
    bool quadratureSupported = true;
    bool quadratureUsesInternalResampling = false;

    SidechainEnvelopeFollower sidechainFollower;
};

} // namespace SubOrbit::DSP
