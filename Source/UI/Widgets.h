#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace oblivium::ui
{
/** A rotary knob with its name and current value printed underneath.
    The knob takes a square of the component's width; the rest is the caption. */
class LabelledKnob final : public juce::Component
{
public:
    explicit LabelledKnob (juce::String name, bool hero = false);
    ~LabelledKnob() override;

    juce::Slider& getSlider() noexcept { return slider; }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

private:
    juce::Slider slider;
    juce::String name;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LabelledKnob)
};

/** Stomp switch. Clicking it calls onClick; the owner decides what "engaged" means. */
class Footswitch final : public juce::Button
{
public:
    Footswitch();

    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    bool hitTest (int x, int y) override;
};

/** Status LED; brightness follows the signal while the effect is engaged. */
class Led final : public juce::Component
{
public:
    void setOn (bool shouldBeOn);
    void setLevel (float newLevel);

    void paint (juce::Graphics&) override;

private:
    bool on = true;
    float level = 0.0f;
};
}
