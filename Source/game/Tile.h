#pragma once

#include <juce_core/juce_core.h>
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

enum class FeatureType { city, road, cloister };

struct Feature
{
    FeatureType type = FeatureType::cloister;
    std::vector<Direction> edges;   // empty for cloister
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
