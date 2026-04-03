#pragma once

#include <atomic>

#include <juce_dsp/juce_dsp.h>

namespace SubOrbit::DSP
{

class SidechainEnvelopeFollower
{
  public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        juce::dsp::ProcessSpec monoSpec{spec.sampleRate, spec.maximumBlockSize, 1};
        ballistics.prepare (monoSpec);
        ballistics.setAttackTime (attackMs);
        ballistics.setReleaseTime (releaseMs);
        ballistics.setLevelCalculationType (juce::dsp::BallisticsFilterLevelCalculationType::peak);
        reset ();
    }

    void reset ()
    {
        ballistics.reset ();
        envelopeLevel.store (0.0f, std::memory_order_relaxed);
    }

    void processBlock (const float* sidechainData, int numSamples)
    {
        float level = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            level = ballistics.processSample (0, std::abs (sidechainData[i]));
        }

        envelopeLevel.store (juce::jlimit (0.0f, 1.0f, level), std::memory_order_relaxed);
    }

    [[nodiscard]] float getEnvelopeLevel () const noexcept
    {
        return envelopeLevel.load (std::memory_order_relaxed);
    }

    void setAttackTime (float ms)
    {
        attackMs = ms;
        ballistics.setAttackTime (ms);
    }

    void setReleaseTime (float ms)
    {
        releaseMs = ms;
        ballistics.setReleaseTime (ms);
    }

  private:
    float attackMs = 10.0f;
    float releaseMs = 300.0f;

    juce::dsp::BallisticsFilter<float> ballistics;
    std::atomic<float> envelopeLevel{0.0f};
};

} // namespace SubOrbit::DSP
