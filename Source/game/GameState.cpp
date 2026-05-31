#include "GameState.h"

namespace game
{

GameState::GameState (const juce::String& tilesJson, juce::uint32 randomSeed)
    : deck (tilesJson, randomSeed)
{
    // Place the reserved start tile at the origin in its canonical orientation.
    PlacedTile start;
    start.type     = deck.getStartTileType();
    start.rotation = 0;
    board.place ({ 0, 0 }, start);
}

} // namespace game
