#include "GameState.h"

#include "Features.h"
#include "Terrain.h"

#include <gin_controllers/gin_controllers.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>

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

    // ---- AI helpers ----

    /** 4-connected BFS over placed cells from `start`. */
    std::set<GridCoord> reachablePlacedCells (const Board& board, GridCoord start)
    {
        std::set<GridCoord> visited;
        if (! board.isOccupied (start))
            return visited;

        std::queue<GridCoord> q;
        q.push (start);
        visited.insert (start);

        while (! q.empty())
        {
            const auto cur = q.front();
            q.pop();
            for (auto d : allDirections)
            {
                const auto nb = cur.neighbour (d);
                if (board.isOccupied (nb) && visited.insert (nb).second)
                    q.push (nb);
            }
        }
        return visited;
    }

    /** Shortest 4-connected path through placed cells. Returns the sequence
        of cells from `start` (exclusive) to `goal` (inclusive). */
    std::vector<GridCoord> findPath (const Board& board, GridCoord start, GridCoord goal)
    {
        if (! board.isOccupied (start) || ! board.isOccupied (goal))
            return {};
        if (start == goal)
            return {};

        std::queue<GridCoord> q;
        std::map<GridCoord, GridCoord> cameFrom;
        q.push (start);
        cameFrom[start] = start;

        while (! q.empty())
        {
            const auto cur = q.front();
            q.pop();
            if (cur == goal)
            {
                std::vector<GridCoord> path;
                auto walk = cur;
                while (walk != start)
                {
                    path.push_back (walk);
                    walk = cameFrom[walk];
                }
                std::reverse (path.begin(), path.end());
                return path;
            }
            for (auto d : allDirections)
            {
                const auto nb = cur.neighbour (d);
                if (board.isOccupied (nb) && cameFrom.find (nb) == cameFrom.end())
                {
                    cameFrom[nb] = cur;
                    q.push (nb);
                }
            }
        }
        return {};
    }

    /** A simple "feature priority" for picking what to claim. */
    int claimPriority (FeatureType t) noexcept
    {
        switch (t)
        {
            case FeatureType::city:     return 4;
            case FeatureType::cloister: return 3;
            case FeatureType::road:     return 2;
            case FeatureType::field:    return 1;
        }
        return 0;
    }

}

GameState::GameState (const juce::String& tilesJson,
                      int maxPlayers_,
                      int tileMultiplier,
                      juce::uint32 randomSeed)
    : deck (tilesJson, tileMultiplier, randomSeed), maxPlayers (maxPlayers_)
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
    p.position        = kPortSpawnOffset[controllerIndex];
    p.heldTile        = deck.draw();
    p.heldRotation    = 0;
    players.push_back (p);
}

int GameState::findFreeSlot() const noexcept
{
    std::array<bool, kMaxPlayers> used {};
    for (const auto& existing : players)
        for (int i = 0; i < kMaxPlayers; ++i)
            if (existing.colour == kPortColours[i])
                used[(size_t) i] = true;

    for (int i = 0; i < kMaxPlayers; ++i)
        if (! used[(size_t) i])
            return i;
    return -1;
}

void GameState::spawnAi()
{
    spawnAi (findFreeSlot());
}

void GameState::spawnAi (int slot)
{
    if (slot < 0 || slot >= kMaxPlayers)
        return;

    Player p;
    p.controllerIndex = -1;
    p.colour          = kPortColours[slot];
    p.position        = kPortSpawnOffset[slot];
    p.heldTile        = deck.draw();
    p.heldRotation    = 0;
    p.ai              = AiBrain {};
    players.push_back (std::move (p));
}

bool GameState::allTilesPlaced() const noexcept
{
    if (! deck.empty())
        return false;
    for (const auto& p : players)
        if (p.heldTile != nullptr)
            return false;
    return ! players.empty();
}

bool GameState::isGameOver() const noexcept
{
    return endTimer == 0.0f;
}

/** Returns true if any segment of the instance containing `ref` is already
    in any player's claims set. */
