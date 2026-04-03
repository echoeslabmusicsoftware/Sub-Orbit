#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "DSP/LowBandAnalyzer.h"

TEST_CASE ("LowBandAnalyzer reports monoSafe when inactive", "[dsp][analyzer]")
{
    SubOrbit::DSP::LowBandAnalyzer analyzer;
    analyzer.reset ();

    const auto state = analyzer.update (1.0, 1.0, 1.0, 0.5f, false, 512, 48000.0);

    REQUIRE (state.lowBandCorrelation == 1.0f);
    REQUIRE (state.effectiveOrbit == 0.0f);
}

TEST_CASE ("LowBandAnalyzer handles silence without NaN or division by zero", "[dsp][analyzer][edge]")
{
    SubOrbit::DSP::LowBandAnalyzer analyzer;
    analyzer.reset ();

    const auto state = analyzer.update (0.0, 0.0, 0.0, 0.5f, true, 512, 48000.0);

    REQUIRE (std::isfinite (state.lowBandCorrelation));
    REQUIRE (state.lowBandCorrelation >= -1.0f);
    REQUIRE (state.lowBandCorrelation <= 1.0f);
}

TEST_CASE ("LowBandAnalyzer tracks low orbit values", "[dsp][analyzer]")
{
    SubOrbit::DSP::LowBandAnalyzer analyzer;
    analyzer.reset ();

    const auto state = analyzer.update (1.0, 1.0, 1.0, 0.005f, true, 512, 48000.0);

    REQUIRE (state.effectiveOrbit == 0.005f);
    REQUIRE (state.lowBandCorrelation > 0.0f);
}

TEST_CASE ("LowBandAnalyzer tracks anti-correlated signal", "[dsp][analyzer]")
{
    SubOrbit::DSP::LowBandAnalyzer analyzer;
    analyzer.reset ();

    // Feed many blocks of anti-correlated data to push smoothed correlation well below 0.25
    for (int i = 0; i < 100; ++i)
        analyzer.update (1.0, 1.0, -1.0, 0.5f, true, 512, 48000.0);

    const auto state = analyzer.update (1.0, 1.0, -1.0, 0.5f, true, 512, 48000.0);

    REQUIRE (state.lowBandCorrelation < 0.25f);
    REQUIRE (state.effectiveOrbit == 0.5f);
}

TEST_CASE ("LowBandAnalyzer tracks well-correlated active signal", "[dsp][analyzer]")
{
    SubOrbit::DSP::LowBandAnalyzer analyzer;
    analyzer.reset ();

    // Feed several blocks of perfectly correlated data
    for (int i = 0; i < 20; ++i)
        analyzer.update (1.0, 1.0, 1.0, 0.5f, true, 512, 48000.0);

    const auto state = analyzer.update (1.0, 1.0, 1.0, 0.5f, true, 512, 48000.0);

    REQUIRE (state.lowBandCorrelation > 0.95f);
    REQUIRE (state.effectiveOrbit == 0.5f);
}
