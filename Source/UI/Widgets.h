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

/** Vertical segmented peak meter with peak hold and a latching clip segment on top.
    Clicking it clears the hold and the clip indicator. */
class LevelMeter final : public juce::Component
{
public:
    /** Feed the highest linear peak since the previous call and the time that has passed. */
    void pushPeak (float peak, float elapsedSeconds);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    static constexpr float floorDb = -45.0f;
    static constexpr int segments = 15; // 3 dB each, plus the clip segment

private:
    int litSegments() const noexcept;
    int holdSegment() const noexcept;

    float levelDb = floorDb, holdDb = floorDb;
    float holdSecondsLeft = 0.0f, clipSecondsLeft = 0.0f;
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
