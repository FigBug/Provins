#include "Features.h"

#include <queue>
#include <set>

namespace game
{

namespace
{
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

    int findFieldWithHalfEdge (const PlacedTile& tile, HalfEdge canonicalHE)
    {
        for (size_t fi = 0; fi < tile.type->features.size(); ++fi)
        {
            const auto& f = tile.type->features[fi];
            if (f.type != FeatureType::field)
                continue;
            for (auto he : f.halfEdges)
                if (he == canonicalHE)
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
    inst.isComplete = (inst.type != FeatureType::field);

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

        if (inst.type == FeatureType::field)
        {
            // Field tracing: follow half-edge connections across tile boundaries.
            for (auto canonicalHE : feat.halfEdges)
            {
                auto worldHE = rotateHalfEdgeCW (canonicalHE, tile->rotation);
                auto worldDir = halfEdgeDirection (worldHE);
                auto neighbourPos = cur.cell.neighbour (worldDir);
                const auto* nbrTile = board.at (neighbourPos);

                if (nbrTile == nullptr)
                    continue;

                auto nbrWorldHE       = oppositeHalfEdge (worldHE);
                auto nbrCanonicalHE   = rotateHalfEdgeCW (nbrWorldHE, (-nbrTile->rotation) & 7);

                int nbrFi = findFieldWithHalfEdge (*nbrTile, nbrCanonicalHE);
                if (nbrFi < 0)
                    continue;

                const FeatureRef nextRef { neighbourPos, nbrFi };
                if (visited.insert (nextRef).second)
                    q.push (nextRef);
            }
        }
        else
        {
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

                const auto incomingWorld     = opposite (worldEdge);
                const auto incomingCanonical = rotateCW (incomingWorld, (-nbrTile->rotation) & 3);

                const int nbrFeatureIdx = findFeatureWithEdge (*nbrTile, inst.type, incomingCanonical);
                if (nbrFeatureIdx < 0)
                    continue;

                const FeatureRef nextRef { neighbourPos, nbrFeatureIdx };
                if (visited.insert (nextRef).second)
                    q.push (nextRef);
            }
        }
    }

    // Cloisters have no edges so the BFS never flips isComplete to false.
    // A cloister is complete when all 8 surrounding cells are occupied.
    if (inst.type == FeatureType::cloister && ! inst.segments.empty())
    {
        const auto centre = inst.segments.front().cell;
        inst.isComplete = true;
        for (int dy = -1; dy <= 1 && inst.isComplete; ++dy)
            for (int dx = -1; dx <= 1 && inst.isComplete; ++dx)
                if ((dx != 0 || dy != 0)
                    && ! board.isOccupied ({ centre.col + dx, centre.row + dy }))
                    inst.isComplete = false;
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

juce::Point<float> featureSamplePoint (const Feature& f)
{
    const juce::Point<float> centre { 0.5f, 0.5f };

    auto edgeMidpoint = [] (Direction d) -> juce::Point<float>
    {
        switch (d)
        {
            case Direction::north: return { 0.5f, 0.0f };
            case Direction::east:  return { 1.0f, 0.5f };
            case Direction::south: return { 0.5f, 1.0f };
            case Direction::west:  return { 0.0f, 0.5f };
        }
        return { 0.5f, 0.5f };
    };

    if (f.type == FeatureType::field && ! f.halfEdges.empty())
    {
        auto halfEdgePos = [] (HalfEdge he) -> juce::Point<float>
        {
            switch (he)
            {
                case HalfEdge::NW: return { 0.25f, 0.0f };
                case HalfEdge::NE: return { 0.75f, 0.0f };
                case HalfEdge::EN: return { 1.0f,  0.25f };
                case HalfEdge::ES: return { 1.0f,  0.75f };
                case HalfEdge::SE: return { 0.75f, 1.0f };
                case HalfEdge::SW: return { 0.25f, 1.0f };
                case HalfEdge::WS: return { 0.0f,  0.75f };
                case HalfEdge::WN: return { 0.0f,  0.25f };
                default: return { 0.5f, 0.5f };
            }
        };

        juce::Point<float> avg { 0.0f, 0.0f };
        for (auto he : f.halfEdges)
            avg += halfEdgePos (he);
        avg = { avg.x / (float) f.halfEdges.size(), avg.y / (float) f.halfEdges.size() };
        return avg + (centre - avg) * 0.25f;
    }

    if (f.type == FeatureType::cloister || f.edges.empty())
        return centre;

    juce::Point<float> avg { 0.0f, 0.0f };
    for (auto e : f.edges)
        avg += edgeMidpoint (e);
    avg = { avg.x / (float) f.edges.size(), avg.y / (float) f.edges.size() };

    float t = 0.5f;
    return avg + (centre - avg) * t;
}

juce::Point<float> rotateLocalCW (juce::Point<float> p, int steps) noexcept
{
    steps &= 3;
    while (steps-- > 0)
    {
        const float dx = p.x - 0.5f;
        const float dy = p.y - 0.5f;
        p = { 0.5f - dy, 0.5f + dx };
    }
    return p;
}

} // namespace game
