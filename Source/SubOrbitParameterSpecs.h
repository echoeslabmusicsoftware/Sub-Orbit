#pragma once

#include <array>
#include <string_view>

namespace SubOrbitParameterSpecs
{
enum class FloatScaling
{
    linear,
    logarithmic,
};

struct FloatParameterSpec
{
    std::string_view id;
    std::string_view name;
    std::string_view label;
    float defaultValue;
    float minValue;
    float maxValue;
    float step;
    FloatScaling scaling;
};

struct BoolParameterSpec
{
    std::string_view id;
    std::string_view name;
    bool defaultValue;
};

inline constexpr int kParameterVersionHint = 3;

inline constexpr FloatParameterSpec kOrbit{"orbit", "Orbit", "", 0.0f, 0.0f, 1.0f, 0.001f, FloatScaling::linear};

inline constexpr FloatParameterSpec kRangeHz{
    "rangeHz", "Range", "Hz", 100.0f, 60.0f, 180.0f, 0.0f, FloatScaling::logarithmic};

inline constexpr FloatParameterSpec kSidechainAmount{
    "sidechainAmount", "SC Amount", "", 1.0f, 0.0f, 1.0f, 0.001f, FloatScaling::linear};

inline constexpr FloatParameterSpec kSidechainAttackMs{
    "sidechainAttackMs", "SC Attack", "ms", 10.0f, 0.1f, 100.0f, 0.0f, FloatScaling::logarithmic};

inline constexpr FloatParameterSpec kSidechainReleaseMs{
    "sidechainReleaseMs", "SC Release", "ms", 300.0f, 1.0f, 3000.0f, 0.0f, FloatScaling::logarithmic};

inline constexpr std::array<std::string_view, 5> kAutomatableParameterIDs{
    kOrbit.id, kRangeHz.id, kSidechainAmount.id, kSidechainAttackMs.id, kSidechainReleaseMs.id};
} // namespace SubOrbitParameterSpecs
