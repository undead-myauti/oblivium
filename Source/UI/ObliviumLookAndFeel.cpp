#include "ObliviumLookAndFeel.h"

#include "BinaryData.h"

namespace oblivium::ui
{
namespace
{
    constexpr auto twoPi = juce::MathConstants<float>::twoPi;

    // Light comes from the top left; returns 0 (shadow) .. 1 (lit) for a point at this angle.
    float lightAt (float angle)
    {
        return 0.5f + 0.5f * std::cos (angle + juce::MathConstants<float>::pi * 0.25f);
    }

    juce::Path circle (juce::Point<float> centre, float radius)
    {
        juce::Path p;
        p.addEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        return p;
    }
}

ObliviumLookAndFeel::ObliviumLookAndFeel()
{
    display     = juce::Typeface::createSystemTypefaceFor (ObliviumBinary::CinzelBold_ttf, ObliviumBinary::CinzelBold_ttfSize);
    labelMedium = juce::Typeface::createSystemTypefaceFor (ObliviumBinary::MontserratMedium_ttf, ObliviumBinary::MontserratMedium_ttfSize);
    labelBold   = juce::Typeface::createSystemTypefaceFor (ObliviumBinary::MontserratBold_ttf, ObliviumBinary::MontserratBold_ttfSize);

    setColour (juce::ResizableWindow::backgroundColourId, colours::window);

    setColour (juce::ComboBox::textColourId, colours::goldLight);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff111113));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffd9d4c7));
    setColour (juce::PopupMenu::headerTextColourId, colours::goldMuted);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::gold.withAlpha (0.22f));
    setColour (juce::PopupMenu::highlightedTextColourId, colours::goldLight);
}

juce::Font ObliviumLookAndFeel::getDisplayFont (float height) const
{
    return juce::Font (juce::FontOptions (display).withHeight (height));
}

juce::Font ObliviumLookAndFeel::getLabelFont (float height, bool bold) const
{
    return juce::Font (juce::FontOptions (bold ? labelBold : labelMedium).withHeight (height));
}

juce::ColourGradient ObliviumLookAndFeel::goldGradient (juce::Rectangle<float> area)
{
    juce::ColourGradient gradient (colours::goldLight, area.getX(), area.getY(),
                                   colours::goldShadow, area.getX(), area.getBottom(), false);
    gradient.addColour (0.45, colours::gold);
    gradient.addColour (0.55, colours::goldDeep);
    gradient.addColour (0.8, colours::gold);
    return gradient;
}

void ObliviumLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float position, float startAngle, float endAngle,
                                            juce::Slider& slider)
{
    const bool hero = slider.getProperties()[heroProperty];
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = startAngle + position * (endAngle - startAngle);
    const bool active = slider.isMouseOverOrDragging() && slider.isEnabled();

    // Scale ticks around the hero knob.
    if (hero)
    {
        constexpr int ticks = 11;
        for (int i = 0; i < ticks; ++i)
        {
            const auto a = startAngle + (float) i / (float) (ticks - 1) * (endAngle - startAngle);
            const bool major = i == 0 || i == ticks / 2 || i == ticks - 1;
            const auto inner = centre.getPointOnCircumference (radius * (major ? 0.93f : 0.95f), a);
            const auto outer = centre.getPointOnCircumference (radius * 0.995f, a);
            g.setColour (colours::goldMuted.withAlpha (major ? 0.9f : 0.5f));
            g.drawLine ({ inner, outer }, radius * (major ? 0.022f : 0.014f));
        }
    }

    // Value ring.
    const auto ringRadius = radius * (hero ? 0.85f : 0.9f);
    const auto ringWidth = radius * (hero ? 0.045f : 0.075f);
    const juce::PathStrokeType ringStroke (ringWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (colours::track);
    g.strokePath (track, ringStroke);

    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const auto originAngle = bipolar
        ? startAngle + (float) slider.valueToProportionOfLength (0.0) * (endAngle - startAngle)
        : startAngle;

    if (std::abs (angle - originAngle) > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f,
                             juce::jmin (originAngle, angle), juce::jmax (originAngle, angle), true);

        for (int i = 3; i > 0; --i)
        {
            g.setColour (colours::gold.withAlpha (active ? 0.09f : 0.06f));
            g.strokePath (value, juce::PathStrokeType (ringWidth * (1.0f + (float) i * 1.1f),
                                                       juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        }

        g.setGradientFill (goldGradient (bounds));
        g.strokePath (value, ringStroke);
    }
    else if (bipolar)
    {
        g.setColour (colours::gold);
        g.fillEllipse (juce::Rectangle<float> (ringWidth, ringWidth).withCentre (centre.getPointOnCircumference (ringRadius, originAngle)));
    }

    // Knurled skirt, rotating with the knob.
    const auto bodyRadius = radius * (hero ? 0.72f : 0.68f);
    const auto body = circle (centre, bodyRadius);

    juce::DropShadow (juce::Colours::black.withAlpha (0.85f), juce::roundToInt (bodyRadius * 0.45f),
                      { 0, juce::roundToInt (bodyRadius * 0.14f) }).drawForPath (g, body);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b3b40), centre.x, centre.y - bodyRadius,
                                             juce::Colour (0xff060607), centre.x, centre.y + bodyRadius, false));
    g.fillPath (body);

    const int ridges = hero ? 64 : 40;
    for (int i = 0; i < ridges; ++i)
    {
        const auto a = angle + (float) i * twoPi / (float) ridges;
        const auto lit = lightAt (a);
        g.setColour (juce::Colours::white.withAlpha (0.02f + 0.13f * lit * lit));
        g.drawLine ({ centre.getPointOnCircumference (bodyRadius * 0.84f, a),
                      centre.getPointOnCircumference (bodyRadius * 0.985f, a) },
                    juce::jmax (0.6f, bodyRadius * 0.025f));
    }

    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.0f);

    // Cap.
    const auto capRadius = bodyRadius * 0.8f;
    const auto cap = circle (centre, capRadius);

    juce::DropShadow (juce::Colours::black.withAlpha (0.6f), juce::roundToInt (capRadius * 0.18f),
                      { 0, juce::roundToInt (capRadius * 0.06f) }).drawForPath (g, cap);

    if (hero)
    {
        juce::ColourGradient metal (colours::goldLight, centre.x - capRadius * 0.7f, centre.y - capRadius,
                                    colours::goldShadow, centre.x + capRadius * 0.6f, centre.y + capRadius, false);
        metal.addColour (0.4, colours::gold);
        metal.addColour (0.75, colours::goldDeep);
        g.setGradientFill (metal);
        g.fillPath (cap);

        // Spun-metal rings.
        for (float r = capRadius * 0.12f; r < capRadius; r += juce::jmax (1.2f, capRadius * 0.045f))
        {
            g.setColour (juce::Colours::white.withAlpha (0.035f));
            g.drawEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 0.5f);
        }

        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.45f), centre.x - capRadius * 0.35f, centre.y - capRadius * 0.55f,
                                                 juce::Colours::transparentWhite, centre.x, centre.y, true));
        g.fillPath (cap);

        g.setColour (colours::goldShadow.darker (0.4f));
        g.drawEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, 1.0f);
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e2e33), centre.x, centre.y - capRadius,
                                                 juce::Colour (0xff0c0c0e), centre.x, centre.y + capRadius, false));
        g.fillPath (cap);

        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.12f), centre.x - capRadius * 0.3f, centre.y - capRadius * 0.6f,
                                                 juce::Colours::transparentWhite, centre.x, centre.y, true));
        g.fillPath (cap);

        g.setColour (colours::gold.withAlpha (active ? 0.75f : 0.45f));
        g.drawEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, juce::jmax (0.8f, capRadius * 0.04f));
    }

    // Pointer.
    const auto pointerWidth = juce::jmax (1.5f, capRadius * (hero ? 0.1f : 0.12f));
    juce::Path pointer;
    pointer.startNewSubPath (centre.getPointOnCircumference (capRadius * 0.3f, angle));
    pointer.lineTo (centre.getPointOnCircumference (capRadius * 0.86f, angle));
    const juce::PathStrokeType pointerStroke (pointerWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    if (hero)
    {
        g.setColour (juce::Colour (0xff120d04));
        g.strokePath (pointer, pointerStroke);
    }
    else
    {
        g.setColour (colours::gold.withAlpha (0.25f));
        g.strokePath (pointer, juce::PathStrokeType (pointerWidth * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (colours::goldLight);
        g.strokePath (pointer, pointerStroke);
    }
}

void ObliviumLookAndFeel::drawCornerResizer (juce::Graphics& g, int width, int height,
                                             bool isMouseOver, bool isMouseDragging)
{
    const auto size = (float) juce::jmin (width, height);
    g.setColour (colours::gold.withAlpha (isMouseOver || isMouseDragging ? 0.8f : 0.35f));

    for (float i = 0.3f; i < 1.0f; i += 0.3f)
        g.drawLine ((float) width - size * i, (float) height - 1.0f,
                    (float) width - 1.0f, (float) height - size * i, 1.0f);
}

void ObliviumLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                        int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float> ((float) width, (float) height).reduced (0.5f);
    const auto corner = bounds.getHeight() * 0.5f;
    const bool hot = box.isMouseOver (true) || isButtonDown || box.isPopupActive();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0a0a0b), 0.0f, bounds.getY(),
                                             juce::Colour (0xff161618), 0.0f, bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, corner);
    g.setColour (colours::gold.withAlpha (hot ? 0.8f : 0.4f));
    g.drawRoundedRectangle (bounds, corner, 1.0f);

    // Microphone glyph on the left.
    const auto unit = bounds.getHeight();
    const auto mic = juce::Rectangle<float> (unit * 0.2f, unit * 0.36f).withCentre ({ bounds.getX() + unit * 0.75f, bounds.getCentreY() - unit * 0.08f });
    g.setColour (colours::gold);
    g.fillRoundedRectangle (mic, mic.getWidth() * 0.5f);

    juce::Path holder;
    holder.addCentredArc (mic.getCentreX(), mic.getCentreY() + unit * 0.02f, unit * 0.17f, unit * 0.17f, 0.0f,
                          juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi * 3.0f, true);
    holder.startNewSubPath (mic.getCentreX(), mic.getCentreY() + unit * 0.19f);
    holder.lineTo (mic.getCentreX(), mic.getCentreY() + unit * 0.3f);
    g.strokePath (holder, juce::PathStrokeType (juce::jmax (1.0f, unit * 0.06f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Chevron on the right.
    const auto arrowCentre = juce::Point<float> (bounds.getRight() - unit * 0.7f, bounds.getCentreY());
    const auto arrowSize = unit * 0.13f;
    juce::Path arrow;
    arrow.startNewSubPath (arrowCentre.translated (-arrowSize, -arrowSize * 0.5f));
    arrow.lineTo (arrowCentre.translated (0.0f, arrowSize * 0.5f));
    arrow.lineTo (arrowCentre.translated (arrowSize, -arrowSize * 0.5f));
    g.setColour (hot ? colours::goldLight : colours::gold);
    g.strokePath (arrow, juce::PathStrokeType (juce::jmax (1.0f, unit * 0.07f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Font ObliviumLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return getLabelFont ((float) box.getHeight() * 0.46f, false);
}

void ObliviumLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    const auto unit = box.getHeight();
    label.setBounds (box.getLocalBounds().withTrimmedLeft (juce::roundToInt ((float) unit * 1.2f))
                                         .withTrimmedRight (juce::roundToInt ((float) unit * 1.2f)));
    label.setFont (getComboBoxFont (box));
}

juce::Font ObliviumLookAndFeel::getPopupMenuFont()
{
    return getLabelFont (14.0f, false);
}
}
