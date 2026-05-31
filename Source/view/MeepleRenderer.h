#pragma once

#include <juce_graphics/juce_graphics.h>

namespace view
{

/** Draws a meeple silhouette centred at `position`, sized so it fits within
    `worldSize` world units. World coordinates: 1 unit = 1 tile. */
class MeepleRenderer
{
public:
    static void draw (juce::Graphics& g,
                      juce::Point<float> position,
                      juce::Colour colour,
                      float worldSize = 0.35f);
};

} // namespace view
