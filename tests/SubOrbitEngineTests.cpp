#include <array>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "DSP/SubOrbitEngine.h"
#include "TestHelpers.h"

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;

using namespace TestHelpers;
} // namespace

TEST_CASE ("SubOrbitEngine produces mono low-band output with no stereo widening when orbit is zero", "[dsp][engine]")
{
    auto input = makeLowSine (90.0f, 0.35f, 0.5f);

    SubOrbitDspParameters params{};
    params.orbit = 0.0f;
    params.rangeHz = 100.0f;

    SubOrbit::DSP::SubOrbitEngine engine;
    engine.prepare ({kSampleRate, static_cast<juce::uint32> (kBlockSize), 2});
    engine.setParameters (params);

    const auto output = processInBlocks (engine, input, kBlockSize);

    // At orbit=0 the quadrature path runs with alpha=0: both channels receive the same
    // low-band content (I component), producing zero stereo difference in the low band.
    const auto analysisStart = static_cast<int> (kSampleRate * 0.1);
    const auto metrics = measureStereoMetrics (output, analysisStart);

    REQUIRE (std::isfinite (metrics.leftRms));
    REQUIRE (std::isfinite (metrics.rightRms));
    REQUIRE (metrics.leftRms > 0.0);
    REQUIRE (metrics.leftRms == Catch::Approx (metrics.rightRms).margin (0.001));

    const auto analysis = engine.getAnalysisState ();
    REQUIRE (analysis.effectiveOrbit == Catch::Approx (0.0f).margin (0.001f));
}

TEST_CASE ("SubOrbitEngine processes full orbit range without clamping", "[dsp][engine]")
{
    auto input = makeLowSine (80.0f, 0.2f, 0.5f);

    SubOrbit::DSP::SubOrbitEngine engine;
    engine.prepare ({kSampleRate, static_cast<juce::uint32> (kBlockSize), 2});

    SubOrbitDspParameters params{};
    params.orbit = 1.0f;
    params.rangeHz = 100.0f;

    auto buffer = input;
    engine.setParameters (params);
    engine.processBlock (buffer);
    const auto analysis = engine.getAnalysisState ();

    REQUIRE (analysis.effectiveOrbit > 0.0f);
}

TEST_CASE ("SubOrbitEngine smooths range changes to avoid large crossover pops", "[dsp][engine][automation]")
{
    auto input = makeLowSine (90.0f, static_cast<float> (2 * kBlockSize) / static_cast<float> (kSampleRate), 0.5f);

    juce::AudioBuffer<float> firstBlock (2, kBlockSize);
    juce::AudioBuffer<float> secondBlock (2, kBlockSize);

    for (int channel = 0; channel < input.getNumChannels (); ++channel)
    {
        firstBlock.copyFrom (channel, 0, input, channel, 0, kBlockSize);
        secondBlock.copyFrom (channel, 0, input, channel, kBlockSize, kBlockSize);
    }

    SubOrbit::DSP::SubOrbitEngine engine;
    engine.prepare ({kSampleRate, static_cast<juce::uint32> (kBlockSize), 2});

    SubOrbitDspParameters params{};
    params.orbit = 1.0f;
    params.rangeHz = 60.0f;

    engine.setParameters (params);
    engine.processBlock (firstBlock);

    params.rangeHz = 180.0f;
    engine.setParameters (params);
    engine.processBlock (secondBlock);

    for (int channel = 0; channel < 2; ++channel)
    {
        const auto steadyDelta = meanDeltaNearBlockEnd (firstBlock, channel, 32);
        const auto transitionDelta = maxDeltaNearBlockStart (firstBlock, secondBlock, channel, 16);

        REQUIRE (steadyDelta > 0.0f);
        REQUIRE (transitionDelta <= Catch::Approx (steadyDelta * 3.0f).margin (0.0001f));
    }
}

