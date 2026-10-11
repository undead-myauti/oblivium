#include "Widgets.h"
#include "ObliviumLookAndFeel.h"

namespace oblivium::ui
{
namespace
{
    ObliviumLookAndFeel* oblivium (juce::Component& c)
    {
        return dynamic_cast<ObliviumLookAndFeel*> (&c.getLookAndFeel());
    }
}

//==============================================================================
LabelledKnob::LabelledKnob (juce::String knobName, bool hero)
    : name (std::move (knobName))
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (hero ? 320 : 220);
    slider.setVelocityModeParameters (1.0, 1, 0.0, false, juce::ModifierKeys::shiftModifier);
    slider.getProperties().set (ObliviumLookAndFeel::heroProperty, hero);
    slider.setTitle (name);
    slider.onValueChange = [this] { repaint(); };
    slider.addMouseListener (this, false);
    addAndMakeVisible (slider);
}

LabelledKnob::~LabelledKnob()
{
    slider.removeMouseListener (this);
}

void LabelledKnob::resized()
{
    slider.setBounds (getLocalBounds().removeFromTop (getWidth()));
}

void LabelledKnob::paint (juce::Graphics& g)
{
    auto* lnf = oblivium (*this);
    if (lnf == nullptr)
        return;

    auto caption = getLocalBounds().toFloat().withTrimmedTop ((float) getWidth());
    const auto lineHeight = caption.getHeight() * 0.5f;
    const bool active = slider.isMouseOverOrDragging();

    g.setColour (colours::gold);
    g.setFont (lnf->getLabelFont (lineHeight * 0.78f, true).withExtraKerningFactor (0.22f));
    g.drawText (name, caption.removeFromTop (lineHeight), juce::Justification::centred, false);

    g.setColour (active ? colours::goldLight : colours::textMuted);
    g.setFont (lnf->getLabelFont (lineHeight * 0.68f, false).withExtraKerningFactor (0.06f));
    g.drawText (slider.getTextFromValue (slider.getValue()), caption, juce::Justification::centred, false);
}

//==============================================================================
Footswitch::Footswitch() : juce::Button ("Bypass")
{
    setTitle ("Bypass");
    setTooltip ("Engage / bypass");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

bool Footswitch::hitTest (int x, int y)
{
    const auto centre = getLocalBounds().toFloat().getCentre();
    const auto radius = (float) juce::jmin (getWidth(), getHeight()) * 0.5f;
    return centre.getDistanceFrom ({ (float) x, (float) y }) <= radius;
}

void Footswitch::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 2.0f;

    // Hex nut.
    juce::Path nut;
    nut.addPolygon (centre, 6, radius, juce::MathConstants<float>::pi / 6.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.9f), juce::roundToInt (radius * 0.3f),
                      { 0, juce::roundToInt (radius * 0.1f) }).drawForPath (g, nut);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3f), centre.x - radius, centre.y - radius,
                                             juce::Colour (0xff09090a), centre.x + radius, centre.y + radius, false));
    g.fillPath (nut);
    g.setColour (colours::gold.withAlpha (highlighted ? 0.75f : 0.45f));
    g.strokePath (nut, juce::PathStrokeType (1.0f));

    // Threaded collar.
    const auto collar = radius * 0.74f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff55555b), centre.x, centre.y - collar,
                                             juce::Colour (0xff111113), centre.x, centre.y + collar, false));
    g.fillEllipse (juce::Rectangle<float> (collar * 2.0f, collar * 2.0f).withCentre (centre));
    for (float r = collar * 0.78f; r < collar; r += juce::jmax (1.0f, collar * 0.05f))
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (centre), 0.6f);
    }

    // Plunger.
    const auto dome = collar * (down ? 0.7f : 0.74f);
    const auto domeArea = juce::Rectangle<float> (dome * 2.0f, dome * 2.0f).withCentre (centre.translated (0.0f, down ? 1.0f : 0.0f));

    juce::DropShadow (juce::Colours::black.withAlpha (0.8f), juce::roundToInt (dome * (down ? 0.12f : 0.25f)),
                      { 0, juce::roundToInt (dome * (down ? 0.02f : 0.08f)) })
        .drawForRectangle (g, domeArea.toNearestInt());

    juce::ColourGradient chrome (juce::Colour (0xff6c6c72), domeArea.getX() + dome * 0.6f, domeArea.getY() + dome * 0.5f,
                                 juce::Colour (0xff050506), domeArea.getRight(), domeArea.getBottom(), true);
    chrome.addColour (0.45, juce::Colour (0xff222226));
    g.setGradientFill (chrome);
    g.fillEllipse (domeArea);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (down ? 0.18f : 0.32f),
                                             domeArea.getCentreX() - dome * 0.3f, domeArea.getY() + dome * 0.35f,
                                             juce::Colours::transparentWhite,
                                             domeArea.getCentreX() - dome * 0.3f, domeArea.getY() + dome * 1.1f, true));
    g.fillEllipse (domeArea.reduced (dome * 0.12f));

    g.setGradientFill (ObliviumLookAndFeel::goldGradient (domeArea));
    g.drawEllipse (domeArea, juce::jmax (1.2f, dome * 0.06f));
}

//==============================================================================
void Led::setOn (bool shouldBeOn)
{
    if (on != shouldBeOn)
    {
        on = shouldBeOn;
        repaint();
    }
}

void Led::setLevel (float newLevel)
{
    newLevel = juce::jlimit (0.0f, 1.0f, newLevel);
    if (std::abs (newLevel - level) > 0.01f)
    {
        level = newLevel;
        if (on)
            repaint();
    }
}

void Led::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const auto lens = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.16f;
    const juce::Colour amber { 0xffffc94a };

    if (on)
    {
        const auto glow = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto intensity = 0.55f + 0.45f * level;
        g.setGradientFill (juce::ColourGradient (amber.withAlpha (0.55f * intensity), centre.x, centre.y,
                                                 amber.withAlpha (0.0f), centre.x + glow, centre.y, true));
        g.fillEllipse (juce::Rectangle<float> (glow * 2.0f, glow * 2.0f).withCentre (centre));
    }

    const auto lensArea = juce::Rectangle<float> (lens * 2.0f, lens * 2.0f).withCentre (centre);
    g.setColour (juce::Colour (0xff1a1408));
    g.fillEllipse (lensArea.expanded (lens * 0.45f));
    g.setGradientFill (ObliviumLookAndFeel::goldGradient (lensArea.expanded (lens * 0.45f)));
    g.drawEllipse (lensArea.expanded (lens * 0.45f), juce::jmax (0.8f, lens * 0.18f));

    g.setGradientFill (on ? juce::ColourGradient (juce::Colour (0xfffff6d8), centre.x - lens * 0.3f, centre.y - lens * 0.3f,
                                                  amber, centre.x + lens, centre.y + lens, true)
                          : juce::ColourGradient (juce::Colour (0xff4a3a1c), centre.x - lens * 0.3f, centre.y - lens * 0.3f,
                                                  juce::Colour (0xff1c1508), centre.x + lens, centre.y + lens, true));
    g.fillEllipse (lensArea);
}
}
