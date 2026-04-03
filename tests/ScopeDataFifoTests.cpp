#include <catch2/catch_test_macros.hpp>

#include "ScopeDataFifo.h"

TEST_CASE ("ScopeDataFifo push and pull round-trips stereo samples", "[fifo]")
{
    ScopeDataFifo fifo;
    constexpr int numSamples = 128;
    float left[numSamples];
    float right[numSamples];

    for (int i = 0; i < numSamples; ++i)
    {
        left[i] = static_cast<float> (i);
        right[i] = static_cast<float> (-i);
    }

    fifo.push (left, right, numSamples);

    std::array<StereoScopeSample, 256> dest{};
    const auto pulled = fifo.pull (dest.data (), static_cast<int> (dest.size ()));

    REQUIRE (pulled == numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        REQUIRE (dest[static_cast<size_t> (i)].left == static_cast<float> (i));
        REQUIRE (dest[static_cast<size_t> (i)].right == static_cast<float> (-i));
    }
}

TEST_CASE ("ScopeDataFifo pull returns zero when empty", "[fifo]")
{
    ScopeDataFifo fifo;
    std::array<StereoScopeSample, 64> dest{};
    const auto pulled = fifo.pull (dest.data (), static_cast<int> (dest.size ()));
    REQUIRE (pulled == 0);
}

TEST_CASE ("ScopeDataFifo drops samples gracefully when full", "[fifo][edge]")
{
    ScopeDataFifo fifo;
    constexpr int chunkSize = 4096;
    float left[chunkSize];
    float right[chunkSize];

    for (int i = 0; i < chunkSize; ++i)
    {
        left[i] = 1.0f;
        right[i] = 1.0f;
    }

    // AbstractFifo reserves one slot internally, so usable capacity is kCapacity - 1.
    constexpr int usableCapacity = ScopeDataFifo::kCapacity - 1;

    // Fill the FIFO completely
    for (int i = 0; i < usableCapacity / chunkSize; ++i)
        fifo.push (left, right, chunkSize);

    // Push the remaining samples to fully saturate
    const auto remainder = usableCapacity % chunkSize;
    if (remainder > 0)
        fifo.push (left, right, remainder);

    // This push should be silently dropped since the FIFO is full
    float overflowLeft[64];
    float overflowRight[64];

    for (int i = 0; i < 64; ++i)
    {
        overflowLeft[i] = 99.0f;
        overflowRight[i] = 99.0f;
    }

    fifo.push (overflowLeft, overflowRight, 64);

    // Drain everything — should get exactly usableCapacity samples, none with 99.0f
    std::array<StereoScopeSample, ScopeDataFifo::kCapacity> dest{};
    const auto pulled = fifo.pull (dest.data (), ScopeDataFifo::kCapacity);

    REQUIRE (pulled == usableCapacity);

    for (int i = 0; i < pulled; ++i)
    {
        REQUIRE (dest[static_cast<size_t> (i)].left == 1.0f);
        REQUIRE (dest[static_cast<size_t> (i)].right == 1.0f);
    }
}
