#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

namespace ids = oblivium::params;

ObliviumAudioProcessor::ObliviumAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "OBLIVIUM", ids::createLayout())
{
    input  = parameters.getRawParameterValue (ids::input);
    drive  = parameters.getRawParameterValue (ids::drive);
    bass   = parameters.getRawParameterValue (ids::bass);
    mid    = parameters.getRawParameterValue (ids::mid);
    treble = parameters.getRawParameterValue (ids::treble);
    mix    = parameters.getRawParameterValue (ids::mix);
    output = parameters.getRawParameterValue (ids::output);
    bypass = parameters.getRawParameterValue (ids::bypass);
    bypassParameter = parameters.getParameter (ids::bypass);
}

void ObliviumAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    maxBlockSize = juce::jmax (1, samplesPerBlock);
    engine.prepare (sampleRate, maxBlockSize, juce::jmax (1, getTotalNumInputChannels()));
    setLatencySamples (engine.getLatencyInSamples());
    outputLevel.store (0.0f);
}

void ObliviumAudioProcessor::releaseResources()
{
    engine.reset();
}

bool ObliviumAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

oblivium::EngineParameters ObliviumAudioProcessor::readParameters() const
{
    oblivium::EngineParameters p;
    p.inputGainDb  = input->load();
    p.drive        = drive->load() / 100.0f;
    p.bassDb       = bass->load();
    p.midDb        = mid->load();
    p.trebleDb     = treble->load();
    p.mix          = mix->load() / 100.0f;
    p.outputGainDb = output->load();
    p.bypassed     = bypass->load() >= 0.5f;
    return p;
}

void ObliviumAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numInputs = getTotalNumInputChannels();
    const auto numSamples = buffer.getNumSamples();

    for (auto ch = numInputs; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numInputs == 0 || numSamples == 0)
        return;

    const auto params = readParameters();
    auto block = juce::dsp::AudioBlock<float> (buffer).getSubsetChannelBlock (0, (size_t) numInputs);

    // Some hosts send blocks larger than announced in prepareToPlay.
    for (size_t start = 0; start < (size_t) numSamples; start += (size_t) maxBlockSize)
        engine.process (block.getSubBlock (start, juce::jmin ((size_t) maxBlockSize, (size_t) numSamples - start)), params);

    outputLevel.store (buffer.getMagnitude (0, numSamples), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* ObliviumAudioProcessor::createEditor()
{
    return new ObliviumAudioProcessorEditor (*this);
}

void ObliviumAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ObliviumAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ObliviumAudioProcessor();
}