bool GameState::isClaimedByAnyone (const FeatureRef& ref) const
{
    const auto inst = traceFeature (board, ref);
    std::set<FeatureRef> instSegs (inst.segments.begin(), inst.segments.end());

    for (const auto& player : players)
        for (const auto& [claim, info] : player.claims)
            if (instSegs.count (claim) != 0)
                return true;

    return false;
}

void GameState::returnCompletedMeeples()
{
    for (auto& p : players)
    {
        for (auto& [ref, info] : p.claims)
        {
            if (info.meepleReturned)
                continue;

            const auto inst = traceFeature (board, ref);
            if (inst.isComplete)
            {
                info.meepleReturned = true;
                ++p.meepleSupply;
            }
        }
    }
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

        if (! alreadyHasPlayer && (int) players.size() < maxPlayers)
            spawnPlayer (port);
    }

    for (auto& p : players)
    {
        if (p.ai.has_value())
        {
            aiUpdate (p, *p.ai, dt);
            continue;
        }

        if (p.controllerIndex < 0)
            continue;

        auto* c = controllers.getController (p.controllerIndex);
        if (c == nullptr || ! c->isConnected())
            continue;

        // ---- Movement: left stick + d-pad, speed scales by terrain.
        float lx = c->getAxis (A::leftX);
        float ly = c->getAxis (A::leftY);

        if (c->isButtonDown (B::dpadLeft))  lx = -1.0f;
        if (c->isButtonDown (B::dpadRight)) lx =  1.0f;
        if (c->isButtonDown (B::dpadUp))    ly = -1.0f;
        if (c->isButtonDown (B::dpadDown))  ly =  1.0f;

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
        const bool stickAtCentre = std::abs (rx) <= kAimDeadzone
                                && std::abs (ry) <= kAimDeadzone;

        if (p.aimSuppressed && stickAtCentre)
            p.aimSuppressed = false;

        std::optional<GridCoord> target;
        if (! p.aimSuppressed && ! stickAtCentre)
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

        // ---- Place (A or either trigger), edge-triggered. Opens a fresh claim window.
        const bool place = c->isButtonDown (B::faceDown)
                        || c->getAxis (A::leftTrigger)  > 0.5f
                        || c->getAxis (A::rightTrigger) > 0.5f;
        if (place && ! p.prevPlace && p.targetValid)
        {
            board.place (*target, PlacedTile { p.heldTile, p.heldRotation });
            ++p.tilesPlaced;
            p.heldTile     = deck.draw();
            p.heldRotation = 0;
            p.targetCell.reset();
            p.targetValid  = false;
            p.lastPlaced   = *target;
            p.hoveredClaimable.reset();
            p.aimSuppressed = true;
        }
        p.prevPlace = place;

        // ---- Hover detection: only the placer, only on the placed tile, only if
        //      the underlying feature isn't already claimed by anyone and
        //      the player has meeples available.
        p.hoveredClaimable.reset();
        if (p.lastPlaced.has_value() && p.meeplesAvailable() > 0)
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
        if (claim && ! p.prevClaim && p.hoveredClaimable.has_value()
            && p.meeplesAvailable() > 0)
        {
            p.claims[*p.hoveredClaimable] = { p.position, false };
            --p.meepleSupply;
            p.lastPlaced.reset();
            p.hoveredClaimable.reset();
        }
        p.prevClaim = claim;
    }

    returnCompletedMeeples();

    if (allTilesPlaced() && endTimer < 0.0f)
        endTimer = 10.0f;

    if (endTimer > 0.0f)
    {
        endTimer -= dt;
        if (endTimer <= 0.0f)
            endTimer = 0.0f;
    }
}

int GameState::computeScore (int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= (int) players.size())
        return 0;

    const auto& player = players[(size_t) playerIndex];

    int total = 0;
    std::set<FeatureRef> alreadyTraced;
    for (const auto& [ref, info] : player.claims)
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

