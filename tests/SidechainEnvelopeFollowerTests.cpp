#include <cmath>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "DSP/SidechainEnvelopeFollower.h"

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;

juce::dsp::ProcessSpec makeSpec (double sampleRate = kSampleRate, int blockSize = kBlockSize)
{
    return {sampleRate, static_cast<juce::uint32> (blockSize), 1};
}
} // namespace

TEST_CASE ("SidechainEnvelopeFollower stays at zero with silence", "[dsp][sidechain]")
{
    SubOrbit::DSP::SidechainEnvelopeFollower follower;
    follower.prepare (makeSpec ());

    std::vector<float> silence (static_cast<size_t> (kBlockSize), 0.0f);

    for (int block = 0; block < 10; ++block)
        follower.processBlock (silence.data (), kBlockSize);

    REQUIRE (follower.getEnvelopeLevel () == Catch::Approx (0.0f).margin (1e-6f));
}

TEST_CASE ("SidechainEnvelopeFollower approaches 1.0 with constant full-scale signal", "[dsp][sidechain]")
{
    SubOrbit::DSP::SidechainEnvelopeFollower follower;
    follower.prepare (makeSpec ());

    std::vector<float> fullScale (static_cast<size_t> (kBlockSize), 1.0f);

    // Process enough blocks for the envelope to converge
    for (int block = 0; block < 50; ++block)
        follower.processBlock (fullScale.data (), kBlockSize);

    REQUIRE (follower.getEnvelopeLevel () == Catch::Approx (1.0f).margin (0.01f));
}

TEST_CASE ("SidechainEnvelopeFollower rises within ~10ms on impulse", "[dsp][sidechain]")
{
    SubOrbit::DSP::SidechainEnvelopeFollower follower;
    follower.prepare (makeSpec ());

    // Create an impulse: a block of 1.0 signal
    const auto attackSamples = static_cast<int> (kSampleRate * 0.010); // 10ms
    std::vector<float> impulse (static_cast<size_t> (attackSamples), 1.0f);

    follower.processBlock (impulse.data (), attackSamples);
    const auto levelAfterAttack = follower.getEnvelopeLevel ();

    // After 10ms of full-scale signal, envelope should be well above 0.5
    REQUIRE (levelAfterAttack > 0.5f);
}

TEST_CASE ("SidechainEnvelopeFollower decays over ~300ms after signal stops", "[dsp][sidechain]")
{
    SubOrbit::DSP::SidechainEnvelopeFollower follower;
    follower.prepare (makeSpec ());

    // First drive to near 1.0
    std::vector<float> fullScale (static_cast<size_t> (kBlockSize), 1.0f);
    for (int block = 0; block < 50; ++block)
        follower.processBlock (fullScale.data (), kBlockSize);

    const auto peakLevel = follower.getEnvelopeLevel ();
    REQUIRE (peakLevel > 0.9f);

    // Now feed silence for 300ms
    const auto releaseSamples = static_cast<int> (kSampleRate * 0.300);
    std::vector<float> silence (static_cast<size_t> (releaseSamples), 0.0f);
    follower.processBlock (silence.data (), releaseSamples);

    const auto levelAfterRelease = follower.getEnvelopeLevel ();

    // After one release time constant, envelope should have decayed significantly
    REQUIRE (levelAfterRelease < peakLevel * 0.5f);
}

TEST_CASE ("SidechainEnvelopeFollower is block-size independent", "[dsp][sidechain]")
{
    // Process the same signal with different block sizes and compare final envelope
    std::vector<float> signal (4800, 0.0f);
    for (size_t i = 0; i < signal.size (); ++i)
        signal[i] = static_cast<float> (std::sin (2.0 * 3.14159265 * 100.0 * static_cast<double> (i) / kSampleRate));

    float referenceLevel = 0.0f;

    for (const auto blockSize : {1, 64, 512})
    {
        SubOrbit::DSP::SidechainEnvelopeFollower follower;
        follower.prepare (makeSpec (kSampleRate, blockSize));

        for (size_t offset = 0; offset < signal.size (); offset += static_cast<size_t> (blockSize))
        {
            const auto samplesThisBlock =
                static_cast<int> (std::min (static_cast<size_t> (blockSize), signal.size () - offset));
            follower.processBlock (signal.data () + offset, samplesThisBlock);
        }

        const auto level = follower.getEnvelopeLevel ();

        if (blockSize == 1)
            referenceLevel = level;
        else
            REQUIRE (level == Catch::Approx (referenceLevel).epsilon (0.05));
    }
}

TEST_CASE ("SidechainEnvelopeFollower is sample-rate independent", "[dsp][sidechain]")
{
    // At different sample rates, a 100ms burst of 1.0 should produce similar envelope levels
    float referenceLevel = 0.0f;

    for (const auto sampleRate : {44100.0, 48000.0, 96000.0})
    {
        SubOrbit::DSP::SidechainEnvelopeFollower follower;
        follower.prepare (makeSpec (sampleRate, 512));

        // 100ms of full-scale signal
        const auto burstSamples = static_cast<int> (sampleRate * 0.1);
        std::vector<float> burst (static_cast<size_t> (burstSamples), 1.0f);

        for (size_t offset = 0; offset < burst.size (); offset += 512)
        {
            const auto samplesThisBlock =
                static_cast<int> (std::min (static_cast<size_t> (512), burst.size () - offset));
            follower.processBlock (burst.data () + offset, samplesThisBlock);
        }

        const auto level = follower.getEnvelopeLevel ();

        if (sampleRate == 44100.0)
            referenceLevel = level;
        else
            REQUIRE (level == Catch::Approx (referenceLevel).epsilon (0.1));
    }
}

TEST_CASE ("SidechainEnvelopeFollower reset clears state", "[dsp][sidechain]")
{
    SubOrbit::DSP::SidechainEnvelopeFollower follower;
    follower.prepare (makeSpec ());

    std::vector<float> fullScale (static_cast<size_t> (kBlockSize), 1.0f);
    for (int block = 0; block < 50; ++block)
        follower.processBlock (fullScale.data (), kBlockSize);

    REQUIRE (follower.getEnvelopeLevel () > 0.9f);

    follower.reset ();
    REQUIRE (follower.getEnvelopeLevel () == Catch::Approx (0.0f).margin (1e-6f));
}
