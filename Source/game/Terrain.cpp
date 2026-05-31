#include "Terrain.h"

#include <juce_core/juce_core.h>

namespace game
{

namespace
{
    // NB: keep in sync with TileRenderer.cpp's kRoadWidth (full stroke width)
    // and kCityDepth.
    constexpr float kRoadHalfWidth = 0.05f;
    constexpr float kCityDepth     = 0.30f;

    /** Rotate `p` counter-clockwise by `steps` 90° increments around the tile
        centre (0.5, 0.5). Used to convert a world-oriented local position into
        the tile's canonical coordinate system. */
    juce::Point<float> rotateLocalCCW (juce::Point<float> p, int steps) noexcept
    {
        steps &= 3;
        while (steps-- > 0)
        {
            const float dx = p.x - 0.5f;
            const float dy = p.y - 0.5f;
            p = { 0.5f + dy, 0.5f - dx };
        }
        return p;
    }

    juce::Point<float> edgeMidpoint (Direction d) noexcept
    {
        switch (d)
        {
            case Direction::north: return { 0.5f, 0.0f };
            case Direction::east:  return { 1.0f, 0.5f };
            case Direction::south: return { 0.5f, 1.0f };
            case Direction::west:  return { 0.0f, 0.5f };
        }
        return { 0.5f, 0.5f };
    }

    float distSqToSegment (juce::Point<float> p,
                           juce::Point<float> a,
                           juce::Point<float> b) noexcept
    {
        const auto ab = b - a;
        const auto ap = p - a;
        const float denom = ab.x * ab.x + ab.y * ab.y;
        if (denom < 1.0e-9f)
            return ap.x * ap.x + ap.y * ap.y;

        const float t = juce::jlimit (0.0f, 1.0f, (ap.x * ab.x + ap.y * ab.y) / denom);
        const auto closest = a + ab * t;
        const auto d = p - closest;
        return d.x * d.x + d.y * d.y;
    }

    bool inCityTrapezoid (juce::Point<float> p, Direction edge) noexcept
    {
        // depth = how far inward from `edge`, lateral = position along `edge`.
        float depth = 0.0f, lateral = 0.0f;
        switch (edge)
        {
            case Direction::north: depth = p.y;        lateral = p.x; break;
            case Direction::east:  depth = 1.0f - p.x; lateral = p.y; break;
            case Direction::south: depth = 1.0f - p.y; lateral = p.x; break;
            case Direction::west:  depth = p.x;        lateral = p.y; break;
        }
        return depth >= 0.0f
            && depth <= kCityDepth
            && lateral >= depth
            && lateral <= 1.0f - depth;
    }
}

namespace
{
    // Cloister occupies a small square at the centre; matches the visual.
    constexpr float kCloisterHalfSide = 0.18f;

    bool inCloisterSquare (juce::Point<float> p) noexcept
    {
        return std::abs (p.x - 0.5f) <= kCloisterHalfSide
            && std::abs (p.y - 0.5f) <= kCloisterHalfSide;
    }
}

EdgeType terrainAt (const PlacedTile& tile, juce::Point<float> local) noexcept
{
    if (tile.type == nullptr)
        return EdgeType::field;

    const auto canonical = rotateLocalCCW (local, tile.rotation);

    // Roads first — they're drawn on top of fields in the renderer, so they win
    // any overlap test at boundaries.
    for (const auto& f : tile.type->features)
    {
        if (f.type != FeatureType::road)
            continue;
        for (auto edge : f.edges)
        {
            const auto a = edgeMidpoint (edge);
            const juce::Point<float> b { 0.5f, 0.5f };
            if (distSqToSegment (canonical, a, b) <= kRoadHalfWidth * kRoadHalfWidth)
                return EdgeType::road;
        }
    }

    // City patches: one trapezoid per city-typed edge.
    for (auto edge : allDirections)
    {
        if (tile.type->edges[(size_t) edge] != EdgeType::city)
            continue;
        if (inCityTrapezoid (canonical, edge))
            return EdgeType::city;
    }

    return EdgeType::field;
}

std::optional<TileFeatureSample>
    sampleTileFeature (const PlacedTile& tile, juce::Point<float> local) noexcept
{
    if (tile.type == nullptr)
        return std::nullopt;

    const auto canonical = rotateLocalCCW (local, tile.rotation);

    // Roads — pick the nearest road segment within the road band.
    int   bestRoadFi = -1;
    float bestRoadD  = kRoadHalfWidth * kRoadHalfWidth;
    for (size_t fi = 0; fi < tile.type->features.size(); ++fi)
    {
        const auto& f = tile.type->features[fi];
        if (f.type != FeatureType::road)
            continue;
        for (auto edge : f.edges)
        {
            const float d = distSqToSegment (canonical, edgeMidpoint (edge), { 0.5f, 0.5f });
            if (d <= bestRoadD)
            {
                bestRoadD  = d;
                bestRoadFi = (int) fi;
            }
        }
    }
    if (bestRoadFi >= 0)
        return TileFeatureSample { FeatureType::road, bestRoadFi };

    // Cities — find which city edge's trapezoid contains the point, then find
    // the feature that includes that edge.
    for (auto edge : allDirections)
    {
        if (tile.type->edges[(size_t) edge] != EdgeType::city)
            continue;
        if (! inCityTrapezoid (canonical, edge))
            continue;
        for (size_t fi = 0; fi < tile.type->features.size(); ++fi)
        {
            const auto& f = tile.type->features[fi];
            if (f.type != FeatureType::city)
                continue;
            for (auto e : f.edges)
                if (e == edge)
                    return TileFeatureSample { FeatureType::city, (int) fi };
        }
    }

    // Cloister — central square only.
    if (inCloisterSquare (canonical))
    {
        for (size_t fi = 0; fi < tile.type->features.size(); ++fi)
            if (tile.type->features[fi].type == FeatureType::cloister)
                return TileFeatureSample { FeatureType::cloister, (int) fi };
    }

    // Field — fallback to the synthesized field feature (if the tile has one).
    for (size_t fi = 0; fi < tile.type->features.size(); ++fi)
        if (tile.type->features[fi].type == FeatureType::field)
            return TileFeatureSample { FeatureType::field, (int) fi };

    return std::nullopt;
}

} // namespace game