GameState::ScoreBreakdown GameState::computeScoreBreakdown (int playerIndex) const
{
    ScoreBreakdown bd;
    if (playerIndex < 0 || playerIndex >= (int) players.size())
        return bd;

    const auto& player = players[(size_t) playerIndex];
    bd.tilesPlaced = player.tilesPlaced;

    std::set<FeatureRef> alreadyTraced;
    for (const auto& [ref, info] : player.claims)
    {
        if (alreadyTraced.count (ref))
            continue;

        const auto inst = traceFeature (board, ref);
        for (const auto& s : inst.segments)
            alreadyTraced.insert (s);

        int pts = scoreOf (inst, board);
        switch (inst.type)
        {
            case FeatureType::road:     bd.roads            += pts; break;
            case FeatureType::city:     (inst.isComplete ? bd.citiesComplete : bd.citiesIncomplete) += pts; break;
            case FeatureType::cloister: bd.cloisters        += pts; break;
            case FeatureType::field:    bd.farms            += pts; break;
        }
    }
    return bd;
}

// ============================================================================
// AI player
// ============================================================================

void GameState::aiUpdate (Player& p, AiBrain& brain, float dt)
{
    brain.planCooldown -= dt;

    const bool noActivePlan = ! brain.placeCell.has_value()
                              && ! brain.fineTarget.has_value();

    if (noActivePlan)
        brain.stuckTimer = 0.0f;
    else
    {
        brain.stuckTimer += dt;
        if (brain.stuckTimer > 7.0f)
        {
            brain.placeCell.reset();
            brain.placeFromCell.reset();
            brain.fineTarget.reset();
            brain.path.clear();
            brain.pathIdx      = 0;
            brain.placeDelay   = 0.0f;
            brain.claimDelay   = 0.0f;
            brain.stuckTimer   = 0.0f;
            brain.planCooldown = 0.0f;
            p.lastPlaced.reset();
            p.hoveredClaimable.reset();
            return;
        }
    }

    if (noActivePlan && brain.planCooldown <= 0.0f)
    {
        bool planned = false;

        // Priority: settle any open claim window from the last placement.
        if (p.lastPlaced.has_value())
        {
            planned = aiPlanClaim (p, brain);
            if (! planned)
                p.lastPlaced.reset();    // no claimable feature here — forfeit
        }

        // Otherwise look for a place to drop the held tile.
        if (! planned && p.heldTile != nullptr)
        {
            planned = aiPlanPlacement (p, brain);
            if (! planned)
            {
                // Stuck — discard and redraw so we don't softlock the game.
                p.heldTile     = deck.draw();
                p.heldRotation = 0;
            }
        }

        brain.planCooldown = 0.15f;
    }

    aiMoveAlongPath (p, brain, dt);

    // ---- Commit placement when we've arrived at placeFromCell.
    if (brain.placeCell.has_value() && brain.placeFromCell.has_value())
    {
        if (cellOf (p.position) == *brain.placeFromCell)
        {
            if (brain.placeDelay <= 0.0f)
                brain.placeDelay = 3.3f;

            brain.placeDelay -= dt;
            if (brain.placeDelay > 0.0f)
                return;

            const PlacedTile pt { p.heldTile, brain.placeRotation };
            if (p.heldTile != nullptr
                && ! board.isOccupied (*brain.placeCell)
                && board.canPlace (*brain.placeCell, pt))
            {
                board.place (*brain.placeCell, pt);
                ++p.tilesPlaced;
                p.lastPlaced   = *brain.placeCell;
                p.heldTile     = deck.draw();
                p.heldRotation = 0;
            }
            brain.placeCell.reset();
            brain.placeFromCell.reset();
            brain.path.clear();
            brain.pathIdx      = 0;
            brain.placeDelay   = 0.0f;
            brain.planCooldown = 0.0f;
        }
    }

    // ---- Commit claim when we're standing on the target feature.
    if (brain.fineTarget.has_value() && p.lastPlaced.has_value())
    {
        const GridCoord cell = cellOf (p.position);
        if (cell == *p.lastPlaced)
        {
            const auto* tile = board.at (cell);
            if (tile != nullptr)
            {
                const juce::Point<float> local { p.position.x - (float) cell.col,
                                                 p.position.y - (float) cell.row };
                if (const auto sample = sampleTileFeature (*tile, local))
                {
                    const FeatureRef ref { cell, sample->featureIndex };

                    if (! isClaimedByAnyone (ref))
                        p.hoveredClaimable = ref;
                    else
                        p.hoveredClaimable.reset();

                    const auto diff = p.position - *brain.fineTarget;
                    if (diff.getDistanceFromOrigin() < 0.10f)
                    {
                        if (brain.claimDelay <= 0.0f)
                            brain.claimDelay = 1.7f;

                        brain.claimDelay -= dt;
                        if (brain.claimDelay > 0.0f)
                            return;

                        if (! isClaimedByAnyone (ref) && p.meeplesAvailable() > 0)
                        {
                            p.claims[ref] = { p.position, false };
                            --p.meepleSupply;
                        }

                        p.lastPlaced.reset();
                        p.hoveredClaimable.reset();
                        brain.fineTarget.reset();
                        brain.path.clear();
                        brain.pathIdx      = 0;
                        brain.claimDelay   = 0.0f;
                        brain.planCooldown = 0.0f;
                    }
                }
            }
        }
    }
}