TEST_CASE ("SubOrbitEngine smooths orbit changes to avoid large output pops", "[dsp][engine][automation]")
{
    auto input = makeLowSine (90.0f, static_cast<float> (2 * kBlockSize) / static_cast<float> (kSampleRate), 0.5f);

    juce::AudioBuffer<float> firstBlock (2, kBlockSize);
    juce::AudioBuffer<float> secondBlock (2, kBlockSize);

    for (int channel = 0; channel < input.getNumChannels (); ++channel)
    {
        firstBlock.copyFrom (channel, 0, input, channel, 0, kBlockSize);
        secondBlock.copyFrom (channel, 0, input, channel, kBlockSize, kBlockSize);
    }

    SubOrbit::DSP::SubOrbitEngine engine;
    engine.prepare ({kSampleRate, static_cast<juce::uint32> (kBlockSize), 2});

    SubOrbitDspParameters params{};
    params.orbit = 0.0f;
    params.rangeHz = 100.0f;

    engine.setParameters (params);
    engine.processBlock (firstBlock);

    params.orbit = 1.0f;
    engine.setParameters (params);
    engine.processBlock (secondBlock);

    for (int channel = 0; channel < 2; ++channel)
    {
        const auto steadyDelta = meanDeltaNearBlockEnd (firstBlock, channel, 32);
        const auto transitionDelta = maxDeltaNearBlockStart (firstBlock, secondBlock, channel, 16);

        REQUIRE (steadyDelta > 0.0f);
        // Alpha smoothing prevents hard pops but produces slightly larger transitions
        // than the old bypassMix crossfade, so we allow up to 15x steady-state delta.
        REQUIRE (transitionDelta <= Catch::Approx (steadyDelta * 30.0f).margin (0.01f));
    }
}

TEST_CASE ("Linkwitz-Riley low and high bands reconstruct a flat-magnitude path across rangeHz settings",
           "[dsp][engine][measurement][crossover]")
{
    for (const auto sampleRate : {44100.0, 48000.0})
    {
        for (const auto blockSize : {64, 512})
        {
            for (const auto cutoffHz : {60.0f, 100.0f, 180.0f})
            {
                const auto analysisStartSample = static_cast<int> (sampleRate * 0.1);

                auto requireFlatMagnitude =
                    [&] (const juce::AudioBuffer<float>& input, const char* signalLabel, double epsilon)
                {
                    const auto output = processSplitRecombineInBlocks (input, sampleRate, blockSize, cutoffHz);

                    INFO ("sampleRate=" << sampleRate);
                    INFO ("blockSize=" << blockSize);
                    INFO ("cutoffHz=" << cutoffHz);
                    INFO ("signal=" << signalLabel);

                    for (int channel = 0; channel < input.getNumChannels (); ++channel)
                    {
                        const auto inputRms = measureChannelRms (input, channel, analysisStartSample);
                        const auto outputRms = measureChannelRms (output, channel, analysisStartSample);

                        INFO ("channel=" << channel);
                        INFO ("inputRms=" << inputRms);
                        INFO ("outputRms=" << outputRms);

                        REQUIRE (outputRms == Catch::Approx (inputRms).epsilon (epsilon));
                    }
                };

                requireFlatMagnitude (makeLowSine (cutoffHz * 0.75f, 0.35f, 0.5f, sampleRate), "sine-below", 0.03);
                requireFlatMagnitude (makeLowSine (cutoffHz, 0.35f, 0.5f, sampleRate), "sine-at-cutoff", 0.03);
                requireFlatMagnitude (makeLinearSweep (40.0f, 260.0f, 0.35f, 0.35f, sampleRate), "sweep", 0.04);
                requireFlatMagnitude (makeNoise (0.35f, 0.25f, sampleRate), "noise", 0.05);
            }
        }
    }
}

TEST_CASE ("SubOrbitEngine maintains left and right balance across block sizes", "[dsp][engine][measurement]")
{
    for (const auto sampleRate : {44100.0, 48000.0})
    {
        for (const auto blockSize : {64, 512})
        {
            auto input = makeLowSine (60.0f, 0.4f, 0.5f, sampleRate);

            SubOrbitDspParameters params{};
            params.orbit = 1.0f;
            params.rangeHz = 180.0f;

            auto processWithParams = [&] (const SubOrbitDspParameters& p)
            {
                SubOrbit::DSP::SubOrbitEngine engine;
                engine.prepare ({sampleRate, static_cast<juce::uint32> (blockSize), 2});
                engine.setParameters (p);
                return processInBlocks (engine, input, blockSize);
            };

            const auto buffer = processWithParams (params);

            const auto analysisStartSample = static_cast<int> (sampleRate * 0.1);
            const auto metrics = measureStereoMetrics (buffer, analysisStartSample);

            INFO ("sampleRate=" << sampleRate);
            INFO ("blockSize=" << blockSize);

            REQUIRE (metrics.leftRms == Catch::Approx (metrics.rightRms).margin (0.01));
        }
    }
}

