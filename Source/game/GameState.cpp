#include "GameState.h"

#include "Features.h"
#include "Terrain.h"

#include <gin_controllers/gin_controllers.h>

#include <cmath>

namespace game
{

namespace
{
    // Per-terrain max speed in tiles per second.
    constexpr float kSpeedRoad  = 2.6f;
    constexpr float kSpeedCity  = 1.4f;
    constexpr float kSpeedField = 0.75f;

    constexpr float kAimDeadzone = 0.30f;

    constexpr int kMaxPlayers = 4;

    // Per-port colours and initial spawn offsets inside the start tile so
    // multiple meeples don't perfectly overlap on game start.
    const juce::Colour kPortColours[kMaxPlayers] {
        juce::Colour::fromRGB (230,  70,  70),   // red
        juce::Colour::fromRGB ( 70, 140, 230),   // blue
        juce::Colour::fromRGB ( 80, 200,  90),   // green
        juce::Colour::fromRGB (240, 200,  60),   // yellow
    };
    const juce::Point<float> kPortSpawnOffset[kMaxPlayers] {
        { 0.30f, 0.30f },
        { 0.70f, 0.30f },
        { 0.30f, 0.70f },
        { 0.70f, 0.70f },
    };

    float terrainSpeed (EdgeType t) noexcept
    {
        switch (t)
        {
            case EdgeType::road: return kSpeedRoad;
            case EdgeType::city: return kSpeedCity;
            case EdgeType::field: default: return kSpeedField;
        }
    }

