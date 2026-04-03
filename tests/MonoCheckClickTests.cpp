#include <cmath>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <juce_audio_processors/juce_audio_processors.h>

#ifndef JucePlugin_Name
#define JucePlugin_Name "SUB ORBIT"
#endif

#include "PluginProcessor.h"

namespace
{
float maxSampleDelta (const juce::AudioBuffer<float>& buffer, int channel, float previousLastSample)
{
    float maxDelta = std::abs (buffer.getSample (channel, 0) - previousLastSample);

    for (int sample = 1; sample < buffer.getNumSamples (); ++sample)
    {
        const auto delta = std::abs (buffer.getSample (channel, sample) - buffer.getSample (channel, sample - 1));
        maxDelta = std::max (maxDelta, delta);
    }

    return maxDelta;
}

juce::AudioBuffer<float>
makeStereoOppositePolarity (float frequency, float amplitude, double sampleRate, int numSamples, int sampleOffset)
{
    juce::AudioBuffer<float> block (2, numSamples);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto phase = juce::MathConstants<double>::twoPi * static_cast<double> (frequency) *
                           static_cast<double> (sample + sampleOffset) / sampleRate;
        const auto value = amplitude * static_cast<float> (std::sin (phase));
        block.setSample (0, sample, value);
        block.setSample (1, sample, -value);
    }

    return block;
}
} // namespace

TEST_CASE ("processBlockBypassed applies mono check when active", "[processor][audio]")
{
    SubOrbitAudioProcessor processor;
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;
    processor.prepareToPlay (sampleRate, blockSize);
    processor.setMonoCheckActive (true);

    // Process enough blocks for the crossfade ramp to complete (~5ms = 240 samples at 48kHz).
    juce::MidiBuffer midi;
    juce::AudioBuffer<float> rampBlock (2, blockSize);
    rampBlock.clear ();
    processor.processBlockBypassed (rampBlock, midi);

    // Now the ramp has settled — a fully out-of-phase signal should sum to zero.
    juce::AudioBuffer<float> buffer (2, 256);
    for (int sample = 0; sample < 256; ++sample)
    {
        buffer.setSample (0, sample, 1.0f);
        buffer.setSample (1, sample, -1.0f);
    }

    processor.processBlockBypassed (buffer, midi);

    REQUIRE (buffer.getSample (0, 0) == Catch::Approx (0.0f).margin (1.0e-6f));
    REQUIRE (buffer.getSample (1, 0) == Catch::Approx (0.0f).margin (1.0e-6f));
}

TEST_CASE ("Mono check toggle produces no click (smooth crossfade)", "[processor][audio]")
{
    // A click manifests as a large sample-to-sample discontinuity at the moment
    // the mono-check state changes.  This test feeds a wide stereo signal (left
    // and right are opposite-polarity sines), toggles mono check between blocks,
    // and asserts that the maximum per-sample delta never exceeds what the sine
    // alone would produce — i.e. the transition is smooth, not instantaneous.

    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr float frequency = 100.0f;
    constexpr float amplitude = 0.5f;

    SubOrbitAudioProcessor processor;
    processor.prepareToPlay (sampleRate, blockSize);

    // Maximum delta a 100 Hz sine at 0.5 amplitude can produce between two
    // consecutive samples at 48 kHz ≈ 0.0065.  A generous 4x multiplier gives
    // headroom for the ramp blending while still catching an abrupt click.
    const auto sineMaxDelta =
        static_cast<float> (amplitude * 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate);
    const auto clickThreshold = sineMaxDelta * 4.0f;

    juce::MidiBuffer midi;
    int sampleOffset = 0;

    // Establish steady state with mono check OFF.
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;

    for (int i = 0; i < 4; ++i)
    {
        auto block = makeStereoOppositePolarity (frequency, amplitude, sampleRate, blockSize, sampleOffset);
        processor.processBlock (block, midi);
        lastSampleL = block.getSample (0, blockSize - 1);
        lastSampleR = block.getSample (1, blockSize - 1);
        sampleOffset += blockSize;
    }

    // Toggle mono check ON — this is where a click would appear without crossfading.
    processor.setMonoCheckActive (true);

    SECTION ("engaging mono check produces no click")
    {
        auto block = makeStereoOppositePolarity (frequency, amplitude, sampleRate, blockSize, sampleOffset);
        processor.processBlock (block, midi);

        const auto deltaL = maxSampleDelta (block, 0, lastSampleL);
        const auto deltaR = maxSampleDelta (block, 1, lastSampleR);

        INFO ("Max delta L at mono-on transition: " << deltaL << " (threshold: " << clickThreshold << ")");
        INFO ("Max delta R at mono-on transition: " << deltaR << " (threshold: " << clickThreshold << ")");
        REQUIRE (deltaL < clickThreshold);
        REQUIRE (deltaR < clickThreshold);
    }

    SECTION ("disengaging mono check produces no click")
    {
        // Let the ramp fully engage mono.
        for (int i = 0; i < 4; ++i)
        {
            auto block = makeStereoOppositePolarity (frequency, amplitude, sampleRate, blockSize, sampleOffset);
            processor.processBlock (block, midi);
            lastSampleL = block.getSample (0, blockSize - 1);
            lastSampleR = block.getSample (1, blockSize - 1);
            sampleOffset += blockSize;
        }

        // Toggle mono check OFF.
        processor.setMonoCheckActive (false);

        auto block = makeStereoOppositePolarity (frequency, amplitude, sampleRate, blockSize, sampleOffset);
        processor.processBlock (block, midi);

        const auto deltaL = maxSampleDelta (block, 0, lastSampleL);
        const auto deltaR = maxSampleDelta (block, 1, lastSampleR);

        INFO ("Max delta L at mono-off transition: " << deltaL << " (threshold: " << clickThreshold << ")");
        INFO ("Max delta R at mono-off transition: " << deltaR << " (threshold: " << clickThreshold << ")");
        REQUIRE (deltaL < clickThreshold);
        REQUIRE (deltaR < clickThreshold);
    }
}
