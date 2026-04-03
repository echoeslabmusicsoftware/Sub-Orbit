#pragma once

#include <array>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

struct StereoScopeSample
{
    float left = 0.0f;
    float right = 0.0f;
};

class ScopeDataFifo
{
  public:
    static constexpr int kCapacity = 16384;

    ScopeDataFifo () : abstractFifo (kCapacity)
    {
    }

    void push (const float* left, const float* right, int numSamples) noexcept
    {
        const auto toWrite = juce::jmin (numSamples, abstractFifo.getFreeSpace ());
        if (toWrite == 0)
            return;

        const auto scope = abstractFifo.write (toWrite);

        for (int index = 0; index < scope.blockSize1; ++index)
            data[static_cast<size_t> (scope.startIndex1 + index)] = {left[index], right[index]};

        for (int index = 0; index < scope.blockSize2; ++index)
            data[static_cast<size_t> (scope.startIndex2 + index)] = {left[scope.blockSize1 + index],
                                                                     right[scope.blockSize1 + index]};
    }

    int pull (StereoScopeSample* dest, int maxSamples) noexcept
    {
        const auto available = juce::jmin (maxSamples, abstractFifo.getNumReady ());
        if (available == 0)
            return 0;

        const auto scope = abstractFifo.read (available);

        for (int index = 0; index < scope.blockSize1; ++index)
            dest[index] = data[static_cast<size_t> (scope.startIndex1 + index)];

        for (int index = 0; index < scope.blockSize2; ++index)
            dest[scope.blockSize1 + index] = data[static_cast<size_t> (scope.startIndex2 + index)];

        return available;
    }

  private:
    juce::AbstractFifo abstractFifo;
    std::array<StereoScopeSample, kCapacity> data{};
};
