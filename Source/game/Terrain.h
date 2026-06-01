#pragma once

#include "Tile.h"
#include <JuceHeader.h>
#include <optional>

namespace game
{

/** Returns the terrain type at a point inside a placed tile.

    `local` is in unit-tile coordinates [0, 1]^2 relative to the tile's
    top-left corner, in world orientation (the function undoes the tile's
    rotation internally before sampling the canonical feature geometry).

    The geometry mirrors what TileRenderer draws — road bands of width 0.10
    from each road edge midpoint to the centre, and city trapezoids of depth
    0.30 from each city edge inward. Anywhere not covered by those is field. */
EdgeType terrainAt (const PlacedTile& tile, juce::Point<float> local) noexcept;

/** Which feature on the tile is at `local`, or nullopt if standing on field.
    Returned `.featureIndex` indexes into PlacedTile::type->features. */
struct TileFeatureSample
{
    FeatureType type;
    int         featureIndex;
};

std::optional<TileFeatureSample>
    sampleTileFeature (const PlacedTile& tile, juce::Point<float> local) noexcept;

} // namespace game
