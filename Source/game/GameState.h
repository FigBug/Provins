#pragma once

#include "Board.h"
#include "TileDeck.h"

namespace game
{

/** Top-level container. Owns the deck and the board. Phase 1: just sets up
    the start tile at the origin. Later phases add players, scoring, etc. */
class GameState
{
public:
    explicit GameState (const juce::String& tilesJson, juce::uint32 randomSeed = 0);

    const TileDeck& getDeck()  const noexcept   { return deck; }
    const Board&    getBoard() const noexcept   { return board; }

private:
    TileDeck deck;
    Board    board;
};

} // namespace game