    GridCoord cellOf (juce::Point<float> p) noexcept
    {
        return { (int) std::floor (p.x), (int) std::floor (p.y) };
    }
}

GameState::GameState (const juce::String& tilesJson, juce::uint32 randomSeed)
    : deck (tilesJson, randomSeed)
{
    PlacedTile start;
    start.type     = deck.getStartTileType();
    start.rotation = 0;
    board.place ({ 0, 0 }, start);
}

void GameState::spawnPlayer (int controllerIndex)
{
    jassert (controllerIndex >= 0 && controllerIndex < kMaxPlayers);

    Player p;
    p.controllerIndex = controllerIndex;
    p.colour          = kPortColours[controllerIndex];
    p.position        = kPortSpawnOffset[controllerIndex];   // staggered on the start tile
    p.heldTile        = deck.draw();
    p.heldRotation    = 0;
    players.push_back (p);
}

bool GameState::isGameOver() const noexcept
{
    if (! deck.empty())
        return false;
    for (const auto& p : players)
        if (p.heldTile != nullptr)
            return false;
    return ! players.empty();
}

/** Returns true if any segment of the instance containing `ref` is already
    in any player's claims set. */
bool GameState::isClaimedByAnyone (const FeatureRef& ref) const
{
    const auto inst = traceFeature (board, ref);
    std::set<FeatureRef> instSegs (inst.segments.begin(), inst.segments.end());

    for (const auto& player : players)
        for (const auto& claim : player.claims)
            if (instSegs.count (claim) != 0)
                return true;

    return false;
}

void GameState::update (float dt, gin::GameControllerManager& controllers)
{
    if (dt <= 0.0f)
        return;

    dt = juce::jmin (dt, 0.1f);

    using B = gin::GameController::Button;
    using A = gin::GameController::Axis;

    // ---- Hot-join: any connected controller without a player gets one.
    for (int port = 0; port < kMaxPlayers; ++port)
    {
        auto* c = controllers.getController (port);
        if (c == nullptr || ! c->isConnected())
            continue;

        bool alreadyHasPlayer = false;
        for (const auto& existing : players)
            if (existing.controllerIndex == port)
                { alreadyHasPlayer = true; break; }

        if (! alreadyHasPlayer)
            spawnPlayer (port);
    }

    for (auto& p : players)
    {
        if (p.controllerIndex < 0)
            continue;

        auto* c = controllers.getController (p.controllerIndex);
        if (c == nullptr || ! c->isConnected())
            continue;

        // ---- Movement: speed scales by terrain at the meeple's current point.
        const float lx = c->getAxis (A::leftX);
        const float ly = c->getAxis (A::leftY);

        const GridCoord currentCell = cellOf (p.position);
        const auto*     currentTile = board.at (currentCell);

        float speed = kSpeedField;
        if (currentTile != nullptr)
        {
            const juce::Point<float> local { p.position.x - (float) currentCell.col,
                                             p.position.y - (float) currentCell.row };
            speed = terrainSpeed (terrainAt (*currentTile, local));
        }

        const float dx = lx * speed * dt;
        const float dy = ly * speed * dt;

        auto tryAxis = [&] (float ax, float ay)
        {
            const juce::Point<float> candidate { p.position.x + ax, p.position.y + ay };
            if (board.isOccupied (cellOf (candidate)))
                p.position = candidate;
        };
        tryAxis (dx, 0.0f);
        tryAxis (0.0f, dy);

        // ---- Rotation, edge-triggered
        const bool rotateCW  = c->isButtonDown (B::rightShoulder);
        const bool rotateCCW = c->isButtonDown (B::leftShoulder);
        if (rotateCW  && ! p.prevRotateCW)  p.heldRotation = (p.heldRotation + 1) & 3;
        if (rotateCCW && ! p.prevRotateCCW) p.heldRotation = (p.heldRotation + 3) & 3;
        p.prevRotateCW  = rotateCW;
        p.prevRotateCCW = rotateCCW;

        // ---- Aim with right stick → one of the 8 neighbours of the meeple's current cell.
        const float rx = c->getAxis (A::rightX);
        const float ry = c->getAxis (A::rightY);

        std::optional<GridCoord> target;
        if (std::abs (rx) > kAimDeadzone || std::abs (ry) > kAimDeadzone)
        {
            const GridCoord home = cellOf (p.position);
            const int tdx = rx >  kAimDeadzone ?  1 : rx < -kAimDeadzone ? -1 : 0;
            const int tdy = ry >  kAimDeadzone ?  1 : ry < -kAimDeadzone ? -1 : 0;
            if (tdx != 0 || tdy != 0)
                target = GridCoord { home.col + tdx, home.row + tdy };
        }
        p.targetCell  = target;
        p.targetValid = target.has_value()
                          && p.heldTile != nullptr
                          && board.canPlace (*target, PlacedTile { p.heldTile, p.heldRotation });

        // ---- Place (A), edge-triggered. Opens a fresh claim window.
        const bool place = c->isButtonDown (B::faceDown);
        if (place && ! p.prevPlace && p.targetValid)
        {
            board.place (*target, PlacedTile { p.heldTile, p.heldRotation });
            p.heldTile     = deck.draw();
            p.heldRotation = 0;
            p.targetCell.reset();
            p.targetValid  = false;
            p.lastPlaced   = *target;
            p.hoveredClaimable.reset();
        }
        p.prevPlace = place;

        // ---- Hover detection: only the placer, only on the placed tile, only if
        //      the underlying feature isn't already claimed by anyone.
        p.hoveredClaimable.reset();
        if (p.lastPlaced.has_value())
        {
            const GridCoord nowCell = cellOf (p.position);
            if (nowCell == *p.lastPlaced)
            {
                if (const auto* nowTile = board.at (nowCell))
                {
                    const juce::Point<float> nowLocal { p.position.x - (float) nowCell.col,
                                                        p.position.y - (float) nowCell.row };
                    if (const auto sample = sampleTileFeature (*nowTile, nowLocal))
                    {
                        const FeatureRef ref { nowCell, sample->featureIndex };
                        if (! isClaimedByAnyone (ref))
                            p.hoveredClaimable = ref;
                    }
                }
            }
        }

        // ---- Claim (B), edge-triggered.
        const bool claim = c->isButtonDown (B::faceRight);
        if (claim && ! p.prevClaim && p.hoveredClaimable.has_value())
        {
            p.claims.insert (*p.hoveredClaimable);
            p.lastPlaced.reset();        // claim window closes
            p.hoveredClaimable.reset();
        }
        p.prevClaim = claim;
    }
}

int GameState::computeScore (int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= (int) players.size())
        return 0;

    const auto& player = players[(size_t) playerIndex];

    int total = 0;
    std::set<FeatureRef> alreadyTraced;
    for (const auto& ref : player.claims)
    {
        if (alreadyTraced.count (ref))
            continue;

        const auto inst = traceFeature (board, ref);
        for (const auto& s : inst.segments)
            alreadyTraced.insert (s);

        total += scoreOf (inst, board);
    }
    return total;
}

} // namespace game
