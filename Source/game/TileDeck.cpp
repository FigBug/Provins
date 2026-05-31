#include "TileDeck.h"

#include <algorithm>

namespace game
{

namespace
{
    Direction parseDirection (const juce::String& s)
    {
        if (s == "N") return Direction::north;
        if (s == "E") return Direction::east;
        if (s == "S") return Direction::south;
        if (s == "W") return Direction::west;
        jassertfalse;
        return Direction::north;
    }

    EdgeType parseEdge (const juce::String& s)
    {
        if (s == "field")    return EdgeType::field;
        if (s == "road")     return EdgeType::road;
        if (s == "city")     return EdgeType::city;
        jassertfalse;
        return EdgeType::field;
    }

    FeatureType parseFeatureType (const juce::String& s)
    {
        if (s == "city")     return FeatureType::city;
        if (s == "road")     return FeatureType::road;
        if (s == "cloister") return FeatureType::cloister;
        jassertfalse;
        return FeatureType::cloister;
    }
}

TileDeck::TileDeck (const juce::String& json, juce::uint32 randomSeed)
{
    const auto parsed = juce::JSON::parse (json);
    jassert (parsed.isObject());

    const auto* root = parsed.getDynamicObject();
    jassert (root != nullptr);

    const auto startId = root->getProperty ("meta")
                              .getDynamicObject()
                              ->getProperty ("startTile").toString();

    const auto tileArr = root->getProperty ("tiles");
    jassert (tileArr.isArray());

    const auto& arr = *tileArr.getArray();
    types.reserve ((size_t) arr.size());

    for (const auto& v : arr)
    {
        auto* obj = v.getDynamicObject();
        jassert (obj != nullptr);

        TileType t;
        t.id          = obj->getProperty ("id").toString();
        t.count       = (int) obj->getProperty ("count");
        t.description = obj->getProperty ("description").toString();

        auto* edgesObj = obj->getProperty ("edges").getDynamicObject();
        jassert (edgesObj != nullptr);
        t.edges[(size_t) Direction::north] = parseEdge (edgesObj->getProperty ("N").toString());
        t.edges[(size_t) Direction::east]  = parseEdge (edgesObj->getProperty ("E").toString());
        t.edges[(size_t) Direction::south] = parseEdge (edgesObj->getProperty ("S").toString());
        t.edges[(size_t) Direction::west]  = parseEdge (edgesObj->getProperty ("W").toString());

        const auto featuresVar = obj->getProperty ("features");
        if (featuresVar.isArray())
        {
            for (const auto& fv : *featuresVar.getArray())
            {
                auto* fobj = fv.getDynamicObject();
                jassert (fobj != nullptr);

                Feature f;
                f.type    = parseFeatureType (fobj->getProperty ("type").toString());
                f.pennant = (bool) fobj->getProperty ("pennant");

                const auto edgesVar = fobj->getProperty ("edges");
                if (edgesVar.isArray())
                    for (const auto& ev : *edgesVar.getArray())
                        f.edges.push_back (parseDirection (ev.toString()));

                t.features.push_back (std::move (f));
            }
        }

        // Synthesize one field feature per tile bundling every field-typed
        // edge. Simplified model — a real Carcassonne tile may have multiple
        // field regions when a road bisects the field, but the JSON's meta
        // explicitly omits half-edge geometry and we approximate it as one
        // region per tile. Fields connect across tiles via shared field edges.
        {
            Feature fieldFeature;
            fieldFeature.type    = FeatureType::field;
            fieldFeature.pennant = false;
            for (auto d : allDirections)
                if (t.edges[(size_t) d] == EdgeType::field)
                    fieldFeature.edges.push_back (d);
            if (! fieldFeature.edges.empty())
                t.features.push_back (std::move (fieldFeature));
        }

        types.push_back (std::move (t));
    }

    // Build the draw pile: every TileType contributes `count` instances.
    int totalInstances = 0;
    for (const auto& t : types)
        totalInstances += t.count;

    drawPile.reserve ((size_t) totalInstances);
    for (const auto& t : types)
        for (int i = 0; i < t.count; ++i)
            drawPile.push_back (&t);

    // Reserve the start tile (one copy of the type whose id == meta.startTile).
    startTile = findById (startId);
    jassert (startTile != nullptr);
    {
        auto it = std::find (drawPile.begin(), drawPile.end(), startTile);
        jassert (it != drawPile.end());
        drawPile.erase (it);
    }

    // Shuffle.
    std::mt19937 rng (randomSeed != 0 ? randomSeed : std::random_device{}());
    std::shuffle (drawPile.begin(), drawPile.end(), rng);
}

const TileType* TileDeck::draw()
{
    if (drawPile.empty())
        return nullptr;

    const auto* t = drawPile.back();
    drawPile.pop_back();
    return t;
}

const TileType* TileDeck::findById (const juce::String& id) const noexcept
{
    for (const auto& t : types)
        if (t.id == id)
            return &t;
    return nullptr;
}

} // namespace game
