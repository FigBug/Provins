#include "Features.h"

#include <queue>
#include <set>

namespace game
{

namespace
{
    /** Given a placed tile, find which feature of `type` contains `canonicalEdge`
        (i.e., the edge already in the tile's canonical coord system). Returns
        -1 if no such feature exists. */
    int findFeatureWithEdge (const PlacedTile& tile,
                             FeatureType type,
                             Direction canonicalEdge)
    {
        for (size_t fi = 0; fi < tile.type->features.size(); ++fi)
        {
            const auto& f = tile.type->features[fi];
            if (f.type != type)
                continue;
            for (auto e : f.edges)
                if (e == canonicalEdge)
                    return (int) fi;
        }
        return -1;
    }
}

FeatureInstance traceFeature (const Board& board, FeatureRef start)
{
    FeatureInstance inst;

    const auto* startTile = board.at (start.cell);
    if (startTile == nullptr
        || start.featureIndex < 0
        || start.featureIndex >= (int) startTile->type->features.size())
        return inst;

    inst.type       = startTile->type->features[(size_t) start.featureIndex].type;
    inst.isComplete = true;   // flipped to false on the first open edge

    std::set<FeatureRef> visited { start };
    std::set<GridCoord>  cells;
    std::queue<FeatureRef> q;
    q.push (start);

    while (! q.empty())
    {
        const auto cur = q.front();
        q.pop();
        inst.segments.push_back (cur);

        const auto* tile = board.at (cur.cell);
        // Invariant: only push refs we know are placed.
        jassert (tile != nullptr);

        cells.insert (cur.cell);

        const auto& feat = tile->type->features[(size_t) cur.featureIndex];
        if (feat.pennant)
            ++inst.pennantCount;

        // Each canonical edge of this feature exits one face of the tile (in
        // world coordinates) — follow it to the neighbour and merge their
        // matching feature into this instance.
        for (auto canonicalEdge : feat.edges)
        {
            const auto worldEdge    = rotateCW (canonicalEdge, tile->rotation);
            const auto neighbourPos = cur.cell.neighbour (worldEdge);
            const auto* nbrTile     = board.at (neighbourPos);

            if (nbrTile == nullptr)
            {
                inst.isComplete = false;
                continue;
            }

            // The world edge facing back into us is the opposite direction;
            // convert that into the neighbour's canonical edge space.
            const auto incomingWorld     = opposite (worldEdge);
            const auto incomingCanonical = rotateCW (incomingWorld, -nbrTile->rotation & 3);

            const int nbrFeatureIdx = findFeatureWithEdge (*nbrTile, inst.type, incomingCanonical);
            if (nbrFeatureIdx < 0)
                continue;   // shouldn't happen for a valid placement, but be defensive

            const FeatureRef nextRef { neighbourPos, nbrFeatureIdx };
            if (visited.insert (nextRef).second)
                q.push (nextRef);
        }
    }

    inst.tileCount = (int) cells.size();
    return inst;
}

int scoreOf (const FeatureInstance& inst, const Board& board)
{
    switch (inst.type)
    {
        case FeatureType::road:
            return inst.tileCount;

        case FeatureType::city:
            return inst.isComplete
                     ? 2 * (inst.tileCount + inst.pennantCount)
                     : 1 * (inst.tileCount + inst.pennantCount);

        case FeatureType::cloister:
        {
            if (inst.segments.empty())
                return 0;

            const auto centre = inst.segments.front().cell;
            int n = 1;   // the cloister tile itself
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    if ((dx != 0 || dy != 0)
                        && board.isOccupied ({ centre.col + dx, centre.row + dy }))
                        ++n;
            return n;
        }

        case FeatureType::field:
        {
            // Field score = 3 × the number of distinct *completed* city
            // instances that border this field. Approximated as: any city
            // feature sharing a tile with any segment of this field counts.
            std::set<FeatureRef> completedCityIds;
            for (const auto& seg : inst.segments)
            {
                const auto* tile = board.at (seg.cell);
                if (tile == nullptr)
                    continue;

                for (size_t fi = 0; fi < tile->type->features.size(); ++fi)
                {
                    if (tile->type->features[fi].type != FeatureType::city)
                        continue;

                    const auto cityInst = traceFeature (board, { seg.cell, (int) fi });
                    if (! cityInst.isComplete || cityInst.segments.empty())
                        continue;

                    // Canonical id = smallest segment in the instance — stable
                    // across re-traces.
                    const auto canon = *std::min_element (cityInst.segments.begin(),
                                                          cityInst.segments.end());
                    completedCityIds.insert (canon);
                }
            }
            return 3 * (int) completedCityIds.size();
        }
    }
    return 0;
}

} // namespace game
