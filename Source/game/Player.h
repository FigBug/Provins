#pragma once

#include "Features.h"
#include "Tile.h"
#include <juce_graphics/juce_graphics.h>
#include <optional>
#include <set>

namespace game
{

struct Player
{
    int                controllerIndex = -1;   // -1 = no controller bound (AI later)
    juce::Colour       colour;
    juce::Point<float> position;               // world coordinates (1 unit = 1 tile)

    /** The tile this player is currently holding, ready to place. nullptr when
        the deck is exhausted. */
    const TileType*    heldTile = nullptr;
    int                heldRotation = 0;       // 0..3, 90° CW per step

    /** Cell currently aimed at with the right stick. Recomputed each tick. */
    std::optional<GridCoord> targetCell;
    bool                     targetValid = false;

    /** Most recently placed tile with an open claim window. Set when this
        player places a tile; cleared on successful claim or when they place
        another tile (forfeiting the prior claim opportunity). */
    std::optional<GridCoord> lastPlaced;

    /** Feature currently under the meeple that the player can claim with B.
        Only set when this player is the placer of the tile under them AND
        the underlying feature isn't claimed by anyone. */
    std::optional<FeatureRef> hoveredClaimable;

    /** Persistent claims. There's no return-to-supply — once a player commits
        a claim it stays for the rest of the game (user's "no limit"). */
    std::set<FeatureRef> claims;

    /** Previous-tick button state for edge detection. */
    bool prevPlace     = false;
    bool prevClaim     = false;
    bool prevRotateCW  = false;
    bool prevRotateCCW = false;
};

} // namespace game
