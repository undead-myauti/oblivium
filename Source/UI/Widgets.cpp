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
    const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto radius = size * 0.44f;
    const auto thickness = radius * 0.1f;
    const auto centre = bounds.getCentre().translated (0.0f, -thickness * 0.5f);

    // Hex nut: shadows on the stone, then its flank, then its top face.
    ObliviumLookAndFeel::fillRadialShadow (g, centre.translated (0.0f, thickness + radius * 0.12f), radius * 1.12f, 0.6f, 0.55f);
    ObliviumLookAndFeel::fillRadialShadow (g, centre.translated (0.0f, thickness), radius * 0.98f, 0.9f, 0.85f);

    juce::Path nut;
    nut.addPolygon (centre, 6, radius, juce::MathConstants<float>::pi / 6.0f);

    juce::Path flank (nut);
    flank.applyTransform (juce::AffineTransform::translation (0.0f, thickness));
    flank.addRectangle (centre.x - radius * 0.87f, centre.y, radius * 1.74f, thickness);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1c1c1f), centre.x - radius, 0.0f,
                                             juce::Colour (0xff040405), centre.x + radius, 0.0f, false));
    g.fillPath (flank);

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
namespace
{
    constexpr float meterFallDbPerSecond = 26.0f;
    constexpr float meterHoldSeconds = 1.0f;
    constexpr float clipLatchSeconds = 2.0f;

    float segmentFloorDb (int index)
    {
        return LevelMeter::floorDb * (1.0f - (float) index / (float) LevelMeter::segments);
    }
}

int LevelMeter::litSegments() const noexcept
{
    int lit = 0;
    while (lit < segments && levelDb > segmentFloorDb (lit))
        ++lit;
    return lit;
}

int LevelMeter::holdSegment() const noexcept
{
    int index = -1;
    while (index + 1 < segments && holdDb > segmentFloorDb (index + 1))
        ++index;
    return index;
}

void LevelMeter::pushPeak (float peak, float elapsedSeconds)
{
    const auto previous = std::tuple { litSegments(), holdSegment(), clipSecondsLeft > 0.0f };
    const auto peakDb = peak > 0.0f ? juce::jmax (floorDb - 1.0f, 20.0f * std::log10 (peak)) : floorDb - 1.0f;

    levelDb = juce::jmax (peakDb, levelDb - meterFallDbPerSecond * elapsedSeconds);

    if (peakDb >= holdDb)
    {
        holdDb = peakDb;
        holdSecondsLeft = meterHoldSeconds;
    }
    else if ((holdSecondsLeft -= elapsedSeconds) <= 0.0f)
    {
        holdDb = juce::jmax (levelDb, holdDb - meterFallDbPerSecond * elapsedSeconds);
    }

    clipSecondsLeft = peak >= 1.0f ? clipLatchSeconds : juce::jmax (0.0f, clipSecondsLeft - elapsedSeconds);

    if (previous != std::tuple { litSegments(), holdSegment(), clipSecondsLeft > 0.0f })
        repaint();
}

void LevelMeter::mouseDown (const juce::MouseEvent&)
{
    holdDb = levelDb;
    holdSecondsLeft = 0.0f;
    clipSecondsLeft = 0.0f;
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto corner = bounds.getWidth() * 0.35f;

    // A slot cut into the stone: the lower lip catches the light, the upper wall casts a shadow inside.
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 1.0f), corner);
    g.setColour (juce::Colour (0xff030304));
    g.fillRoundedRectangle (bounds, corner);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black, 0.0f, bounds.getY(),
                                             juce::Colours::transparentBlack, 0.0f, bounds.getY() + bounds.getWidth() * 0.8f, false));
    g.fillRoundedRectangle (bounds, corner);
    g.setColour (colours::gold.withAlpha (0.22f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 0.8f);

    auto inner = bounds.reduced (juce::jmax (1.5f, bounds.getWidth() * 0.22f));
    const auto gap = juce::jmax (1.0f, inner.getHeight() * 0.012f);
    const auto segmentHeight = (inner.getHeight() - gap * (float) segments) / (float) (segments + 1);

    const auto colourFor = [] (int index)
    {
        if (index >= segments - 1) return juce::Colour (0xffffa53d); // last 3 dB before clipping
        if (index >= segments - 4) return colours::goldLight;        // -12 .. -3 dB
        return colours::gold;
    };

    const auto lit = litSegments();
    const auto hold = holdSegment();

    for (int i = 0; i <= segments; ++i)
    {
        const auto segment = inner.removeFromBottom (segmentHeight);
        inner.removeFromBottom (gap);

        const bool isClip = i == segments;
        const auto colour = isClip ? juce::Colour (0xffe8432e) : colourFor (i);
        const bool on = isClip ? clipSecondsLeft > 0.0f : (i < lit || i == hold);

        g.setColour (on ? colour : colour.withAlpha (0.13f));
        g.fillRoundedRectangle (segment, segmentHeight * 0.25f);
    }
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