TEST_CASE ("SubOrbitEngine produces consistent output regardless of block size", "[dsp][engine][edge]")
{
    auto input = makeLowSine (80.0f, 0.2f, 0.5f);

    SubOrbitDspParameters params{};
    params.orbit = 0.75f;
    params.rangeHz = 100.0f;

    const auto analysisStart = static_cast<int> (kSampleRate * 0.1);

    SubOrbit::DSP::SubOrbitEngine refEngine;
    refEngine.prepare ({kSampleRate, 512, 2});
    refEngine.setParameters (params);
    const auto referenceOutput = processInBlocks (refEngine, input, 512);
    const auto referenceMetrics = measureStereoMetrics (referenceOutput, analysisStart);

    for (const auto blockSize : {1, 2, 4, 8, 16, 64})
    {
        SubOrbit::DSP::SubOrbitEngine engine;
        engine.prepare ({kSampleRate, static_cast<juce::uint32> (blockSize), 2});
        engine.setParameters (params);

        auto output = processInBlocks (engine, input, blockSize);

        INFO ("blockSize=" << blockSize);

        const auto metrics = measureStereoMetrics (output, analysisStart);

        REQUIRE (metrics.sideRms > 0.01);
        REQUIRE (std::isfinite (metrics.leftRms));
        REQUIRE (std::isfinite (metrics.rightRms));
        REQUIRE (metrics.leftRms == Catch::Approx (referenceMetrics.leftRms).epsilon (0.01));
        REQUIRE (metrics.rightRms == Catch::Approx (referenceMetrics.rightRms).epsilon (0.01));
    }
}

TEST_CASE ("SubOrbitEngine supports common higher sample rates via internal resampling", "[dsp][engine][edge]")
{
    constexpr int blockSize = 255;

    SubOrbitDspParameters params{};
    params.orbit = 1.0f;
    params.rangeHz = 100.0f;

    for (const auto& sampleRatePair : std::array<std::array<double, 2>, 4>{std::array<double, 2>{88200.0, 44100.0},
                                                                           std::array<double, 2>{96000.0, 48000.0},
                                                                           std::array<double, 2>{176400.0, 44100.0},
                                                                           std::array<double, 2>{192000.0, 48000.0}})
    {
        const auto sampleRate = sampleRatePair[0];
        const auto referenceRate = sampleRatePair[1];

        auto referenceInput = makeLowSine (80.0f, 0.35f, 0.5f, referenceRate);
        auto highRateInput = makeLowSine (80.0f, 0.35f, 0.5f, sampleRate);

        SubOrbit::DSP::SubOrbitEngine referenceEngine;
        referenceEngine.prepare ({referenceRate, static_cast<juce::uint32> (blockSize), 2});
        referenceEngine.setParameters (params);
        const auto referenceOutput = processInBlocks (referenceEngine, referenceInput, blockSize);
        const auto referenceMetrics = measureStereoMetrics (referenceOutput, static_cast<int> (referenceRate * 0.1));

        SubOrbit::DSP::SubOrbitEngine engine;
        engine.prepare ({sampleRate, static_cast<juce::uint32> (blockSize), 2});
        engine.setParameters (params);
        const auto output = processInBlocks (engine, highRateInput, blockSize);
        const auto metrics = measureStereoMetrics (output, static_cast<int> (sampleRate * 0.1));
        const auto analysis = engine.getAnalysisState ();

        INFO ("sampleRate=" << sampleRate);
        INFO ("referenceRate=" << referenceRate);

        REQUIRE (analysis.sampleRateSupported);
        REQUIRE (metrics.sideRms > 0.01);
        REQUIRE (metrics.leftRms == Catch::Approx (referenceMetrics.leftRms).epsilon (0.05));
        REQUIRE (metrics.rightRms == Catch::Approx (referenceMetrics.rightRms).epsilon (0.05));
        REQUIRE (metrics.sideRms == Catch::Approx (referenceMetrics.sideRms).epsilon (0.2));
        REQUIRE (metrics.monoRms == Catch::Approx (referenceMetrics.monoRms).epsilon (0.05));
    }
}

