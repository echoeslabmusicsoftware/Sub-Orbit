#include "SubOrbitEngine.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace SubOrbit::DSP
{

namespace Constants
{
constexpr float kAlphaRampSeconds = 0.01f;
constexpr double kRangeRampSeconds = 0.02;
constexpr float kMaxAlpha = juce::MathConstants<float>::pi / 4.0f;
constexpr float kOrbitEpsilon = 1.0e-4f;
constexpr int kCutoffUpdateInterval = 32;
constexpr double kCutoffChangeThreshold = 0.01;
constexpr int kTrigUpdateInterval = 8;
} // namespace Constants

using namespace Constants;

SubOrbitEngine::QuadratureConfig SubOrbitEngine::getQuadratureConfig (double hostSampleRate) noexcept
{
    const auto isApproxSampleRate = [hostSampleRate] (double expectedRate)
    { return std::abs (hostSampleRate - expectedRate) < 1.0; };

    if (isApproxSampleRate (44100.0) || isApproxSampleRate (48000.0))
        return {true, false, hostSampleRate, 1};

    if (isApproxSampleRate (88200.0))
        return {true, true, 44100.0, 2};

    if (isApproxSampleRate (96000.0))
        return {true, true, 48000.0, 2};

    if (isApproxSampleRate (176400.0))
        return {true, true, 44100.0, 4};

    if (isApproxSampleRate (192000.0))
        return {true, true, 48000.0, 4};

    return {};
}

void SubOrbitEngine::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    blockSize = static_cast<int> (spec.maximumBlockSize);
    alphaRadians.reset (sampleRate, kAlphaRampSeconds);
    rangeHz.reset (sampleRate, kRangeRampSeconds);
    alphaRadians.setCurrentAndTargetValue (0.0f);
    rangeHz.setCurrentAndTargetValue (lastRangeHz);

    const auto quadratureConfig = getQuadratureConfig (sampleRate);
    quadratureSupported = quadratureConfig.supported;
    quadratureUsesInternalResampling = quadratureConfig.usesInternalResampling;
    quadratureInternalSampleRate = quadratureSupported ? quadratureConfig.internalSampleRate : sampleRate;
    quadratureResampleFactor = quadratureSupported ? juce::jmax (1, quadratureConfig.resampleFactor) : 1;

    splitFilter.prepare (spec);

    if (quadratureSupported)
        quadraturePair.prepare (quadratureInternalSampleRate);

    sidechainFollower.prepare ({spec.sampleRate, spec.maximumBlockSize, 1});

    if (quadratureUsesInternalResampling)
    {
        const auto fullRateBufferSize = juce::jmax (1, blockSize);
        const auto lowRateBufferSize =
            juce::jmax (2, (blockSize + quadratureResampleFactor - 1) / quadratureResampleFactor + 1);
        lowMidBlock.resize (static_cast<size_t> (fullRateBufferSize));
        highLeftBlock.resize (static_cast<size_t> (fullRateBufferSize));
        highRightBlock.resize (static_cast<size_t> (fullRateBufferSize));
        quadratureIBlock.resize (static_cast<size_t> (lowRateBufferSize));
        quadratureQBlock.resize (static_cast<size_t> (lowRateBufferSize));
        fullRateIBlock.resize (static_cast<size_t> (fullRateBufferSize));
        fullRateQBlock.resize (static_cast<size_t> (fullRateBufferSize));
    }

    analyzer.reset ();
    updateSplitFilterCutoff (lastRangeHz);
    reset ();
}

void SubOrbitEngine::reset ()
{
    splitFilter.reset ();
    quadraturePair.reset ();
    analyzer.reset ();
    sidechainFollower.reset ();
    quadratureUpsamplerI.reset ();
    quadratureUpsamplerQ.reset ();
    quadratureResamplePhase = 0;
    quadratureResampleAccumulator = 0.0;
    quadratureAlphaAccumulator = 0.0;
    heldQuadratureI = 0.0;
    heldQuadratureQ = 0.0;

    const auto resetRangeHz = juce::jlimit (60.0, 180.0, static_cast<double> (parameters.rangeHz));
    rangeHz.setCurrentAndTargetValue (resetRangeHz);
    updateSplitFilterCutoff (resetRangeHz);

    const auto requestedAlpha = juce::jlimit (0.0f, kMaxAlpha, parameters.orbit * kMaxAlpha);
    alphaRadians.setCurrentAndTargetValue (requestedAlpha);

    analysisBuffers = {};
    latestIndex.store (0, std::memory_order_release);
    writeIndex = 1;

    auto resetAnalysisState = SubOrbitAnalysisState{};
    resetAnalysisState.sampleRateSupported = quadratureSupported;
    publishAnalysisState (resetAnalysisState);
}

