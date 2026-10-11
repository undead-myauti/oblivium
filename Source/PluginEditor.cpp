#include "PluginEditor.h"
#include "Parameters.h"

namespace ids = oblivium::params;
namespace colours = oblivium::ui::colours;

namespace
{
    // Layout in base (unscaled) pixels; everything scales with the window.
    const juce::Rectangle<float> enclosureArea { 8.0f, 8.0f, 424.0f, 644.0f };
    constexpr float enclosureCorner = 26.0f;

    const juce::Rectangle<float> driveArea  { 145.0f, 52.0f, 150.0f, 188.0f };
    const juce::Rectangle<float> inputArea  { 39.0f, 132.0f, 78.0f, 108.0f };
    const juce::Rectangle<float> outputArea { 323.0f, 132.0f, 78.0f, 108.0f };

    // Meters sit on the inner side of the input / output knobs, level with the knob.
    const juce::Rectangle<float> inputMeterArea  { 121.0f, 134.0f, 10.0f, 96.0f };
    const juce::Rectangle<float> outputMeterArea { 309.0f, 134.0f, 10.0f, 96.0f };

    constexpr float toneTop = 282.0f, toneKnob = 62.0f, toneCaption = 30.0f;
    constexpr float bassX = 72.0f, midX = 164.0f, trebleX = 256.0f, mixX = 368.0f;
    constexpr float toneMixSeparatorX = 312.0f;
    constexpr float dividerY = 262.0f;

    const juce::Rectangle<float> logoArea { 20.0f, 398.0f, 400.0f, 66.0f };
    constexpr float taglineY = 474.0f;
    const juce::Rectangle<float> ledArea { 205.0f, 497.0f, 30.0f, 30.0f };
    const juce::Rectangle<float> footswitchArea { 172.0f, 534.0f, 96.0f, 96.0f };

    juce::Rectangle<float> toneKnobArea (float centreX)
    {
        return { centreX - toneKnob * 0.5f, toneTop, toneKnob, toneKnob + toneCaption };
    }

    void drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius, float slotAngle)
    {
        const auto area = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (area.expanded (1.5f).translated (0.0f, 1.0f));

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a5a60), area.getX(), area.getY(),
                                                 juce::Colour (0xff0c0c0d), area.getRight(), area.getBottom(), false));
        g.fillEllipse (area);
        g.setColour (colours::goldDeep.withAlpha (0.6f));
        g.drawEllipse (area, 0.7f);

        for (const auto angle : { slotAngle, slotAngle + juce::MathConstants<float>::halfPi })
        {
            const auto offset = juce::Point<float> (std::cos (angle), std::sin (angle)) * (radius * 0.62f);
            g.setColour (juce::Colours::black.withAlpha (0.85f));
            g.drawLine ({ centre - offset, centre + offset }, radius * 0.28f);
        }
    }

    void drawRule (juce::Graphics& g, float fromX, float toX, float y, bool fadeOutwards)
    {
        const auto edge = colours::gold.withAlpha (0.0f);
        const auto centre = colours::gold.withAlpha (0.55f);
        g.setGradientFill (juce::ColourGradient (fadeOutwards ? edge : centre, fromX, y,
                                                 fadeOutwards ? centre : edge, toX, y, false));
        g.fillRect (juce::Rectangle<float> (fromX, y - 0.5f, toX - fromX, 1.0f));
    }
}

ObliviumAudioProcessorEditor::ObliviumAudioProcessorEditor (ObliviumAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lookAndFeel);

    attach (input, ids::input);
    attach (drive, ids::drive);
    attach (output, ids::output);
    attach (bass, ids::bass);
    attach (mid, ids::mid);
    attach (treble, ids::treble);
    attach (mix, ids::mix);

    inputMeter.setTitle ("Input level");
    outputMeter.setTitle ("Output level");
    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);

    footswitch.onClick = [this] { bypassAttachment->setValueAsCompleteGesture (engaged ? 1.0f : 0.0f); };
    addAndMakeVisible (footswitch);
    addAndMakeVisible (led);

    bypassAttachment = std::make_unique<juce::ParameterAttachment> (
        *audioProcessor.parameters.getParameter (ids::bypass),
        [this] (float value) { setEngaged (value < 0.5f); });
    bypassAttachment->sendInitialUpdate();

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) baseWidth / (double) baseHeight);
    setResizeLimits (baseWidth * 3 / 4, baseHeight * 3 / 4, baseWidth * 7 / 4, baseHeight * 7 / 4);
    setSize (baseWidth, baseHeight);

    startTimerHz (30);
}

