#include "Parameters.h"

namespace oblivium::params
{
namespace
{
    juce::String decibelText (float value, int)
    {
        if (std::abs (value) < 0.05f)
            return "0.0 dB";

        return (value > 0.0f ? "+" : "") + juce::String (value, 1) + " dB";
    }

    juce::String percentText (float value, int)
    {
        return juce::String (juce::roundToInt (value)) + " %";
    }

    float numberFromText (const juce::String& text)
    {
        return text.retainCharacters ("-+.0123456789").getFloatValue();
    }

    std::unique_ptr<juce::AudioParameterFloat> decibelParameter (const char* id, const juce::String& name, float range)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 },
            name,
            juce::NormalisableRange<float> { -range, range, 0.1f },
            0.0f,
            juce::AudioParameterFloatAttributes()
                .withLabel ("dB")
                .withStringFromValueFunction (decibelText)
                .withValueFromStringFunction (numberFromText));
    }

    std::unique_ptr<juce::AudioParameterFloat> percentParameter (const char* id, const juce::String& name, float defaultValue)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 },
            name,
            juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f },
            defaultValue,
            juce::AudioParameterFloatAttributes()
                .withLabel ("%")
                .withStringFromValueFunction (percentText)
                .withValueFromStringFunction (numberFromText));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (decibelParameter (input, "Input", gainRangeDb),
                percentParameter (drive, "Drive", 40.0f),
                decibelParameter (bass, "Bass", eqRangeDb),
                decibelParameter (mid, "Mid", eqRangeDb),
                decibelParameter (treble, "Treble", eqRangeDb),
                percentParameter (mix, "Mix", 100.0f),
                decibelParameter (output, "Output", gainRangeDb),
                std::make_unique<juce::AudioParameterBool> (juce::ParameterID { bypass, 1 }, "Bypass", false));

    return layout;
}
}
