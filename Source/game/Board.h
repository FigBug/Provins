#pragma once

#include "Tile.h"
#include <JuceHeader.h>
#include <map>
#include <optional>

namespace game
{

/** Sparse infinite grid of placed tiles. Coordinates are integer (col, row). */
class Board
{
public:
    Board() = default;

    /** Place a tile. No validation — caller is responsible for checking canPlace(). */
    void place (GridCoord c, PlacedTile t);

    void remove (GridCoord c);

    const PlacedTile* at (GridCoord c) const noexcept;

    bool isOccupied (GridCoord c) const noexcept   { return at (c) != nullptr; }

    /** Returns true if this position is empty AND every neighbour with a tile
        matches edge types with this candidate, AND at least one neighbour exists. */
    bool canPlace (GridCoord c, const PlacedTile& candidate) const noexcept;

    /** Bounding box of placed tiles. Empty optional if board is empty. */
    std::optional<juce::Rectangle<int>> bounds() const noexcept;

    int size() const noexcept   { return (int) tiles.size(); }

    const std::map<GridCoord, PlacedTile>& placed() const noexcept   { return tiles; }

private:
    std::map<GridCoord, PlacedTile> tiles;
};

} // namespace game