void SubOrbitEngine::setParameters (const SubOrbitDspParameters& newParameters)
{
    parameters = newParameters;
    rangeHz.setTargetValue (juce::jlimit (60.0, 180.0, static_cast<double> (parameters.rangeHz)));
    sidechainFollower.setAttackTime (parameters.sidechainAttackMs);
    sidechainFollower.setReleaseTime (parameters.sidechainReleaseMs);

    const auto envelope = sidechainFollower.getEnvelopeLevel ();
    const auto ducking = envelope * parameters.sidechainAmount;
    const auto effectiveOrbit = parameters.orbit * (1.0f - ducking);

    const auto requestedAlpha = juce::jlimit (0.0f, kMaxAlpha, effectiveOrbit * kMaxAlpha);
    alphaRadians.setTargetValue (requestedAlpha);
}

void SubOrbitEngine::processBlock (juce::AudioBuffer<float>& buffer)
{
    jassert (buffer.getNumChannels () >= 2);

    double sumLL = 0.0;
    double sumRR = 0.0;
    double sumLR = 0.0;
    double cosAlpha = 1.0;
    double sinAlpha = 0.0;

    const auto envelope = sidechainFollower.getEnvelopeLevel ();
    const auto ducking = envelope * parameters.sidechainAmount;
    const auto effectiveOrbit = parameters.orbit * (1.0f - ducking);

    const auto numSamples = buffer.getNumSamples ();

    auto* leftChannel = buffer.getWritePointer (0);
    auto* rightChannel = buffer.getWritePointer (1);

    if (!quadratureSupported)
    {
        auto analysisState = SubOrbitAnalysisState{};
        analysisState.sampleRateSupported = false;
        publishAnalysisState (analysisState);
        return;
    }

    if (quadratureUsesInternalResampling)
    {
        auto numResampledSamples = 0;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto inL = static_cast<double> (leftChannel[sample]);
            const auto inR = static_cast<double> (rightChannel[sample]);
            const auto alpha = static_cast<double> (alphaRadians.getNextValue ());

            const auto smoothedRangeHz = rangeHz.getNextValue ();
            if (((sample & (kCutoffUpdateInterval - 1)) == 0) || sample == numSamples - 1)
            {
                if (std::abs (smoothedRangeHz - lastRangeHz) > kCutoffChangeThreshold)
                    updateSplitFilterCutoff (smoothedRangeHz);
            }

            double lowL = 0.0;
            double highL = 0.0;
            double lowR = 0.0;
            double highR = 0.0;
            splitFilter.processSample (0, inL, lowL, highL);
            splitFilter.processSample (1, inR, lowR, highR);

            const auto lowMid = 0.5 * (lowL + lowR);
            lowMidBlock[static_cast<size_t> (sample)] = static_cast<float> (lowMid);
            highLeftBlock[static_cast<size_t> (sample)] = static_cast<float> (highL);
            highRightBlock[static_cast<size_t> (sample)] = static_cast<float> (highR);

            quadratureResampleAccumulator += lowMid;
            quadratureAlphaAccumulator += alpha;
            ++quadratureResamplePhase;

            if (quadratureResamplePhase >= quadratureResampleFactor)
            {
                const auto averagedAlpha = quadratureAlphaAccumulator / static_cast<double> (quadratureResampleFactor);
                const auto [i, q] = quadraturePair.processSample (quadratureResampleAccumulator /
                                                                  static_cast<double> (quadratureResampleFactor));
                const auto lowCos = std::cos (averagedAlpha);
                const auto lowSin = std::sin (averagedAlpha);
                heldQuadratureI = lowCos * i + lowSin * q;
                heldQuadratureQ = lowCos * i - lowSin * q;
                quadratureIBlock[static_cast<size_t> (numResampledSamples)] = static_cast<float> (heldQuadratureI);
                quadratureQBlock[static_cast<size_t> (numResampledSamples)] = static_cast<float> (heldQuadratureQ);
                ++numResampledSamples;
                quadratureResampleAccumulator = 0.0;
                quadratureAlphaAccumulator = 0.0;
                quadratureResamplePhase = 0;
            }
        }

        if (numResampledSamples == 0)
        {
            quadratureIBlock[0] = static_cast<float> (heldQuadratureI);
            quadratureQBlock[0] = static_cast<float> (heldQuadratureQ);
            numResampledSamples = 1;
        }

        quadratureIBlock[static_cast<size_t> (numResampledSamples)] =
            quadratureIBlock[static_cast<size_t> (numResampledSamples - 1)];
        quadratureQBlock[static_cast<size_t> (numResampledSamples)] =
            quadratureQBlock[static_cast<size_t> (numResampledSamples - 1)];
        ++numResampledSamples;

        quadratureUpsamplerI.process (1.0 / static_cast<double> (quadratureResampleFactor),
                                      quadratureIBlock.data (),
                                      fullRateIBlock.data (),
                                      numSamples,
                                      numResampledSamples,
                                      0);
        quadratureUpsamplerQ.process (1.0 / static_cast<double> (quadratureResampleFactor),
                                      quadratureQBlock.data (),
                                      fullRateQBlock.data (),
                                      numSamples,
                                      numResampledSamples,
                                      0);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto lowOutL = static_cast<double> (fullRateIBlock[static_cast<size_t> (sample)]);
            const auto lowOutR = static_cast<double> (fullRateQBlock[static_cast<size_t> (sample)]);

            sumLL += lowOutL * lowOutL;
            sumRR += lowOutR * lowOutR;
            sumLR += lowOutL * lowOutR;

            leftChannel[sample] =
                static_cast<float> (static_cast<double> (highLeftBlock[static_cast<size_t> (sample)]) + lowOutL);
            rightChannel[sample] =
                static_cast<float> (static_cast<double> (highRightBlock[static_cast<size_t> (sample)]) + lowOutR);
        }
    }
    else
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto inL = static_cast<double> (leftChannel[sample]);
            const auto inR = static_cast<double> (rightChannel[sample]);

            const auto smoothedRangeHz = rangeHz.getNextValue ();
            if (((sample & (kCutoffUpdateInterval - 1)) == 0) || sample == numSamples - 1)
            {
                if (std::abs (smoothedRangeHz - lastRangeHz) > kCutoffChangeThreshold)
                    updateSplitFilterCutoff (smoothedRangeHz);
            }

            double lowL = 0.0;
            double highL = 0.0;
            double lowR = 0.0;
            double highR = 0.0;
            splitFilter.processSample (0, inL, lowL, highL);
            splitFilter.processSample (1, inR, lowR, highR);

            const auto lowMid = 0.5 * (lowL + lowR);
            double i = 0.0;
            double q = 0.0;
            std::tie (i, q) = quadraturePair.processSample (lowMid);

            const auto alpha = static_cast<double> (alphaRadians.getNextValue ());
            if ((sample & (kTrigUpdateInterval - 1)) == 0 || sample == numSamples - 1)
            {
                sinAlpha = std::sin (alpha);
                cosAlpha = std::cos (alpha);
            }

            const auto lowOutL = cosAlpha * i + sinAlpha * q;
            const auto lowOutR = cosAlpha * i - sinAlpha * q;

            sumLL += lowOutL * lowOutL;
            sumRR += lowOutR * lowOutR;
            sumLR += lowOutL * lowOutR;

            leftChannel[sample] = static_cast<float> (highL + lowOutL);
            rightChannel[sample] = static_cast<float> (highR + lowOutR);
        }
    }

    // The plugin processor wraps engine calls in ScopedNoDenormals, but the engine is
    // also exercised directly in tests and other headless contexts. Snap the sample-by-
    // sample crossover state to zero at block boundaries so denormals cannot linger.
    splitFilter.snapToZero ();

    auto analysisState = analyzer.update (sumLL,
                                          sumRR,
                                          sumLR,
                                          angleToOrbit (alphaRadians.getTargetValue ()),
                                          quadratureSupported && effectiveOrbit > kOrbitEpsilon,
                                          buffer.getNumSamples (),
                                          sampleRate);
    analysisState.sampleRateSupported = quadratureSupported;
    analysisState.sidechainEnvelopeLevel = envelope;
    publishAnalysisState (analysisState);
}

