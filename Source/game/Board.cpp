#include "Board.h"

namespace game
{

void Board::place (GridCoord c, PlacedTile t)
{
    jassert (! isOccupied (c));
    tiles[c] = t;
}

const PlacedTile* Board::at (GridCoord c) const noexcept
{
    auto it = tiles.find (c);
    return it == tiles.end() ? nullptr : &it->second;
}

bool Board::canPlace (GridCoord c, const PlacedTile& candidate) const noexcept
{
    if (isOccupied (c))
        return false;

    bool touchesAny = false;

    for (auto d : allDirections)
    {
        const auto* neighbour = at (c.neighbour (d));
        if (neighbour == nullptr)
            continue;

        touchesAny = true;

        if (candidate.edgeAt (d) != neighbour->edgeAt (opposite (d)))
            return false;
    }

    return touchesAny;
}

std::optional<juce::Rectangle<int>> Board::bounds() const noexcept
{
    if (tiles.empty())
        return {};

    auto it = tiles.begin();
    int minC = it->first.col, maxC = minC;
    int minR = it->first.row, maxR = minR;
    for (++it; it != tiles.end(); ++it)
    {
        minC = std::min (minC, it->first.col);
        maxC = std::max (maxC, it->first.col);
        minR = std::min (minR, it->first.row);
        maxR = std::max (maxR, it->first.row);
    }
    return juce::Rectangle<int> (minC, minR, maxC - minC + 1, maxR - minR + 1);
}

} // namespace game
