#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace oblivium::ui
{
/** Procedural black marble, generated once and shared by every open editor
    (hold a juce::SharedResourcePointer<MarbleTexture>). */
struct MarbleTexture
{
    MarbleTexture();

    /** Covers the pedal enclosure; draw it stretched over the enclosure area. */
    juce::Image image;
};

/** Thin kintsugi-style gold cracks running through the stone.
    The pattern is fixed, laid out in the editor's base coordinates. */
void drawGoldCracks (juce::Graphics&);
}
