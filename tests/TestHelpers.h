#pragma once

#include <algorithm>
#include <cmath>
#include <random>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace TestHelpers
{

inline juce::AudioBuffer<float>
makeLowSine (float frequency, float seconds, float amplitude, double sampleRate = 48000.0)
{
    const auto numSamples = static_cast<int> (seconds * static_cast<float> (sampleRate));
    juce::AudioBuffer<float> buffer (2, numSamples);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto phase = juce::MathConstants<double>::twoPi * static_cast<double> (frequency) *
                           static_cast<double> (sample) / sampleRate;
        const auto value = amplitude * static_cast<float> (std::sin (phase));
        buffer.setSample (0, sample, value);
        buffer.setSample (1, sample, value);
    }

    return buffer;
}

inline juce::AudioBuffer<float>
makeLinearSweep (float startFrequency, float endFrequency, float seconds, float amplitude, double sampleRate = 48000.0)
{
    const auto numSamples = static_cast<int> (seconds * static_cast<float> (sampleRate));
    juce::AudioBuffer<float> buffer (2, numSamples);

    auto phase = 0.0;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto t = static_cast<double> (sample) / sampleRate;
        const auto sweepProgress = juce::jlimit (0.0, 1.0, t / static_cast<double> (seconds));
        const auto frequency =
            juce::jmap (sweepProgress, static_cast<double> (startFrequency), static_cast<double> (endFrequency));

        phase += juce::MathConstants<double>::twoPi * frequency / sampleRate;
        const auto value = amplitude * static_cast<float> (std::sin (phase));
        buffer.setSample (0, sample, value);
        buffer.setSample (1, sample, value);
    }

    return buffer;
}

inline juce::AudioBuffer<float> makeNoise (float seconds, float amplitude, double sampleRate = 48000.0)
{
    const auto numSamples = static_cast<int> (seconds * static_cast<float> (sampleRate));
    juce::AudioBuffer<float> buffer (2, numSamples);
    std::minstd_rand random (12345);
    std::uniform_real_distribution<float> distribution (-amplitude, amplitude);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto value = distribution (random);
        buffer.setSample (0, sample, value);
        buffer.setSample (1, sample, value);
    }

    return buffer;
}

inline float meanDeltaNearBlockEnd (const juce::AudioBuffer<float>& buffer, int channel, int lookback)
{
    const auto startSample = buffer.getNumSamples () - lookback;
    float sum = 0.0f;

    for (int sample = startSample; sample < buffer.getNumSamples (); ++sample)
        sum += std::abs (buffer.getSample (channel, sample) - buffer.getSample (channel, sample - 1));

    return sum / static_cast<float> (lookback);
}

inline float maxDeltaNearBlockStart (const juce::AudioBuffer<float>& previous,
                                     const juce::AudioBuffer<float>& next,
                                     int channel,
                                     int window)
{
    auto maxDelta =
        std::abs (next.getSample (channel, 0) - previous.getSample (channel, previous.getNumSamples () - 1));

    for (int sample = 1; sample < window; ++sample)
        maxDelta =
            std::max (maxDelta, std::abs (next.getSample (channel, sample) - next.getSample (channel, sample - 1)));

    return maxDelta;
}

inline double measureChannelRms (const juce::AudioBuffer<float>& buffer, int channel, int startSample)
{
    const auto numSamples = buffer.getNumSamples () - startSample;
    if (numSamples <= 0)
        return 0.0;

    auto sumSquares = 0.0;

    for (int sample = startSample; sample < buffer.getNumSamples (); ++sample)
    {
        const auto value = static_cast<double> (buffer.getSample (channel, sample));
        sumSquares += value * value;
    }

    return std::sqrt (sumSquares / static_cast<double> (numSamples));
}

struct StereoMetrics
{
    double leftRms = 0.0;
    double rightRms = 0.0;
    double monoRms = 0.0;
    double sideRms = 0.0;
};

inline StereoMetrics measureStereoMetrics (const juce::AudioBuffer<float>& buffer, int startSample)
{
    const auto numSamples = buffer.getNumSamples () - startSample;
    if (numSamples <= 0)
        return {};

    double sumL2 = 0.0;
    double sumR2 = 0.0;
    double sumM2 = 0.0;
    double sumS2 = 0.0;

    for (int sample = startSample; sample < buffer.getNumSamples (); ++sample)
    {
        const auto left = static_cast<double> (buffer.getSample (0, sample));
        const auto right = static_cast<double> (buffer.getSample (1, sample));
        const auto mono = 0.5 * (left + right);
        const auto side = 0.5 * (left - right);

        sumL2 += left * left;
        sumR2 += right * right;
        sumM2 += mono * mono;
        sumS2 += side * side;
    }

    const auto scale = 1.0 / static_cast<double> (numSamples);
    return {std::sqrt (sumL2 * scale), std::sqrt (sumR2 * scale), std::sqrt (sumM2 * scale), std::sqrt (sumS2 * scale)};
}

template <typename EngineType>
juce::AudioBuffer<float> processInBlocks (EngineType& engine, const juce::AudioBuffer<float>& input, int blockSize)
{
    juce::AudioBuffer<float> output (input.getNumChannels (), input.getNumSamples ());
    juce::AudioBuffer<float> block (input.getNumChannels (), blockSize);

    for (int startSample = 0; startSample < input.getNumSamples (); startSample += blockSize)
    {
        const auto samplesThisBlock = std::min (blockSize, input.getNumSamples () - startSample);

        for (int channel = 0; channel < input.getNumChannels (); ++channel)
        {
            block.clear (channel, 0, blockSize);
            block.copyFrom (channel, 0, input, channel, startSample, samplesThisBlock);
        }

        engine.processBlock (block);

        for (int channel = 0; channel < output.getNumChannels (); ++channel)
            output.copyFrom (channel, startSample, block, channel, 0, samplesThisBlock);
    }

    return output;
}

inline juce::AudioBuffer<float>
processSplitRecombineInBlocks (const juce::AudioBuffer<float>& input, double sampleRate, int blockSize, float cutoffHz)
{
    juce::dsp::LinkwitzRileyFilter<double> filter;
    filter.prepare (
        {sampleRate, static_cast<juce::uint32> (blockSize), static_cast<juce::uint32> (input.getNumChannels ())});
    filter.setCutoffFrequency (static_cast<double> (cutoffHz));

    juce::AudioBuffer<float> output (input.getNumChannels (), input.getNumSamples ());

    for (int startSample = 0; startSample < input.getNumSamples (); startSample += blockSize)
    {
        const auto samplesThisBlock = std::min (blockSize, input.getNumSamples () - startSample);

        for (int sample = 0; sample < samplesThisBlock; ++sample)
        {
            for (int channel = 0; channel < input.getNumChannels (); ++channel)
            {
                double low = 0.0;
                double high = 0.0;
                const auto inputSample = static_cast<double> (input.getSample (channel, startSample + sample));

                filter.processSample (channel, inputSample, low, high);
                output.setSample (channel, startSample + sample, static_cast<float> (low + high));
            }
        }

        filter.snapToZero ();
    }

    return output;
}

} // namespace TestHelpers
