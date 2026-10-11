#include "ObliviumEngine.h"

namespace oblivium
{
namespace
{
    constexpr size_t oversamplingOrder = 2; // 2^2 = 4x

    // Fixed voicing of the pedal, tuned for voice.
    constexpr float preHighPassHz   = 90.0f;   // keeps plosives and rumble out of the clipper
    constexpr float presenceHz      = 1800.0f; // pushes intelligibility into the clipper
    constexpr float presenceDb      = 4.0f;
    constexpr float dcBlockHz       = 15.0f;
    constexpr float deFizzHz        = 11000.0f;

    constexpr float bassHz          = 150.0f;
    constexpr float midHz           = 1000.0f;
    constexpr float midQ            = 0.9f;
    constexpr float trebleHz        = 4500.0f;
    constexpr float butterworthQ    = 0.7071f;

    // Level at which the saturator is matched to unity gain, so the drive knob
    // changes the character more than the loudness.
    constexpr float levelMatchReference = 0.15f;
    constexpr float maxAsymmetry        = 0.25f;

    constexpr double gainRampSeconds   = 0.05;
    constexpr double bypassRampSeconds = 0.03;

    float dbToGain (float db) { return juce::Decibels::decibelsToGain (db, -1000.0f); }
}

void ObliviumEngine::prepare (double newSampleRate, int maximumBlockSize, int newNumChannels)
{
    sampleRate = newSampleRate;
    maxBlockSize = juce::jmax (1, maximumBlockSize);
    numChannels = juce::jmax (1, newNumChannels);

    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlockSize, (juce::uint32) numChannels };

    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numChannels, oversamplingOrder,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true,  // maximum quality
        true); // integer latency, so the dry path can be aligned exactly
    oversampling->initProcessing ((size_t) maxBlockSize);
    latencySamples = (int) std::round (oversampling->getLatencyInSamples());

    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    const auto nyquistSafe = [this] (float hz) { return juce::jmin (hz, (float) sampleRate * 0.45f); };

    preHighPass.state = Coefficients::makeHighPass (sampleRate, preHighPassHz, butterworthQ);
    prePresence.state = Coefficients::makePeakFilter (sampleRate, nyquistSafe (presenceHz), 0.8f, dbToGain (presenceDb));
    postDcBlock.state = Coefficients::makeFirstOrderHighPass (sampleRate, dcBlockHz);
    postLowPass.state = Coefficients::makeLowPass (sampleRate, nyquistSafe (deFizzHz), butterworthQ);

    currentBassDb = currentMidDb = currentTrebleDb = 0.0f;
    lowShelf.state  = Coefficients::makeLowShelf (sampleRate, bassHz, butterworthQ, 1.0f);
    midPeak.state   = Coefficients::makePeakFilter (sampleRate, midHz, midQ, 1.0f);
    highShelf.state = Coefficients::makeHighShelf (sampleRate, nyquistSafe (trebleHz), butterworthQ, 1.0f);

    for (auto* filter : { &preHighPass, &prePresence, &postDcBlock, &postLowPass, &lowShelf, &midPeak, &highShelf })
        filter->prepare (spec);

    dryDelay.prepare (spec);
    dryDelay.setMaximumDelayInSamples (latencySamples + 1);
    dryDelay.setDelay ((float) latencySamples);
    dryBuffer.setSize (numChannels, maxBlockSize, false, true, false);

    const auto oversampledRate = sampleRate * (double) oversampling->getOversamplingFactor();
    inputGain.reset (sampleRate, gainRampSeconds);
    outputGain.reset (sampleRate, gainRampSeconds);
    mix.reset (sampleRate, gainRampSeconds);
    bypassAmount.reset (sampleRate, bypassRampSeconds);
    drive.reset (oversampledRate, gainRampSeconds);

    reset();
}

void ObliviumEngine::reset()
{
    if (oversampling != nullptr)
        oversampling->reset();

    for (auto* filter : { &preHighPass, &prePresence, &postDcBlock, &postLowPass, &lowShelf, &midPeak, &highShelf })
        filter->reset();

    dryDelay.reset();
    needsSnap = true;
}