ObliviumAudioProcessorEditor::~ObliviumAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ObliviumAudioProcessorEditor::attach (oblivium::ui::LabelledKnob& knob, const char* parameterId)
{
    auto& slider = knob.getSlider();
    sliderAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.parameters, parameterId, slider));

    if (auto* parameter = audioProcessor.parameters.getParameter (parameterId))
        slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));

    addAndMakeVisible (knob);
}

void ObliviumAudioProcessorEditor::setEngaged (bool shouldBeEngaged)
{
    engaged = shouldBeEngaged;
    led.setOn (engaged);
    footswitch.setTooltip (engaged ? "Bypass" : "Engage");
}

void ObliviumAudioProcessorEditor::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounterHiRes();
    const auto elapsed = lastTimerMs > 0.0 ? (float) juce::jlimit (0.0, 0.25, (now - lastTimerMs) / 1000.0) : 0.0f;
    lastTimerMs = now;

    const auto inputPeak = audioProcessor.popInputPeak();
    const auto outputPeak = audioProcessor.popOutputPeak();
    inputMeter.pushPeak (inputPeak, elapsed);
    outputMeter.pushPeak (outputPeak, elapsed);

    displayedLevel = juce::jmax (outputPeak, displayedLevel * 0.82f);
    led.setLevel (std::sqrt (juce::jlimit (0.0f, 1.0f, displayedLevel)));
}

void ObliviumAudioProcessorEditor::resized()
{
    const auto scale = (float) getWidth() / (float) baseWidth;
    const auto place = [scale] (juce::Component& c, juce::Rectangle<float> area)
    {
        c.setBounds ((area * scale).toNearestInt());
    };

    place (drive, driveArea);
    place (input, inputArea);
    place (output, outputArea);
    place (bass, toneKnobArea (bassX));
    place (mid, toneKnobArea (midX));
    place (treble, toneKnobArea (trebleX));
    place (mix, toneKnobArea (mixX));
    place (inputMeter, inputMeterArea);
    place (outputMeter, outputMeterArea);
    place (led, ledArea);
    place (footswitch, footswitchArea);

    background = {};
}

void ObliviumAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto pixelScale = g.getInternalContext().getPhysicalPixelScaleFactor();

    if (background.isNull()
        || ! juce::exactlyEqual (backgroundPixelScale, pixelScale)
        || background.getWidth() != juce::roundToInt ((float) getWidth() * pixelScale))
        renderBackground (pixelScale);

    g.drawImage (background, getLocalBounds().toFloat());
}

void ObliviumAudioProcessorEditor::renderBackground (float pixelScale)
{
    const auto width = juce::jmax (1, juce::roundToInt ((float) getWidth() * pixelScale));
    const auto height = juce::jmax (1, juce::roundToInt ((float) getHeight() * pixelScale));

    background = juce::Image (juce::Image::RGB, width, height, true);
    backgroundPixelScale = pixelScale;

    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale ((float) width / (float) baseWidth,
                                                  (float) height / (float) baseHeight));
    drawPedal (g);
}