void SubOrbitEngine::processSidechainBlock (const float* data, int numSamples)
{
    sidechainFollower.processBlock (data, numSamples);
}

float SubOrbitEngine::getSidechainEnvelopeLevel () const noexcept
{
    return sidechainFollower.getEnvelopeLevel ();
}

void SubOrbitEngine::updateSplitFilterCutoff (double newCutoffHz)
{
    lastRangeHz = juce::jlimit (60.0, 180.0, newCutoffHz);
    splitFilter.setCutoffFrequency (lastRangeHz);
}

void SubOrbitEngine::publishAnalysisState (const SubOrbitAnalysisState& state) noexcept
{
    analysisBuffers[static_cast<size_t> (writeIndex)] = state;
    const auto previousLatest = latestIndex.exchange (writeIndex, std::memory_order_acq_rel);
    writeIndex = previousLatest;
}

int SubOrbitEngine::getLatencySamples () const noexcept
{
    if (!quadratureUsesInternalResampling)
        return 0;

    // Decimation averages resampleFactor samples; centre-of-mass delay ≈ (factor-1)/2.
    // Lagrange 4th-order upsampler adds ≈ 2 samples at the internal (low) rate,
    // which maps to 2*factor samples at the host rate.
    return (quadratureResampleFactor - 1) / 2 + 2 * quadratureResampleFactor;
}

float SubOrbitEngine::angleToOrbit (float alphaRadians) noexcept
{
    return juce::jlimit (0.0f, 1.0f, alphaRadians / kMaxAlpha);
}

} // namespace SubOrbit::DSP
