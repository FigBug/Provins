#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

namespace game
{

enum class Direction : int { north = 0, east = 1, south = 2, west = 3 };

constexpr std::array<Direction, 4> allDirections { Direction::north, Direction::east,
                                                   Direction::south, Direction::west };

inline Direction opposite (Direction d) noexcept
{
    return (Direction) (((int) d + 2) & 3);
}

inline Direction rotateCW (Direction d, int steps) noexcept
{
    return (Direction) (((int) d + steps) & 3);
}

enum class EdgeType { field, road, city };

enum class FeatureType { city, road, cloister, field };

/** Half-edge identifier for field connectivity. Named by edge + side:
    NW = west half of N edge, NE = east half of N edge, etc. */
enum class HalfEdge { NW, NE, EN, ES, SE, SW, WS, WN, count };

/** Which tile edge a half-edge belongs to. */
inline Direction halfEdgeDirection (HalfEdge he) noexcept
{
    switch (he)
    {
        case HalfEdge::NW: case HalfEdge::NE: return Direction::north;
        case HalfEdge::EN: case HalfEdge::ES: return Direction::east;
        case HalfEdge::SE: case HalfEdge::SW: return Direction::south;
        case HalfEdge::WS: case HalfEdge::WN: return Direction::west;
        default: return Direction::north;
    }
}

/** The matching half-edge on the neighboring tile across the shared boundary. */
inline HalfEdge oppositeHalfEdge (HalfEdge he) noexcept
{
    switch (he)
    {
        case HalfEdge::NW: return HalfEdge::SW;
        case HalfEdge::NE: return HalfEdge::SE;
        case HalfEdge::EN: return HalfEdge::WN;
        case HalfEdge::ES: return HalfEdge::WS;
        case HalfEdge::SE: return HalfEdge::NE;
        case HalfEdge::SW: return HalfEdge::NW;
        case HalfEdge::WS: return HalfEdge::ES;
        case HalfEdge::WN: return HalfEdge::EN;
        default: return he;
    }
}

/** Rotate a half-edge CW by `steps` 90-degree increments. */
inline HalfEdge rotateHalfEdgeCW (HalfEdge he, int steps) noexcept
{
    int n = (int) HalfEdge::count;
    return (HalfEdge) ((((int) he + steps * 2) % n + n) % n);
}

struct Feature
{
    FeatureType type = FeatureType::cloister;
    std::vector<Direction> edges;      // for city/road features
    std::vector<HalfEdge>  halfEdges;  // for field features
    bool pennant = false;
};

/** A *type* of tile from tiles.json (id A..X). The deck contains `count` copies. */
struct TileType
{
    juce::String id;
    int count = 0;
    juce::String description;
    std::array<EdgeType, 4> edges { EdgeType::field, EdgeType::field,
                                    EdgeType::field, EdgeType::field };
    std::vector<Feature> features;
};

/** A physical tile placed on the board, with a rotation. */
struct PlacedTile
{
    const TileType* type = nullptr;
    int rotation = 0;   // 0..3, each step is 90° clockwise

    /** Edge type facing `worldDir` after applying this tile's rotation. */
    EdgeType edgeAt (Direction worldDir) const noexcept
    {
        const int canonical = (((int) worldDir - rotation) & 3);
        return type->edges[(size_t) canonical];
    }
};

/** Integer grid coordinate. (0,0) is the start tile. +col is east, +row is south. */
struct GridCoord
{
    int col = 0;
    int row = 0;

    bool operator== (const GridCoord& o) const noexcept { return col == o.col && row == o.row; }
    bool operator!= (const GridCoord& o) const noexcept { return ! (*this == o); }
    bool operator<  (const GridCoord& o) const noexcept
    {
        return std::tie (col, row) < std::tie (o.col, o.row);
    }

    GridCoord neighbour (Direction d) const noexcept
    {
        switch (d)
        {
            case Direction::north: return { col,     row - 1 };
            case Direction::east:  return { col + 1, row     };
            case Direction::south: return { col,     row + 1 };
            case Direction::west:  return { col - 1, row     };
        }
        return *this;
    }
};

} // namespace game