bool GameState::aiPlanPlacement (Player& p, AiBrain& brain)
{
    if (p.heldTile == nullptr)
        return false;

    const GridCoord start = cellOf (p.position);
    if (! board.isOccupied (start))
        return false;

    const auto reachable = reachablePlacedCells (board, start);

    constexpr int kOffsets[8][2] = {
        {-1,-1}, { 0,-1}, { 1,-1},
        {-1, 0},          { 1, 0},
        {-1, 1}, { 0, 1}, { 1, 1}
    };

    // Collect all feature instances this player has claimed.
    std::set<FeatureRef> ownedSegments;
    for (const auto& [claim, claimInfo] : p.claims)
    {
        const auto inst = traceFeature (board, claim);
        for (const auto& seg : inst.segments)
            ownedSegments.insert (seg);
    }

    int       bestScore    = std::numeric_limits<int>::min();
    GridCoord bestPlaceCell { 0, 0 };
    GridCoord bestFromCell  { 0, 0 };
    int       bestRotation = 0;
    bool      found = false;

    for (const auto& placedCell : reachable)
    {
        for (const auto& off : kOffsets)
        {
            const GridCoord candidate { placedCell.col + off[0], placedCell.row + off[1] };
            if (board.isOccupied (candidate))
                continue;

            for (int rot = 0; rot < 4; ++rot)
            {
                const PlacedTile pt { p.heldTile, rot };
                if (! board.canPlace (candidate, pt))
                    continue;

                int score = 0;

                // Temporarily place the tile to evaluate.
                board.place (candidate, pt);

                // Check if this placement extends or completes any owned feature.
                for (auto d : allDirections)
                {
                    const auto nbr = candidate.neighbour (d);
                    if (ownedSegments.count ({ nbr, 0 }) == 0)
                    {
                        bool isOwned = false;
                        const auto* nbrTile = board.at (nbr);
                        if (nbrTile != nullptr)
                        {
                            for (int fi = 0; fi < (int) nbrTile->type->features.size(); ++fi)
                            {
                                if (ownedSegments.count ({ nbr, fi }) != 0)
                                {
                                    isOwned = true;
                                    break;
                                }
                            }
                        }
                        if (! isOwned) continue;
                    }

                    // This neighbor has one of our claims. Check if
                    // the connected feature gets closer to completion.
                    for (const auto& [claim, claimInfo] : p.claims)
                    {
                        const auto inst = traceFeature (board, claim);
                        bool touchesCandidate = false;
                        for (const auto& seg : inst.segments)
                            if (seg.cell == candidate)
                                { touchesCandidate = true; break; }

                        if (touchesCandidate)
                        {
                            score += 10;
                            if (inst.isComplete)
                                score += 50;
                        }
                    }
                    break;
                }

                board.remove (candidate);

                const int dist = std::abs (placedCell.col - start.col)
                               + std::abs (placedCell.row - start.row);
                score -= dist;

                if (score > bestScore)
                {
                    bestScore     = score;
                    bestPlaceCell = candidate;
                    bestFromCell  = placedCell;
                    bestRotation  = rot;
                    found = true;
                }
            }
        }
    }

    if (! found)
        return false;

    brain.placeCell     = bestPlaceCell;
    brain.placeFromCell = bestFromCell;
    brain.placeRotation = bestRotation;
    brain.path          = findPath (board, start, bestFromCell);
    brain.pathIdx       = 0;
    brain.fineTarget.reset();
    return true;
}

