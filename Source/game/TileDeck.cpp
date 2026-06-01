#include "TileDeck.h"

#include <algorithm>
#include <array>
#include <set>

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

TileDeck::TileDeck (const juce::String& json, int tileMultiplier, juce::uint32 randomSeed)
{
    tileMultiplier = juce::jlimit (1, 10, tileMultiplier);
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

        // Compute field features using half-edge connectivity.
        // Each non-city edge has two half-edges (left/right). Roads on an
        // edge separate its two half-edges; corner pairs are always connected.
        // Connected components of the half-edge graph = distinct field regions.
        {
            constexpr int N = (int) HalfEdge::count;

            // Which half-edges exist (not on city edges)?
            std::array<bool, N> exists {};
            for (int i = 0; i < N; ++i)
            {
                auto dir = halfEdgeDirection ((HalfEdge) i);
                exists[(size_t) i] = (t.edges[(size_t) dir] != EdgeType::city);
            }

            // Union-Find
            std::array<int, N> parent;
            for (int i = 0; i < N; ++i) parent[(size_t) i] = i;

            auto find = [&] (int x) {
                while (parent[(size_t) x] != x) x = parent[(size_t) x];
                return x;
            };
            auto unite = [&] (int a, int b) {
                parent[(size_t) find (a)] = find (b);
            };

            // Same-edge pairs: connected if edge is field (not road/city)
            struct EdgePair { HalfEdge a, b; Direction dir; };
            constexpr EdgePair edgePairs[] = {
                { HalfEdge::NW, HalfEdge::NE, Direction::north },
                { HalfEdge::EN, HalfEdge::ES, Direction::east  },
                { HalfEdge::SE, HalfEdge::SW, Direction::south },
                { HalfEdge::WS, HalfEdge::WN, Direction::west  },
            };
            for (auto& ep : edgePairs)
                if (exists[(int) ep.a] && exists[(int) ep.b]
                    && t.edges[(size_t) ep.dir] == EdgeType::field)
                    unite ((int) ep.a, (int) ep.b);

            // Corner pairs: always connected if both exist
            struct CornerPair { HalfEdge a, b; };
            constexpr CornerPair corners[] = {
                { HalfEdge::NE, HalfEdge::EN },
                { HalfEdge::ES, HalfEdge::SE },
                { HalfEdge::SW, HalfEdge::WS },
                { HalfEdge::WN, HalfEdge::NW },
            };
            for (auto& cp : corners)
                if (exists[(int) cp.a] && exists[(int) cp.b])
                    unite ((int) cp.a, (int) cp.b);

            // Collect connected components into field features
            std::set<int> roots;
            for (int i = 0; i < N; ++i)
                if (exists[(size_t) i])
                    roots.insert (find (i));

            for (int r : roots)
            {
                Feature f;
                f.type    = FeatureType::field;
                f.pennant = false;
                for (int i = 0; i < N; ++i)
                    if (exists[(size_t) i] && find (i) == r)
                        f.halfEdges.push_back ((HalfEdge) i);
                t.features.push_back (std::move (f));
            }
        }

        types.push_back (std::move (t));
    }

    // Build the draw pile: every TileType contributes (count * tileMultiplier)
    // instances. The start tile is still reserved as a single copy below.
    int totalInstances = 0;
    for (const auto& t : types)
        totalInstances += t.count * tileMultiplier;

    drawPile.reserve ((size_t) totalInstances);
    for (const auto& t : types)
        for (int i = 0; i < t.count * tileMultiplier; ++i)
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