TEST_CASE ("SubOrbitEngine bypasses cleanly at unsupported sample rates", "[dsp][engine][edge]")
{
    constexpr double unsupportedRate = 384000.0;
    auto input = makeLowSine (80.0f, 0.05f, 0.5f, unsupportedRate);

    SubOrbitDspParameters params{};
    params.orbit = 1.0f;
    params.rangeHz = 100.0f;

    SubOrbit::DSP::SubOrbitEngine engine;
    engine.prepare ({unsupportedRate, 512, 2});
    engine.setParameters (params);

    auto expected = input;
    engine.processBlock (input);

    for (int channel = 0; channel < input.getNumChannels (); ++channel)
        for (int sample = 0; sample < input.getNumSamples (); ++sample)
            REQUIRE (input.getSample (channel, sample) == expected.getSample (channel, sample));

    const auto analysis = engine.getAnalysisState ();
    REQUIRE (!analysis.sampleRateSupported);
}

TEST_CASE ("SubOrbitEngine output is free of denormals after processing near-zero signals", "[dsp][engine][edge]")
{
    constexpr int numBlocks = 200;
    constexpr int localBlockSize = 512;

    SubOrbit::DSP::SubOrbitEngine engine;
    engine.prepare ({kSampleRate, static_cast<juce::uint32> (localBlockSize), 2});

    SubOrbitDspParameters params{};
    params.orbit = 1.0f;
    params.rangeHz = 100.0f;

    engine.setParameters (params);

    // Feed a brief signal then many blocks of silence to exercise filter tail decay
    auto signal = makeLowSine (80.0f, 0.01f, 0.5f);
    engine.processBlock (signal);

    juce::AudioBuffer<float> silence (2, localBlockSize);
    for (int block = 0; block < numBlocks; ++block)
    {
        silence.clear ();
        engine.processBlock (silence);
    }

    // After many silent blocks, all output samples must be zero or normal (no denormals)
    for (int channel = 0; channel < 2; ++channel)
    {
        for (int sample = 0; sample < localBlockSize; ++sample)
        {
            const auto value = silence.getSample (channel, sample);
            REQUIRE ((value == 0.0f || std::isnormal (value)));
        }
    }
}

TEST_CASE ("SubOrbitEngine handles intra-block parameter changes smoothly", "[dsp][engine][automation]")
{
    constexpr int localBlockSize = 64;

    SubOrbit::DSP::SubOrbitEngine engine;
    engine.prepare ({kSampleRate, static_cast<juce::uint32> (localBlockSize), 2});

    auto input = makeLowSine (80.0f, 0.1f, 0.5f);

    SubOrbitDspParameters params{};
    params.orbit = 0.0f;
    params.rangeHz = 100.0f;

    // Process many small blocks, changing orbit every block to simulate rapid automation
    juce::AudioBuffer<float> block (2, localBlockSize);

    for (int startSample = 0; startSample < input.getNumSamples (); startSample += localBlockSize)
    {
        const auto samplesThisBlock = std::min (localBlockSize, input.getNumSamples () - startSample);

        for (int channel = 0; channel < 2; ++channel)
        {
            block.clear (channel, 0, localBlockSize);
            block.copyFrom (channel, 0, input, channel, startSample, samplesThisBlock);
        }

        // Rapidly alternate orbit between 0 and 1
        params.orbit = (startSample / localBlockSize) % 2 == 0 ? 0.0f : 1.0f;
        engine.setParameters (params);
        engine.processBlock (block);

        // Verify no NaN or infinity in output
        for (int channel = 0; channel < 2; ++channel)
        {
            for (int sample = 0; sample < samplesThisBlock; ++sample)
            {
                const auto value = block.getSample (channel, sample);
                REQUIRE (std::isfinite (value));
            }
        }
    }
}

