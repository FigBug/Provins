#pragma once

#include "Board.h"
#include "Tile.h"

#include <vector>

namespace game
{

/** A reference to a single feature entry on a specific placed tile. Stable
    across board mutations as long as the tile at that cell isn't replaced
    (which we don't do — once placed a tile stays put). */
struct FeatureRef
{
    GridCoord cell;
    int       featureIndex = 0;   // index into PlacedTile::type->features

    bool operator== (const FeatureRef& o) const noexcept
    {
        return cell == o.cell && featureIndex == o.featureIndex;
    }
    bool operator!= (const FeatureRef& o) const noexcept { return ! (*this == o); }
    bool operator<  (const FeatureRef& o) const noexcept
    {
        if (cell < o.cell) return true;
        if (o.cell < cell) return false;
        return featureIndex < o.featureIndex;
    }
};

/** A connected component of like-typed feature segments spanning one or more
    placed tiles. Returned by traceFeature(). */
struct FeatureInstance
{
    FeatureType             type = FeatureType::cloister;
    std::vector<FeatureRef> segments;
    int                     tileCount    = 0;   // distinct tiles in the component
    int                     pennantCount = 0;   // city only
    bool                    isComplete   = false;
};

/** BFS over the board from `start`, walking through every connected segment
    of the same feature type. Sets isComplete = true iff every outgoing edge
    of every visited segment lands on a placed tile (no open exits). */
FeatureInstance traceFeature (const Board& board, FeatureRef start);

/** Score for a feature instance under Carcassonne-style rules, plus our
    real-time interpretation that incomplete features score at half rate:

      - road:     1 pt per tile (complete and incomplete are the same)
      - city:     complete    → 2 pt per tile + 2 pt per pennant
                  incomplete  → 1 pt per tile + 1 pt per pennant
      - cloister: 1 + count of placed neighbour cells (1..9; 9 = complete)

    Cloister needs board context for the neighbour count; the others don't. */
int scoreOf (const FeatureInstance& inst, const Board& board);

} // namespace game
