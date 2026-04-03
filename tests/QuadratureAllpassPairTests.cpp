#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

#include "DSP/QuadratureAllpassPair.h"

namespace
{
constexpr double kWarmupSeconds = 1.0;
constexpr double kMeasurementSeconds = 0.5;
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

struct SineFit
{
    double amplitude = 0.0;
    double phaseRadians = 0.0;
};

SineFit fitSineComponent (const std::vector<double>& signal, double frequencyHz, double sampleRate)
{
    double sumSin = 0.0;
    double sumCos = 0.0;

    for (size_t sample = 0; sample < signal.size (); ++sample)
    {
        const auto phase = kTwoPi * frequencyHz * static_cast<double> (sample) / sampleRate;
        sumSin += signal[sample] * std::sin (phase);
        sumCos += signal[sample] * std::cos (phase);
    }

    const auto scale = 2.0 / static_cast<double> (signal.size ());
    const auto sinComponent = sumSin * scale;
    const auto cosComponent = sumCos * scale;

    return {std::sqrt (sinComponent * sinComponent + cosComponent * cosComponent),
            std::atan2 (cosComponent, sinComponent)};
}

double wrapPhaseDifferenceDegrees (double radians)
{
    auto wrapped = std::remainder (radians, kTwoPi);
    return wrapped * 180.0 / kPi;
}
} // namespace

TEST_CASE ("QuadratureAllpassPair stays near 90 degrees across the Sub Orbit band", "[dsp][quadrature]")
{
    for (const auto sampleRate : {44100.0, 48000.0})
    {
        SubOrbit::DSP::QuadratureAllpassPair pair;
        pair.prepare (sampleRate);

        const auto warmupSamples = static_cast<int> (sampleRate * kWarmupSeconds);
        const auto measurementSamples = static_cast<int> (sampleRate * kMeasurementSeconds);

        for (const auto frequencyHz : {60.0, 80.0, 120.0, 180.0})
        {
            pair.reset ();

            for (int sample = 0; sample < warmupSamples; ++sample)
            {
                const auto phase = kTwoPi * frequencyHz * static_cast<double> (sample) / sampleRate;
                const auto input = std::sin (phase);
                static_cast<void> (pair.processSample (input));
            }

            std::vector<double> iSamples;
            std::vector<double> qSamples;
            iSamples.reserve (measurementSamples);
            qSamples.reserve (measurementSamples);

            for (int sample = 0; sample < measurementSamples; ++sample)
            {
                const auto phase = kTwoPi * frequencyHz * static_cast<double> (sample) / sampleRate;
                const auto input = std::sin (phase);
                const auto [i, q] = pair.processSample (input);
                iSamples.push_back (i);
                qSamples.push_back (q);
            }

            const auto iFit = fitSineComponent (iSamples, frequencyHz, sampleRate);
            const auto qFit = fitSineComponent (qSamples, frequencyHz, sampleRate);
            const auto phaseDifferenceDegrees =
                std::abs (wrapPhaseDifferenceDegrees (qFit.phaseRadians - iFit.phaseRadians));

            INFO ("sampleRate=" << sampleRate);
            INFO ("frequencyHz=" << frequencyHz);
            INFO ("phaseDifferenceDegrees=" << phaseDifferenceDegrees);
            INFO ("iAmplitude=" << iFit.amplitude);
            INFO ("qAmplitude=" << qFit.amplitude);

            REQUIRE (phaseDifferenceDegrees == Catch::Approx (90.0).margin (3.0));
            REQUIRE (qFit.amplitude == Catch::Approx (iFit.amplitude).margin (0.03));
        }
    }
}
