#pragma once

#include "PluginProcessor.h"
#include "UI/ObliviumLookAndFeel.h"
#include "UI/Widgets.h"

class ObliviumAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit ObliviumAudioProcessorEditor (ObliviumAudioProcessor&);
    ~ObliviumAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int baseWidth = 440;
    static constexpr int baseHeight = 660;

private:
    void timerCallback() override;
    void attach (oblivium::ui::LabelledKnob&, const char* parameterId);
    void setEngaged (bool engaged);
    void renderBackground (float pixelScale);
    void drawPedal (juce::Graphics&) const;

    ObliviumAudioProcessor& audioProcessor;
    oblivium::ui::ObliviumLookAndFeel lookAndFeel;

    oblivium::ui::LabelledKnob input { "INPUT" }, drive { "DRIVE", true }, output { "OUTPUT" };
    oblivium::ui::LabelledKnob bass { "BASS" }, mid { "MID" }, treble { "TREBLE" }, mix { "MIX" };
    oblivium::ui::Footswitch footswitch;
    oblivium::ui::Led led;

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::unique_ptr<juce::ParameterAttachment> bypassAttachment;
    bool engaged = true;
    float displayedLevel = 0.0f;

    juce::Image background;
    float backgroundPixelScale = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ObliviumAudioProcessorEditor)
};
