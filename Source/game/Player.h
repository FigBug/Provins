#pragma once

#include "Features.h"
#include "Tile.h"
#include <JuceHeader.h>
#include <optional>
#include <map>
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

    float planCooldown = 0.0f;
    float placeDelay   = 0.0f;
    float claimDelay   = 0.0f;
    float stuckTimer   = 0.0f;
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

    static constexpr int kMaxMeeples = 7;

    struct Claim
    {
        juce::Point<float> position;
        bool               meepleReturned = false;
    };

    std::map<FeatureRef, Claim> claims;
    int                         meepleSupply = kMaxMeeples;

    int meeplesAvailable() const noexcept { return meepleSupply; }

    bool prevPlace     = false;
    bool prevClaim     = false;
    bool prevRotateCW  = false;
    bool prevRotateCCW = false;
    bool aimSuppressed = false;

    /** Set iff this player is AI. `controllerIndex` stays at -1 in that case. */
    std::optional<AiBrain> ai;
};

} // namespace game
