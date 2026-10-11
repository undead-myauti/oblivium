#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace oblivium::ui
{
namespace colours
{
    inline const juce::Colour goldLight   { 0xfff7e08a };
    inline const juce::Colour gold        { 0xffd4af37 };
    inline const juce::Colour goldDeep    { 0xff9c7420 };
    inline const juce::Colour goldShadow  { 0xff5c430f };
    inline const juce::Colour goldMuted   { 0xffa88a45 };

    inline const juce::Colour enclosureTop    { 0xff1e1e21 };
    inline const juce::Colour enclosureBottom { 0xff0b0b0d };
    inline const juce::Colour window          { 0xff050506 };
    inline const juce::Colour textMuted       { 0xff86868c };
}

class ObliviumLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ObliviumLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawCornerResizer (juce::Graphics&, int width, int height,
                            bool isMouseOver, bool isMouseDragging) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;

    juce::Font getDisplayFont (float height) const;
    juce::Font getLabelFont (float height, bool bold) const;

    /** Round shadow on the pedal surface: solid up to `solidFraction` of the radius, then fading out. */
    static void fillRadialShadow (juce::Graphics&, juce::Point<float> centre, float radius,
                                  float alpha, float solidFraction);

    /** Brushed-gold fill running top to bottom across the given area. */
    static juce::ColourGradient goldGradient (juce::Rectangle<float> area);

    /** Slider property that turns a knob into the large gold "hero" knob. */
    static constexpr const char* heroProperty = "obliviumHero";

private:
    juce::Typeface::Ptr display, labelMedium, labelBold;
};
}
