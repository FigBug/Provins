#pragma once

#include "Tile.h"
#include <JuceHeader.h>
#include <vector>
#include <random>

namespace game
{

/** Owns the canonical TileType set (one per id) and a shuffled draw pile of
    instances reflecting each type's count. */
class TileDeck
{
public:
    /** Parses tiles.json (as bundled in BinaryData). `tileMultiplier` scales
        every tile type's count by N (1..10) so games can be made longer
        without changing the underlying tile distribution. The start tile is
        still reserved as a single copy regardless of multiplier. */
    explicit TileDeck (const juce::String& json,
                       int tileMultiplier = 1,
                       juce::uint32 randomSeed = 0);

    /** Returns the single tile reserved as the start tile (canonical orientation 0). */
    const TileType* getStartTileType() const noexcept   { return startTile; }

    /** Number of tiles still waiting to be drawn. */
    int  size() const noexcept     { return (int) drawPile.size(); }
    bool empty() const noexcept    { return drawPile.empty(); }

    /** Draws the next tile from the pile, or nullptr if empty. */
    const TileType* draw();

    /** Look up a TileType by its id letter ("A".."X"). */
    const TileType* findById (const juce::String& id) const noexcept;

private:
    std::vector<TileType>        types;
    std::vector<const TileType*> drawPile;
    const TileType*              startTile = nullptr;
};

} // namespace game