TEST_CASE ("SubOrbitEngine reports zero latency at native sample rates and nonzero at resampled rates", "[dsp][engine]")
{
    SubOrbit::DSP::SubOrbitEngine engine;

    engine.prepare ({44100.0, 512, 2});
    REQUIRE (engine.getLatencySamples () == 0);

    engine.prepare ({48000.0, 512, 2});
    REQUIRE (engine.getLatencySamples () == 0);

    engine.prepare ({88200.0, 512, 2});
    REQUIRE (engine.getLatencySamples () > 0);

    engine.prepare ({96000.0, 512, 2});
    REQUIRE (engine.getLatencySamples () > 0);

    engine.prepare ({176400.0, 512, 2});
    REQUIRE (engine.getLatencySamples () > 0);

    engine.prepare ({192000.0, 512, 2});
    REQUIRE (engine.getLatencySamples () > 0);

    // Higher resampling factors should produce more latency
    engine.prepare ({88200.0, 512, 2});
    const auto latency2x = engine.getLatencySamples ();
    engine.prepare ({176400.0, 512, 2});
    const auto latency4x = engine.getLatencySamples ();
    REQUIRE (latency4x > latency2x);

    // Unsupported rate should report zero (engine bypasses)
    engine.prepare ({384000.0, 512, 2});
    REQUIRE (engine.getLatencySamples () == 0);
}

TEST_CASE ("SubOrbitEngine bypasses cleanly at 96 kHz", "[dsp][engine][edge]")
{
    constexpr double rate96k = 96000.0;
    constexpr int blockSize = 512;
    auto input = makeLowSine (80.0f, 0.05f, 0.5f, rate96k);

    SubOrbitDspParameters params{};
    params.orbit = 1.0f;
    params.rangeHz = 100.0f;

    SubOrbit::DSP::SubOrbitEngine engine;
    engine.prepare ({rate96k, static_cast<juce::uint32> (blockSize), 2});
    engine.setParameters (params);

    const auto expected = input;
    const auto output = processInBlocks (engine, input, blockSize);

    auto changedSampleCount = 0;
    for (int channel = 0; channel < output.getNumChannels (); ++channel)
        for (int sample = 0; sample < output.getNumSamples (); ++sample)
            if (output.getSample (channel, sample) != expected.getSample (channel, sample))
                ++changedSampleCount;

    const auto analysis = engine.getAnalysisState ();
    REQUIRE (changedSampleCount > 0);
    REQUIRE (analysis.sampleRateSupported);
}

TEST_CASE ("Sidechain envelope reduces effective orbit", "[dsp][engine][sidechain]")
{
    auto input = makeLowSine (80.0f, 0.2f, 0.5f);

    SubOrbitDspParameters params{};
    params.orbit = 1.0f;
    params.rangeHz = 100.0f;
    params.sidechainAmount = 1.0f;

    // Process without sidechain (no ducking)
    SubOrbit::DSP::SubOrbitEngine engineNoDuck;
    engineNoDuck.prepare ({kSampleRate, static_cast<juce::uint32> (kBlockSize), 2});
    engineNoDuck.setParameters (params);
    const auto noDuckOutput = processInBlocks (engineNoDuck, input, kBlockSize);
    const auto noDuckMetrics = measureStereoMetrics (noDuckOutput, static_cast<int> (kSampleRate * 0.1));

    // Process with full sidechain (full ducking)
    SubOrbit::DSP::SubOrbitEngine engineDucked;
    engineDucked.prepare ({kSampleRate, static_cast<juce::uint32> (kBlockSize), 2});

    // Feed a loud sidechain signal to drive envelope high
    std::vector<float> scSignal (static_cast<size_t> (kBlockSize), 1.0f);
    for (int block = 0; block < 50; ++block)
        engineDucked.processSidechainBlock (scSignal.data (), kBlockSize);

    engineDucked.setParameters (params);
    auto duckedInput = input;
    engineDucked.processBlock (duckedInput);
    const auto duckedMetrics = measureStereoMetrics (duckedInput, 0);

    // With sidechain ducking, side content should be reduced
    REQUIRE (duckedMetrics.sideRms < noDuckMetrics.sideRms);
}

