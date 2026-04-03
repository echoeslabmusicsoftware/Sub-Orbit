# Development

## Prerequisites

- CMake 3.24+
- C++20 compiler (Clang 14+, GCC 12+, MSVC 2022+)
- clang-format (for code formatting)

### Linux only

```sh
sudo apt-get install libasound2-dev libjack-jackd2-dev libfreetype-dev \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
  mesa-common-dev libgl-dev libcurl4-openssl-dev libwebkit2gtk-4.1-dev
```

## Build

```sh
# Debug (default for development)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4

# Release
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j4
```

Dependencies (JUCE 8.0.12, Catch2 v3.7.1) are fetched automatically via CMake FetchContent.

## Test

```sh
ctest --test-dir build --output-on-failure
```

47 tests cover: DSP engine, crossover, quadrature allpass, sidechain envelope, mono check, state persistence, scope FIFO, bus layouts.

## Install to DAW (local testing)

```sh
# macOS VST3
cp -r "build/SubOrbit_artefacts/Debug/VST3/SUB ORBIT.vst3" \
      ~/Library/Audio/Plug-Ins/VST3/

# macOS AU
cp -r "build/SubOrbit_artefacts/Debug/AU/SUB ORBIT.component" \
      ~/Library/Audio/Plug-Ins/Components/
```

Rescan plugins in your DAW after copying.

## Debug

```sh
# Xcode
cmake -S . -B build -G Xcode -DCMAKE_BUILD_TYPE=Debug
open build/SubOrbit.xcodeproj

# lldb (command line)
lldb ./build/SubOrbitTests

```

## Format

CI enforces clang-format. Run before committing:

```sh
find Source tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

Check without modifying:

```sh
find Source tests -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror
```

## pluginval

Validate the built plugin locally:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DSUB_ORBIT_ENABLE_PLUGINVAL=ON \
  -DSUB_ORBIT_PLUGINVAL_EXECUTABLE=/path/to/pluginval
cmake --build build --config Release -j4
ctest --test-dir build --output-on-failure
```

## CI

The CI pipeline runs on every push and pull request:

1. **format-check** - clang-format compliance
2. **build-and-test** (macOS) - build, Catch2 tests, pluginval, auval
3. **build-and-test-linux** - build and Catch2 tests
4. **build-and-test-windows** - build, Catch2 tests, pluginval
5. **sanitizer-check** (macOS) - ASan + UBSan under debug build

## Architecture

```
Source/
  PluginProcessor.h/cpp     Audio processor (JUCE AudioProcessor)
  PluginEditor.h/cpp         UI editor (JUCE AudioProcessorEditor)
  SubOrbitParameterSpecs.h   Parameter definitions (constexpr)
  SubOrbitParameters.h       Parameter layout and APVTS helpers
  SubOrbitTypes.h            Shared types (DspParameters, AnalysisState)
  ScopeDataFifo.h            Lock-free FIFO for goniometer data
  DSP/
    SubOrbitEngine.h/cpp     Main DSP: crossover, quadrature, resampling
    QuadratureAllpassPair.h/cpp  Olli Niemitalo 4-section allpass pair
    LowBandAnalyzer.h       Correlation analysis for low band
    SidechainEnvelopeFollower.h  Ballistics-based envelope follower
  UI/
    CRTLookAndFeel.h/cpp    CRT terminal theme (colors, fonts, effects)
    UIConstants.h            Layout constants
    OrbitPanel.h/cpp         Orbit knob + crossover slider
    FieldMonitorPanel.h/cpp  Goniometer + correlation meter + mono check
    SidechainPanel.h/cpp     Attack/release knobs
    TipsOverlay.h/cpp        Usage tips popup
```
