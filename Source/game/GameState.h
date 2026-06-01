#pragma once

#include "Board.h"
#include "Player.h"
#include "TileDeck.h"

#include <vector>

namespace gin { class GameControllerManager; }

namespace game
{

/** Top-level container. Owns the deck, board, and players. */
class GameState
{
public:
    explicit GameState (const juce::String& tilesJson, int maxPlayers = 4, juce::uint32 randomSeed = 0);

    const TileDeck& getDeck()  const noexcept   { return deck; }
    const Board&    getBoard() const noexcept   { return board; }

    const std::vector<Player>& getPlayers() const noexcept   { return players; }

    /** Advance the simulation by `dt` seconds: spawn players for any new
        controllers, move meeples, handle aim / rotate / place / claim. */
    void update (float dt, gin::GameControllerManager& controllers);

    int computeScore (int playerIndex) const;

    struct ScoreBreakdown
    {
        int farms           = 0;
        int cloisters       = 0;
        int roads           = 0;
        int citiesComplete  = 0;
        int citiesIncomplete = 0;
        int tilesPlaced     = 0;
        int total() const noexcept { return farms + cloisters + roads + citiesComplete + citiesIncomplete; }
    };

    ScoreBreakdown computeScoreBreakdown (int playerIndex) const;

    bool isGameOver() const noexcept;
    bool allTilesPlaced() const noexcept;
    float getEndTimer() const noexcept { return endTimer; }

    /** Add an AI-driven player. Picks the first free colour slot 0..3. */
    void spawnAi();

    /** Add an AI-driven player at a specific colour slot (0..3). */
    void spawnAi (int slot);

    void spawnPlayer (int controllerIndex);

private:
    bool isClaimedByAnyone (const FeatureRef& ref) const;
    void returnCompletedMeeples();
    int  findFreeSlot      () const noexcept;

    void aiUpdate          (Player& p, AiBrain& brain, float dt);
    bool aiPlanPlacement   (Player& p, AiBrain& brain);
    bool aiPlanClaim       (Player& p, AiBrain& brain);
    void aiMoveAlongPath   (Player& p, AiBrain& brain, float dt);

    TileDeck            deck;
    Board               board;
    std::vector<Player> players;
    int                 maxPlayers = 4;
    float               endTimer   = -1.0f;
};

} // namespace game
