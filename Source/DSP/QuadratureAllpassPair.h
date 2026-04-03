#pragma once

#include <array>
#include <utility>

namespace SubOrbit::DSP
{

class QuadratureAllpassPair
{
  public:
    void prepare (double newSampleRate);
    void reset () noexcept;
    std::pair<double, double> processSample (double input) noexcept;

  private:
    struct FirstOrderAllpass
    {
        void setCoefficient (double newCoefficientSquared) noexcept;
        void reset () noexcept;
        double processSample (double input) noexcept;

        double coefficientSquared = 0.0;
        double xPrev = 0.0;
        double yPrev = 0.0;
    };

    std::array<FirstOrderAllpass, 4> chainA;
    std::array<FirstOrderAllpass, 4> chainB;
    double delayedI = 0.0;
};

} // namespace SubOrbit::DSP
