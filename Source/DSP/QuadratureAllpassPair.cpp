#include "QuadratureAllpassPair.h"

#include <array>

#include <juce_core/juce_core.h>

namespace SubOrbit::DSP
{

void QuadratureAllpassPair::prepare ([[maybe_unused]] double newSampleRate)
{
    // Coefficients are designed for 44.1-48 kHz. The engine handles higher sample
    // rates by decimating to this range before calling processSample().
    jassert (newSampleRate >= 44100.0 && newSampleRate <= 48000.0);

    // Olli Niemitalo's 4-section all-pass pair keeps the two outputs near 90 degrees
    // apart across most of the audio band. The first branch includes an extra sample
    // delay to align the pair.
    constexpr std::array<double, 4> coefficientsA{0.6923878 * 0.6923878,
                                                  0.9360654322959 * 0.9360654322959,
                                                  0.9882295226860 * 0.9882295226860,
                                                  0.9987488452737 * 0.9987488452737};

    constexpr std::array<double, 4> coefficientsB{0.4021921162426 * 0.4021921162426,
                                                  0.8561710882420 * 0.8561710882420,
                                                  0.9722909545651 * 0.9722909545651,
                                                  0.9952884791278 * 0.9952884791278};

    for (size_t index = 0; index < chainA.size (); ++index)
    {
        chainA[index].setCoefficient (coefficientsA[index]);
        chainB[index].setCoefficient (coefficientsB[index]);
    }

    reset ();
}

void QuadratureAllpassPair::reset () noexcept
{
    for (auto& stage : chainA)
        stage.reset ();

    for (auto& stage : chainB)
        stage.reset ();

    delayedI = 0.0;
}

std::pair<double, double> QuadratureAllpassPair::processSample (double input) noexcept
{
    auto i = input;
    auto q = input;

    for (auto& stage : chainA)
        i = stage.processSample (i);

    for (auto& stage : chainB)
        q = stage.processSample (q);

    const auto delayedOutput = delayedI;
    delayedI = i;

    return {delayedOutput, q};
}

void QuadratureAllpassPair::FirstOrderAllpass::setCoefficient (double newCoefficientSquared) noexcept
{
    coefficientSquared = newCoefficientSquared;
}

void QuadratureAllpassPair::FirstOrderAllpass::reset () noexcept
{
    xPrev = 0.0;
    yPrev = 0.0;
}

double QuadratureAllpassPair::FirstOrderAllpass::processSample (double input) noexcept
{
    const auto output = coefficientSquared * (input + yPrev) - xPrev;
    xPrev = input;
    yPrev = output;
    return output;
}

} // namespace SubOrbit::DSP
