#pragma once

#include "Features.h"
#include "Tile.h"
#include <juce_graphics/juce_graphics.h>
#include <optional>
#include <set>
#include <vector>

namespace game
{

/** Per-AI-player working memory. Plans are cached between ticks and
    invalidated when the world changes underneath them. */
struct AiBrain
{
    // Pending placement: (placeCell, rotation) reached from placeFromCell.
    std::optional<GridCoord> placeCell;
    int                      placeRotation = 0;
    std::optional<GridCoord> placeFromCell;

    // Path of placed cells from current position to wherever we're walking.
    std::vector<GridCoord> path;
    size_t                 pathIdx = 0;

    // Sub-tile target inside the destination cell (for positioning over a
    // specific feature when claiming).
    std::optional<juce::Point<float>> fineTarget;

    // Re-plan throttle so we don't spam BFS when things are unstable.
    float planCooldown = 0.0f;
};

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

    /** Set iff this player is AI. `controllerIndex` stays at -1 in that case. */
    std::optional<AiBrain> ai;
};

} // namespace game
