#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/ObliviumEngine.h"

class ObliviumAudioProcessor final : public juce::AudioProcessor
{
public:
    ObliviumAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParameter; }

    /** Peak levels (linear) since the previous call, for the editor's meters.
        Reading resets them, so the editor sees every peak regardless of its refresh rate. */
    float popInputPeak() noexcept  { return inputPeak.exchange (0.0f, std::memory_order_relaxed); }
    float popOutputPeak() noexcept { return outputPeak.exchange (0.0f, std::memory_order_relaxed); }

    juce::AudioProcessorValueTreeState parameters;

private:
    oblivium::EngineParameters readParameters() const;

    oblivium::ObliviumEngine engine;
    int maxBlockSize = 0;

    std::atomic<float>* input = nullptr;
    std::atomic<float>* drive = nullptr;
    std::atomic<float>* bass = nullptr;
    std::atomic<float>* mid = nullptr;
    std::atomic<float>* treble = nullptr;
    std::atomic<float>* mix = nullptr;
    std::atomic<float>* output = nullptr;
    std::atomic<float>* bypass = nullptr;
    juce::AudioProcessorParameter* bypassParameter = nullptr;

    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ObliviumAudioProcessor)
};
