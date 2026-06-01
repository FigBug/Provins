#include "TileRenderer.h"

namespace view
{

using game::Direction;
using game::EdgeType;
using game::Feature;
using game::FeatureType;
using game::PlacedTile;

juce::Colour TileRenderer::fieldColour()  { return juce::Colour::fromRGB (130, 158,  86); }
juce::Colour TileRenderer::roadColour()   { return juce::Colour::fromRGB (210, 188, 150); }
juce::Colour TileRenderer::cityColour()   { return juce::Colour::fromRGB (151, 102,  59); }
juce::Colour TileRenderer::borderColour() { return juce::Colour::fromRGB ( 40,  40,  40); }

namespace
{
    // Unit-square geometry helpers. (0,0) is top-left of the tile. Directions
    // are canonical (rotation already applied to the Graphics context).
    constexpr float kTile     = 1.0f;
    constexpr float kHalf     = 0.5f;
    constexpr float kCityDepth          = 0.30f;
    constexpr float kRoadWidth          = 0.10f;
    constexpr float kIntersectionRadius = 0.10f;

    juce::Point<float> edgeMidpoint (Direction d) noexcept
    {
        switch (d)
        {
            case Direction::north: return { kHalf, 0.0f  };
            case Direction::east:  return { kTile, kHalf };
            case Direction::south: return { kHalf, kTile };
            case Direction::west:  return { 0.0f,  kHalf };
        }
        return { kHalf, kHalf };
    }

    // trimStart/trimEnd: inset the trapezoid at the CW-start / CW-end corner
    // to create a visible gap between disconnected city edges.
    void fillCityPatch (juce::Graphics& g, Direction d,
                        bool trimStart = false, bool trimEnd = false)
    {
        const float t = kCityDepth;
        juce::Path p;
        switch (d)
        {
            case Direction::north:
                p.startNewSubPath (trimStart ? t : 0.0f,  0.0f);
                p.lineTo (trimEnd ? kTile - t : kTile, 0.0f);
                p.lineTo (kTile - t, t);
                p.lineTo (t,         t);
                break;
            case Direction::east:
                p.startNewSubPath (kTile, trimStart ? t : 0.0f);
                p.lineTo (kTile, trimEnd ? kTile - t : kTile);
                p.lineTo (kTile - t, kTile - t);
                p.lineTo (kTile - t, t);
                break;
            case Direction::south:
                p.startNewSubPath (trimStart ? kTile - t : kTile, kTile);
                p.lineTo (trimEnd ? t : 0.0f,  kTile);
                p.lineTo (t,         kTile - t);
                p.lineTo (kTile - t, kTile - t);
                break;
            case Direction::west:
                p.startNewSubPath (0.0f, trimStart ? kTile - t : kTile);
                p.lineTo (0.0f, trimEnd ? t : 0.0f);
                p.lineTo (t, t);
                p.lineTo (t, kTile - t);
                break;
        }
        p.closeSubPath();
        g.fillPath (p);
    }

    juce::Point<float> innerCorner (Direction d)
    {
        switch (d)
        {
            case Direction::north: return { kCityDepth,         kCityDepth };
            case Direction::east:  return { kTile - kCityDepth, kCityDepth };
            case Direction::south: return { kTile - kCityDepth, kTile - kCityDepth };
            case Direction::west:  return { kCityDepth,         kTile - kCityDepth };
        }
        return { kHalf, kHalf };
    }

    juce::Point<float> outerCorner (Direction d)
    {
        switch (d)
        {
            case Direction::north: return { 0.0f,  0.0f };
            case Direction::east:  return { kTile, 0.0f };
            case Direction::south: return { kTile, kTile };
            case Direction::west:  return { 0.0f,  kTile };
        }
        return { kHalf, kHalf };
    }

    void fillCityCorner (juce::Graphics& g, Direction d1, Direction d2)
    {
        auto ic1 = innerCorner (d1);
        auto ic2 = innerCorner (d2);

        Direction cornerDir;
        if ((d1 == Direction::north && d2 == Direction::east)
         || (d1 == Direction::east  && d2 == Direction::north))
            cornerDir = Direction::east;
        else if ((d1 == Direction::east  && d2 == Direction::south)
              || (d1 == Direction::south && d2 == Direction::east))
            cornerDir = Direction::south;
        else if ((d1 == Direction::south && d2 == Direction::west)
              || (d1 == Direction::west  && d2 == Direction::south))
            cornerDir = Direction::west;
        else
            cornerDir = Direction::north;

        auto oc = outerCorner (cornerDir);

        juce::Path p;
        p.startNewSubPath (ic1);
        p.lineTo (oc);
        p.lineTo (ic2);
        p.closeSubPath();
        g.fillPath (p);
    }

    void drawRoadSegment (juce::Graphics& g, Direction d, bool stopShort)
    {
        const auto from = edgeMidpoint (d);
        const juce::Point<float> centre { kHalf, kHalf };

        auto to = centre;
        if (stopShort)
        {
            auto diff = centre - from;
            float len = diff.getDistanceFromOrigin();
            float shortenBy = kIntersectionRadius + kRoadWidth * 0.5f;
            if (len > shortenBy)
                to = from + diff * ((len - shortenBy) / len);
        }

        g.drawLine (from.x, from.y, to.x, to.y, kRoadWidth);
    }