bool GameState::aiPlanClaim (Player& p, AiBrain& brain)
{
    if (! p.lastPlaced.has_value() || p.meeplesAvailable() <= 0)
        return false;

    const auto       cell = *p.lastPlaced;
    const auto*      tile = board.at (cell);
    if (tile == nullptr)
        return false;

    int bestPriority = -1;
    int bestFi       = -1;
    for (size_t fi = 0; fi < tile->type->features.size(); ++fi)
    {
        const auto&      f   = tile->type->features[fi];
        const FeatureRef ref { cell, (int) fi };
        if (isClaimedByAnyone (ref))
            continue;

        const int pr = claimPriority (f.type);
        if (pr > bestPriority)
        {
            bestPriority = pr;
            bestFi       = (int) fi;
        }
    }

    if (bestFi < 0)
        return false;

    const auto& feat        = tile->type->features[(size_t) bestFi];
    const auto  canonicalPt = featureSamplePoint (feat);
    const auto  rotated     = rotateLocalCW (canonicalPt, tile->rotation);

    brain.fineTarget = juce::Point<float> { (float) cell.col + rotated.x,
                                            (float) cell.row + rotated.y };

    brain.path    = findPath (board, cellOf (p.position), cell);
    brain.pathIdx = 0;
    brain.placeCell.reset();
    brain.placeFromCell.reset();
    return true;
}

void GameState::aiMoveAlongPath (Player& p, AiBrain& brain, float dt)
{
    juce::Point<float> target;
    bool               hasTarget = false;

    if (brain.pathIdx < brain.path.size())
    {
        const auto cell = brain.path[brain.pathIdx];
        target    = { (float) cell.col + 0.5f, (float) cell.row + 0.5f };
        hasTarget = true;
    }
    else if (brain.fineTarget.has_value())
    {
        target    = *brain.fineTarget;
        hasTarget = true;
    }

    if (! hasTarget)
        return;

    const auto  diff = target - p.position;
    const float dist = diff.getDistanceFromOrigin();

    if (dist < 0.05f)
    {
        if (brain.pathIdx < brain.path.size())
            ++brain.pathIdx;
        return;
    }

    const juce::Point<float> dir { diff.x / dist, diff.y / dist };

    // Match human movement physics (terrain-based speed + axis-separated collision).
    const GridCoord cur     = cellOf (p.position);
    const auto*     curTile = board.at (cur);
    float           speed   = kSpeedField;
    if (curTile != nullptr)
    {
        const juce::Point<float> local { p.position.x - (float) cur.col,
                                         p.position.y - (float) cur.row };
        speed = terrainSpeed (terrainAt (*curTile, local));
    }

    const float dx = dir.x * speed * dt;
    const float dy = dir.y * speed * dt;

    auto tryAxis = [&] (float ax, float ay)
    {
        const juce::Point<float> candidate { p.position.x + ax, p.position.y + ay };
        if (board.isOccupied (cellOf (candidate)))
            p.position = candidate;
    };
    tryAxis (dx,  0.0f);
    tryAxis (0.0f, dy);
}

} // namespace game