void ObliviumAudioProcessorEditor::drawPedal (juce::Graphics& g) const
{
    g.fillAll (colours::window);

    juce::Path enclosure;
    enclosure.addRoundedRectangle (enclosureArea, enclosureCorner);

    juce::DropShadow (juce::Colours::black, 14, { 0, 4 }).drawForPath (g, enclosure);

    g.setGradientFill (juce::ColourGradient (colours::enclosureTop, 0.0f, enclosureArea.getY(),
                                             colours::enclosureBottom, 0.0f, enclosureArea.getBottom(), false));
    g.fillPath (enclosure);

    {
        // Brushed finish and vignette, kept inside the enclosure.
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (enclosure);

        juce::Random random (0x0b11);
        for (auto y = enclosureArea.getY(); y < enclosureArea.getBottom(); y += 1.0f)
        {
            g.setColour (juce::Colours::white.withAlpha (random.nextFloat() * 0.022f));
            g.fillRect (enclosureArea.getX(), y, enclosureArea.getWidth(), 0.6f);
        }

        const auto centre = enclosureArea.getCentre().translated (0.0f, -60.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, centre.x, centre.y,
                                                 juce::Colours::black.withAlpha (0.55f), centre.x + 430.0f, centre.y, true));
        g.fillPath (enclosure);

        // Warm halo behind the drive knob and the logo.
        for (const auto& [glowCentre, glowRadius] : { std::pair { driveArea.withHeight (driveArea.getWidth()).getCentre(), 130.0f },
                                                      std::pair { logoArea.getCentre(), 170.0f } })
        {
            g.setGradientFill (juce::ColourGradient (colours::gold.withAlpha (0.07f), glowCentre.x, glowCentre.y,
                                                     colours::gold.withAlpha (0.0f), glowCentre.x + glowRadius, glowCentre.y, true));
            g.fillEllipse (juce::Rectangle<float> (glowRadius * 2.0f, glowRadius * 2.0f).withCentre (glowCentre));
        }
    }

    // Gold trim.
    g.setGradientFill (oblivium::ui::ObliviumLookAndFeel::goldGradient (enclosureArea));
    g.strokePath (enclosure, juce::PathStrokeType (1.6f));

    juce::Path innerTrim;
    innerTrim.addRoundedRectangle (enclosureArea.reduced (7.0f), enclosureCorner - 6.0f);
    g.setColour (colours::gold.withAlpha (0.18f));
    g.strokePath (innerTrim, juce::PathStrokeType (0.7f));

    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine (juce::roundToInt (enclosureArea.getY() + 2.0f),
                          enclosureArea.getX() + enclosureCorner, enclosureArea.getRight() - enclosureCorner);

    // Screws.
    const auto inset = 24.0f;
    drawScrew (g, { enclosureArea.getX() + inset, enclosureArea.getY() + inset }, 6.5f, 0.3f);
    drawScrew (g, { enclosureArea.getRight() - inset, enclosureArea.getY() + inset }, 6.5f, 1.1f);
    drawScrew (g, { enclosureArea.getX() + inset, enclosureArea.getBottom() - inset }, 6.5f, 0.8f);
    drawScrew (g, { enclosureArea.getRight() - inset, enclosureArea.getBottom() - inset }, 6.5f, 0.1f);

    // Jack markings along the top edge, as on a real enclosure.
    g.setFont (lookAndFeel.getLabelFont (7.5f, true).withExtraKerningFactor (0.3f));
    g.setColour (colours::goldMuted.withAlpha (0.7f));
    g.drawText ("OUT", juce::Rectangle<float> (52.0f, 20.0f, 60.0f, 12.0f), juce::Justification::centredLeft, false);
    g.drawText ("IN", juce::Rectangle<float> (328.0f, 20.0f, 60.0f, 12.0f), juce::Justification::centredRight, false);

    // Divider between the gain stage and the tone stack.
    drawRule (g, 44.0f, 206.0f, dividerY, true);
    drawRule (g, 234.0f, 396.0f, dividerY, false);
    juce::Path diamond;
    diamond.addPolygon ({ 220.0f, dividerY }, 4, 5.0f, 0.0f);
    g.setColour (colours::gold);
    g.fillPath (diamond);

    {
        juce::ColourGradient separator (colours::gold.withAlpha (0.0f), toneMixSeparatorX, toneTop,
                                        colours::gold.withAlpha (0.0f), toneMixSeparatorX, toneTop + toneKnob + toneCaption, false);
        separator.addColour (0.5, colours::gold.withAlpha (0.35f));
        g.setGradientFill (separator);
        g.fillRect (juce::Rectangle<float> (toneMixSeparatorX - 0.5f, toneTop, 1.0f, toneKnob + toneCaption));
    }

    // Logo.
    juce::GlyphArrangement glyphs;
    glyphs.addFittedText (lookAndFeel.getDisplayFont (54.0f).withExtraKerningFactor (0.06f), "OBLIVIUM",
                          logoArea.getX(), logoArea.getY(), logoArea.getWidth(), logoArea.getHeight(),
                          juce::Justification::centred, 1);
    juce::Path logo;
    glyphs.createPath (logo);

    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.fillPath (logo, juce::AffineTransform::translation (0.0f, 3.0f));
    g.setGradientFill (oblivium::ui::ObliviumLookAndFeel::goldGradient (logo.getBounds()));
    g.fillPath (logo);
    g.setColour (colours::goldLight.withAlpha (0.35f));
    g.strokePath (logo, juce::PathStrokeType (0.5f));

    // Tagline.
    const auto tagline = juce::Rectangle<float> (120.0f, taglineY - 7.0f, 200.0f, 14.0f);
    g.setColour (colours::goldMuted);
    g.setFont (lookAndFeel.getLabelFont (9.5f, true).withExtraKerningFactor (0.42f));
    g.drawText ("VOCAL DISTORTION", tagline, juce::Justification::centred, false);
    drawRule (g, 84.0f, 138.0f, taglineY, true);
    drawRule (g, 302.0f, 356.0f, taglineY, false);
}
