#pragma once

#include "../game/Tile.h"
#include <JuceHeader.h>

namespace view
{

/** Stateless procedural drawer for a single PlacedTile. The canonical tile is
    rendered into the unit square (0,0)-(1,1) — the caller is expected to scale
    the Graphics context appropriately before invoking. */
class TileRenderer
{
public:
    static void draw (juce::Graphics& g, const game::PlacedTile& tile);

    // Colour palette — kept public so other views (HUD, minimap) can match.
    static juce::Colour fieldColour();
    static juce::Colour roadColour();
    static juce::Colour cityColour();
    static juce::Colour borderColour();
};

} // namespace view
