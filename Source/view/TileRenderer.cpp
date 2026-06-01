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

    void fillCityPatch (juce::Graphics& g, Direction d)
    {
        // Trapezoid from the edge to a smaller line `kCityDepth` inward.
        juce::Path p;
        switch (d)
        {
            case Direction::north:
                p.startNewSubPath (0.0f, 0.0f);
                p.lineTo (kTile, 0.0f);
                p.lineTo (kTile - kCityDepth, kCityDepth);
                p.lineTo (kCityDepth,         kCityDepth);
                break;
            case Direction::east:
                p.startNewSubPath (kTile, 0.0f);
                p.lineTo (kTile, kTile);
                p.lineTo (kTile - kCityDepth, kTile - kCityDepth);
                p.lineTo (kTile - kCityDepth, kCityDepth);
                break;
            case Direction::south:
                p.startNewSubPath (kTile, kTile);
                p.lineTo (0.0f,  kTile);
                p.lineTo (kCityDepth,         kTile - kCityDepth);
                p.lineTo (kTile - kCityDepth, kTile - kCityDepth);
                break;
            case Direction::west:
                p.startNewSubPath (0.0f, kTile);
                p.lineTo (0.0f, 0.0f);
                p.lineTo (kCityDepth, kCityDepth);
                p.lineTo (kCityDepth, kTile - kCityDepth);
                break;
        }
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

    // City patches: one per city-typed edge. (For connected multi-edge cities
    // this gives the visual impression of one larger blob; refining later.)
    g.setColour (cityColour());
    for (auto d : game::allDirections)
        if (tile.type->edges[(size_t) d] == EdgeType::city)
            fillCityPatch (g, d);

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
