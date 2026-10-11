#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace oblivium::params
{
    inline constexpr auto input  = "input";
    inline constexpr auto drive  = "drive";
    inline constexpr auto bass   = "bass";
    inline constexpr auto mid    = "mid";
    inline constexpr auto treble = "treble";
    inline constexpr auto mix    = "mix";
    inline constexpr auto output = "output";
    inline constexpr auto bypass = "bypass";

    inline constexpr float gainRangeDb = 24.0f;
    inline constexpr float eqRangeDb   = 12.0f;

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
