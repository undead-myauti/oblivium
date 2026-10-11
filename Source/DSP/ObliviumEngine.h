#pragma once

#include <juce_dsp/juce_dsp.h>

namespace oblivium
{
struct EngineParameters
{
    float inputGainDb  = 0.0f;
    float drive        = 0.4f;  // 0..1
    float bassDb       = 0.0f;
    float midDb        = 0.0f;
    float trebleDb     = 0.0f;
    float mix          = 1.0f;  // 0..1
    float outputGainDb = 0.0f;
    bool bypassed      = false;
};

/** Signal chain:
    input gain -> pre-emphasis -> 4x oversampled asymmetric saturation -> DC block / de-fizz
    -> bass / mid / treble -> dry-wet mix -> output gain, with a click-free bypass.
    The dry path is delayed by the oversampling latency so mix and bypass stay phase aligned. */
class ObliviumEngine
{
public:
    void prepare (double sampleRate, int maximumBlockSize, int numChannels);
    void reset();

    /** The block must not hold more samples than the maximum block size given to prepare(). */
    void process (juce::dsp::AudioBlock<float> block, const EngineParameters& parameters);

    int getLatencyInSamples() const noexcept { return latencySamples; }

    /** Peak level after the input gain (what hits the clipper) in the last processed block. */
    float getLastInputPeak() const noexcept { return lastInputPeak; }

    static constexpr float maxDriveDb = 40.0f;

private:
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                  juce::dsp::IIR::Coefficients<float>>;

    void setTargets (const EngineParameters& parameters, bool snapToTargets);
    void updateToneFilters (const EngineParameters& parameters);
    void saturate (juce::dsp::AudioBlock<float> block);

    double sampleRate = 44100.0;
    int maxBlockSize = 0;
    int numChannels = 0;
    int latencySamples = 0;
    bool needsSnap = true;
    float lastInputPeak = 0.0f;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    Filter preHighPass, prePresence, postDcBlock, postLowPass;
    Filter lowShelf, midPeak, highShelf;
    float currentBassDb = 0.0f, currentMidDb = 0.0f, currentTrebleDb = 0.0f;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay;
    juce::AudioBuffer<float> dryBuffer;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inputGain, outputGain;
    juce::SmoothedValue<float> drive, mix, bypassAmount;
};
}