void ObliviumEngine::setTargets (const EngineParameters& p, bool snapToTargets)
{
    const auto set = [snapToTargets] (auto& smoothed, float target)
    {
        if (snapToTargets)
            smoothed.setCurrentAndTargetValue (target);
        else
            smoothed.setTargetValue (target);
    };

    set (inputGain, dbToGain (p.inputGainDb));
    set (outputGain, dbToGain (p.outputGainDb));
    set (drive, juce::jlimit (0.0f, 1.0f, p.drive));
    set (mix, juce::jlimit (0.0f, 1.0f, p.mix));
    set (bypassAmount, p.bypassed ? 1.0f : 0.0f);
}

void ObliviumEngine::updateToneFilters (const EngineParameters& p)
{
    using Coefficients = juce::dsp::IIR::ArrayCoefficients<float>;

    // Assigning into the existing coefficient objects keeps the audio thread allocation free.
    if (! juce::exactlyEqual (p.bassDb, currentBassDb))
    {
        currentBassDb = p.bassDb;
        *lowShelf.state = Coefficients::makeLowShelf (sampleRate, bassHz, butterworthQ, dbToGain (currentBassDb));
    }

    if (! juce::exactlyEqual (p.midDb, currentMidDb))
    {
        currentMidDb = p.midDb;
        *midPeak.state = Coefficients::makePeakFilter (sampleRate, midHz, midQ, dbToGain (currentMidDb));
    }

    if (! juce::exactlyEqual (p.trebleDb, currentTrebleDb))
    {
        currentTrebleDb = p.trebleDb;
        const auto hz = juce::jmin (trebleHz, (float) sampleRate * 0.45f);
        *highShelf.state = Coefficients::makeHighShelf (sampleRate, hz, butterworthQ, dbToGain (currentTrebleDb));
    }
}

void ObliviumEngine::saturate (juce::dsp::AudioBlock<float> block)
{
    const auto numSamples = block.getNumSamples();
    const auto channels = block.getNumChannels();

    for (size_t i = 0; i < numSamples; ++i)
    {
        const auto amount = drive.getNextValue();
        const auto k = dbToGain (amount * maxDriveDb);

        // A DC offset before tanh clips the two half waves differently, adding the even
        // harmonics that make the distortion sound warm instead of fizzy. Subtracting
        // tanh(bias) keeps silence silent; the DC blocker removes what remains.
        const auto bias = maxAsymmetry * amount;
        const auto biasOffset = std::tanh (bias);
        const auto makeup = levelMatchReference / std::tanh (levelMatchReference * k);

        for (size_t ch = 0; ch < channels; ++ch)
        {
            auto* data = block.getChannelPointer (ch);
            data[i] = (std::tanh (k * data[i] + bias) - biasOffset) * makeup;
        }
    }
}

void ObliviumEngine::process (juce::dsp::AudioBlock<float> block, const EngineParameters& p)
{
    jassert (oversampling != nullptr);
    jassert (block.getNumSamples() <= (size_t) maxBlockSize);

    const auto numSamples = block.getNumSamples();
    auto wet = block.getSubsetChannelBlock (0, juce::jmin (block.getNumChannels(), (size_t) numChannels));
    const auto channels = wet.getNumChannels();

    setTargets (p, needsSnap);
    needsSnap = false;
    updateToneFilters (p);

    // Untouched input, delayed to line up with the oversampled wet path.
    auto dry = juce::dsp::AudioBlock<float> (dryBuffer).getSubsetChannelBlock (0, channels).getSubBlock (0, numSamples);
    dry.copyFrom (wet);
    dryDelay.process (juce::dsp::ProcessContextReplacing<float> (dry));

    wet.multiplyBy (inputGain);

    juce::dsp::ProcessContextReplacing<float> context (wet);
    preHighPass.process (context);
    prePresence.process (context);

    saturate (oversampling->processSamplesUp (wet));
    oversampling->processSamplesDown (wet);

    postDcBlock.process (context);
    postLowPass.process (context);
    lowShelf.process (context);
    midPeak.process (context);
    highShelf.process (context);

    for (size_t i = 0; i < numSamples; ++i)
    {
        const auto wetAmount = mix.getNextValue();
        const auto gain = outputGain.getNextValue();
        const auto bypass = bypassAmount.getNextValue();

        for (size_t ch = 0; ch < channels; ++ch)
        {
            const auto drySample = dry.getSample ((int) ch, (int) i);
            const auto blended = (drySample + wetAmount * (wet.getSample ((int) ch, (int) i) - drySample)) * gain;
            wet.setSample ((int) ch, (int) i, blended + bypass * (drySample - blended));
        }
    }
}
}
