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
    explicit GameState (const juce::String& tilesJson, juce::uint32 randomSeed = 0);

    const TileDeck& getDeck()  const noexcept   { return deck; }
    const Board&    getBoard() const noexcept   { return board; }

    const std::vector<Player>& getPlayers() const noexcept   { return players; }

    /** Advance the simulation by `dt` seconds: spawn players for any new
        controllers, move meeples, handle aim / rotate / place / claim. */
    void update (float dt, gin::GameControllerManager& controllers);

    /** Live score for player at index `i`. Sums scoreOf() over every distinct
        feature instance that contains any of the player's claimed segments. */
    int computeScore (int playerIndex) const;

    /** True once the deck is empty AND every player's held tile is gone —
        i.e., no further placement is possible. Movement and last-call claims
        keep working; the HUD draws a winner overlay. */
    bool isGameOver() const noexcept;

    /** Add an AI-driven player. Picks the first free colour slot 0..3. */
    void spawnAi();

private:
    bool isClaimedByAnyone (const FeatureRef& ref) const;
    void spawnPlayer       (int controllerIndex);
    int  findFreeSlot      () const noexcept;

    void aiUpdate          (Player& p, AiBrain& brain, float dt);
    bool aiPlanPlacement   (Player& p, AiBrain& brain);
    bool aiPlanClaim       (Player& p, AiBrain& brain);
    void aiMoveAlongPath   (Player& p, AiBrain& brain, float dt);

    TileDeck            deck;
    Board               board;
    std::vector<Player> players;
};

} // namespace game
