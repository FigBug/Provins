# Provins

A real-time multiplayer adaptation of the classic tile-laying board game Carcassonne, built with JUCE. Players race to place tiles, claim features, and score points — all happening simultaneously rather than turn-by-turn.

## How to Play

### Overview

Players share a single growing map. Each player holds one tile at a time and must physically walk their meeple to a valid placement location, aim at an empty cell, and place it. After placing a tile, there is a brief window to claim one of the features on that tile by standing on it and pressing the claim button. Once all 71 tiles have been placed, a 10-second countdown begins for final claims, then the game ends and scores are tallied.

### Tiles and Placement

Each tile has edges that are either **field** (green), **road** (tan), or **city** (brown). A tile can only be placed in an empty cell if every shared edge matches its neighbor (city-to-city, road-to-road, field-to-field). The ghost tile preview shows green when valid and red when invalid.

### Features and Scoring

- **Roads** — 1 point per tile in the road. Meeple is returned when the road is completed (both ends terminate at a city, cloister, or intersection).
- **Cities** — 2 points per tile + 2 per pennant (gold dot) when complete. Incomplete cities score at half rate (1 point per tile/pennant). Meeple is returned on completion.
- **Cloisters** — 1 point for the cloister tile plus 1 point for each of the 8 surrounding cells that are occupied (max 9). Meeple is returned when all 8 surrounding cells are filled.
- **Farms** — 3 points per completed city that borders the farm. Farms are scored at the end of the game only. Meeples placed on farms are never returned.

### Meeples

Each player starts with 7 meeples. Placing a claim uses one meeple. When a feature is completed, the meeple is returned to the player's supply and can be reused. Farm meeples are committed for the entire game. You can only claim a feature that no other player has already claimed.

### Intersections

Tiles with 3 or more road edges have an intersection (shown as a circle at the center). Each road arm at an intersection is a separate feature — they do not connect through the tile.

### Disconnected Cities

Some tiles have two separate city features on different edges that are not connected through the tile. These are drawn as isolated trapezoids without filled-in interior, making them visually distinct from connected multi-edge cities.

## Title Screen

- **Left/Right arrows** or **LB/RB** — Change number of players (2–4)
- Connected controllers show as **P1**, **P2**, etc. Unconnected slots show as **AI**
- **Any button** or **any key** — Start the game

## Controls (Gamepad)

| Action | Button |
|--------|--------|
| Move | Left stick / D-pad |
| Aim tile placement | Right stick (8 directions) |
| Rotate tile CW | RB (right shoulder) |
| Rotate tile CCW | LB (left shoulder) |
| Place tile | A / Left trigger / Right trigger |
| Claim feature | B |

After placing a tile, the aim is suppressed until the right stick returns to center, preventing accidental preview of the next tile.

## Controls (Keyboard)

Keyboard input is used for menu navigation (left/right arrows to change player count, any key to start/return to menu). In-game controls require a gamepad.

## HUD

- **Top-left panel** — Player scoreboard showing color, name, meeple count (e.g. 5/7), score, and held tile preview
- **Top-right** — Tiles remaining, claim countdown timer, or "Game Over"

## End Screen

The end screen shows a detailed score breakdown per player:

| Column | Description |
|--------|-------------|
| Roads | Points from road claims |
| Cities | Points from completed cities |
| Incomp | Points from incomplete cities |
| Cloist | Points from cloisters |
| Farms | Points from farm scoring |
| Tiles | Number of tiles placed |
| Total | Sum of all points |

Press any button to return to the title screen.

## AI Players

AI players automatically place tiles, move to claim features, and try to complete their claimed cities and roads. They prioritize: cities > cloisters > roads > farms. AI players have brief pauses before placing and claiming to keep the game readable.

## Building

Requires CMake 3.24+ and a C++20 compiler.

```
cmake --preset xcode       # macOS
cmake --build --preset xcode --config Release
```

The game has no external assets — all rendering is procedural and the tile definitions are compiled into the binary from `Assets/tiles.json`.

## License

Copyright Roland Rabien. All rights reserved.