    void drawIntersection (juce::Graphics& g)
    {
        g.setColour (TileRenderer::roadColour());
        g.fillEllipse (kHalf - kIntersectionRadius, kHalf - kIntersectionRadius,
                       kIntersectionRadius * 2.0f, kIntersectionRadius * 2.0f);
        g.setColour (TileRenderer::borderColour());
        g.drawEllipse (kHalf - kIntersectionRadius, kHalf - kIntersectionRadius,
                       kIntersectionRadius * 2.0f, kIntersectionRadius * 2.0f, 0.012f);
    }

    void drawCloister (juce::Graphics& g)
    {
        constexpr float s = 0.18f;
        juce::Rectangle<float> r (kHalf - s, kHalf - s, 2.0f * s, 2.0f * s);
        g.setColour (TileRenderer::roadColour());
        g.fillRect (r);
        g.setColour (TileRenderer::borderColour());
        g.drawRect (r, 0.01f);
        g.drawLine (kHalf, kHalf - s, kHalf, kHalf - s - 0.06f, 0.012f);   // small steeple line
    }

    void drawPennant (juce::Graphics& g, Direction firstEdge)
    {
        // Small dot near the city's "inward" centroid as a pennant marker.
        const auto m = edgeMidpoint (firstEdge);
        const juce::Point<float> centre { kHalf, kHalf };
        const auto dot = m + (centre - m) * 0.5f;
        const float r = 0.04f;
        g.setColour (juce::Colours::gold);
        g.fillEllipse (dot.x - r, dot.y - r, 2 * r, 2 * r);
    }
}

void TileRenderer::draw (juce::Graphics& g, const PlacedTile& tile)
{
    jassert (tile.type != nullptr);

    juce::Graphics::ScopedSaveState save (g);

    // Apply rotation around the tile centre. After this transform, we draw the
    // canonical (rotation=0) layout — directions in feature.edges are the
    // tile's own edges, not world directions.
    if (tile.rotation != 0)
        g.addTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::halfPi * (float) tile.rotation,
                                                         kHalf, kHalf));

    // Background — field colour everywhere.
    g.setColour (fieldColour());
    g.fillRect (juce::Rectangle<float> (0.0f, 0.0f, kTile, kTile));

    // Determine which city edges are connected to their CW/CCW neighbor.
    // An edge needs trimming at a corner if the adjacent edge is also city
    // but belongs to a DIFFERENT feature (disconnected).
    auto connectedAtCorner = [&] (Direction d1, Direction d2) -> bool
    {
        if (tile.type->edges[(size_t) d1] != EdgeType::city
         || tile.type->edges[(size_t) d2] != EdgeType::city)
            return false;

        for (const auto& f : tile.type->features)
        {
            if (f.type != FeatureType::city)
                continue;
            bool has1 = false, has2 = false;
            for (auto e : f.edges)
            {
                if (e == d1) has1 = true;
                if (e == d2) has2 = true;
            }
            if (has1 && has2) return true;
        }
        return false;
    };

    auto needsTrim = [&] (Direction d, Direction adjacent) -> bool
    {
        return tile.type->edges[(size_t) adjacent] == EdgeType::city
            && ! connectedAtCorner (d, adjacent);
    };

    g.setColour (cityColour());
    for (auto d : game::allDirections)
    {
        if (tile.type->edges[(size_t) d] != EdgeType::city)
            continue;

        auto prev = game::rotateCW (d, 3);
        auto next = game::rotateCW (d, 1);
        fillCityPatch (g, d, needsTrim (d, prev), needsTrim (d, next));
    }

    for (const auto& f : tile.type->features)
    {
        if (f.type != FeatureType::city || f.edges.size() < 2)
            continue;

        for (size_t i = 0; i < f.edges.size(); ++i)
        {
            for (size_t j = i + 1; j < f.edges.size(); ++j)
            {
                auto d1 = f.edges[i], d2 = f.edges[j];
                int diff = ((int) d2 - (int) d1 + 4) & 3;
                if (diff == 1 || diff == 3)
                    fillCityCorner (g, d1, d2);
                else if (diff == 2)
                    g.fillRect (juce::Rectangle<float> (kCityDepth, kCityDepth,
                                                        kTile - 2.0f * kCityDepth,
                                                        kTile - 2.0f * kCityDepth));
            }
        }
    }

    // Count total road edges to detect intersections (3+ = junction).
    int roadEdgeCount = 0;
    for (const auto& f : tile.type->features)
        if (f.type == FeatureType::road)
            roadEdgeCount += (int) f.edges.size();

    bool hasIntersection = roadEdgeCount >= 3;

    g.setColour (roadColour());
    for (const auto& f : tile.type->features)
    {
        if (f.type != FeatureType::road)
            continue;
        for (auto d : f.edges)
            drawRoadSegment (g, d, hasIntersection);
    }

    if (hasIntersection)
        drawIntersection (g);

    // Cloister marker.
    for (const auto& f : tile.type->features)
        if (f.type == FeatureType::cloister)
            drawCloister (g);

    // Pennant markers on connected city features.
    for (const auto& f : tile.type->features)
        if (f.type == FeatureType::city && f.pennant && ! f.edges.empty())
            drawPennant (g, f.edges.front());

    // Border.
    g.setColour (borderColour());
    g.drawRect (juce::Rectangle<float> (0.0f, 0.0f, kTile, kTile), 0.01f);
}

} // namespace view