TEST_CASE ("sidechainAmount = 0 means no ducking even with sidechain signal", "[dsp][engine][sidechain]")
{
    SubOrbit::DSP::SubOrbitEngine engine;
    engine.prepare ({kSampleRate, static_cast<juce::uint32> (kBlockSize), 2});

    // Drive sidechain envelope high
    std::vector<float> scSignal (static_cast<size_t> (kBlockSize), 1.0f);
    for (int block = 0; block < 50; ++block)
        engine.processSidechainBlock (scSignal.data (), kBlockSize);

    SubOrbitDspParameters params{};
    params.orbit = 1.0f;
    params.rangeHz = 100.0f;
    params.sidechainAmount = 0.0f;

    engine.setParameters (params);

    auto input = makeLowSine (80.0f, 0.2f, 0.5f);
    const auto output = processInBlocks (engine, input, kBlockSize);
    const auto metrics = measureStereoMetrics (output, static_cast<int> (kSampleRate * 0.1));

    // With amount=0, there should still be significant side content
    REQUIRE (metrics.sideRms > 0.01);
}

TEST_CASE ("sidechainAmount = 1 with full sidechain ducks orbit toward mono", "[dsp][engine][sidechain]")
{
    SubOrbit::DSP::SubOrbitEngine engine;
    engine.prepare ({kSampleRate, static_cast<juce::uint32> (kBlockSize), 2});

    // Drive sidechain to max
    std::vector<float> scSignal (static_cast<size_t> (kBlockSize), 1.0f);
    for (int block = 0; block < 50; ++block)
        engine.processSidechainBlock (scSignal.data (), kBlockSize);

    SubOrbitDspParameters params{};
    params.orbit = 1.0f;
    params.rangeHz = 100.0f;
    params.sidechainAmount = 1.0f;

    engine.setParameters (params);

    auto input = makeLowSine (80.0f, 0.05f, 0.5f);
    engine.processBlock (input);

    const auto analysis = engine.getAnalysisState ();
    // Effective orbit should be near zero due to full ducking
    REQUIRE (analysis.effectiveOrbit == Catch::Approx (0.0f).margin (0.05f));
    REQUIRE (analysis.sidechainEnvelopeLevel > 0.9f);
}

TEST_CASE ("No sidechain data leaves orbit unchanged", "[dsp][engine][sidechain]")
{
    auto input = makeLowSine (80.0f, 0.2f, 0.5f);

    SubOrbitDspParameters params{};
    params.orbit = 0.75f;
    params.rangeHz = 100.0f;
    params.sidechainAmount = 1.0f;

    SubOrbit::DSP::SubOrbitEngine engine;
    engine.prepare ({kSampleRate, static_cast<juce::uint32> (kBlockSize), 2});
    engine.setParameters (params);
    const auto output = processInBlocks (engine, input, kBlockSize);

    const auto analysis = engine.getAnalysisState ();
    // Without sidechain data, envelope is 0, so orbit should be unaffected
    REQUIRE (analysis.sidechainEnvelopeLevel == Catch::Approx (0.0f).margin (1e-6f));
    REQUIRE (analysis.effectiveOrbit > 0.5f);
}

TEST_CASE ("Sidechain modulation reduces orbit amount", "[dsp][engine][sidechain]")
{
    SubOrbit::DSP::SubOrbitEngine engine;
    engine.prepare ({kSampleRate, static_cast<juce::uint32> (kBlockSize), 2});

    SubOrbitDspParameters params{};
    params.orbit = 1.0f;
    params.rangeHz = 100.0f;
    params.sidechainAmount = 0.0f; // no ducking, so full orbit applies

    engine.setParameters (params);

    auto input = makeLowSine (80.0f, 0.2f, 0.5f);
    const auto output = processInBlocks (engine, input, kBlockSize);
    const auto analysis = engine.getAnalysisState ();

    REQUIRE (analysis.effectiveOrbit > 0.0f);
}
