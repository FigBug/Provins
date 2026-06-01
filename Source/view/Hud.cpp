#include "Hud.h"
#include "TileRenderer.h"

#include <algorithm>
#include <cmath>

namespace view
{

namespace
{
    // Computes (winner index, top score). Sets winner to -1 if the leaders tie.
    std::pair<int, int> findWinner (const game::GameState& state)
    {
        int topScore   = std::numeric_limits<int>::min();
        int winnerIdx  = -1;
        int leaderCount = 0;

        for (size_t i = 0; i < state.getPlayers().size(); ++i)
        {
            const int s = state.computeScore ((int) i);
            if (s > topScore)
            {
                topScore    = s;
                winnerIdx   = (int) i;
                leaderCount = 1;
            }
            else if (s == topScore)
            {
                ++leaderCount;
            }
        }

        return { leaderCount == 1 ? winnerIdx : -1, topScore };
    }
}

Hud::Hud (const game::GameState& s) : state (s)
{
    setInterceptsMouseClicks (false, false);
}

void Hud::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    const float pad = 12.0f;

    const auto [winnerIdx, topScore] = findWinner (state);

    // ---- Player scoreboard (top-left).
    constexpr float rowH       = 32.0f;
    constexpr float swatch     = 18.0f;
    constexpr float tileSize   = 26.0f;
    const float     panelW     = 280.0f;
    const float     panelH     = pad + rowH * (float) state.getPlayers().size() + pad;

    juce::Rectangle<float> panel { area.getX() + pad, area.getY() + pad, panelW, panelH };
    g.setColour (juce::Colour::fromRGBA (0, 0, 0, 160));
    g.fillRoundedRectangle (panel, 6.0f);

    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));

    float y = panel.getY() + pad;
    for (size_t i = 0; i < state.getPlayers().size(); ++i)
    {
        const auto& p = state.getPlayers()[i];

        const juce::Rectangle<float> sw { panel.getX() + pad,
                                          y + (rowH - swatch) * 0.5f,
                                          swatch, swatch };
        g.setColour (p.colour);
        g.fillRoundedRectangle (sw, 3.0f);

        // Crown the leader during game-over by tinting their row.
        const bool isWinner = state.isGameOver() && (int) i == winnerIdx;
        if (isWinner)
        {
            const juce::Rectangle<float> rowTint { panel.getX() + 2.0f,
                                                   y,
                                                   panel.getWidth() - 4.0f,
                                                   rowH };
            g.setColour (p.colour.withAlpha (0.18f));
            g.fillRoundedRectangle (rowTint, 4.0f);
        }

        g.setColour (juce::Colours::white);
        const juce::Rectangle<int> textArea {
            (int) (sw.getRight() + 10.0f),
            (int) y,
            (int) (panel.getRight() - pad - tileSize - 8.0f - sw.getRight() - 10.0f),
            (int) rowH };

        const juce::String label = "P" + juce::String (i + 1);
        const int score = state.computeScore ((int) i);
        const juce::String meepleText = juce::String (p.meeplesAvailable())
                                        + "/" + juce::String (game::Player::kMaxMeeples);

        g.drawText (label, textArea, juce::Justification::centredLeft, false);
        g.setFont (juce::FontOptions (14.0f));
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawText (meepleText, textArea.withTrimmedLeft (40).withTrimmedRight (40),
                    juce::Justification::centred, false);
        g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        g.setColour (juce::Colours::white);
        g.drawText (juce::String (score), textArea, juce::Justification::centredRight, false);

        if (p.heldTile != nullptr)
        {
            juce::Graphics::ScopedSaveState tileSave (g);
            float tx = panel.getRight() - pad - tileSize;
            float ty = y + (rowH - tileSize) * 0.5f;
            g.addTransform (juce::AffineTransform::scale (tileSize).translated (tx, ty));
            TileRenderer::draw (g, game::PlacedTile { p.heldTile, p.heldRotation });
            g.setColour (p.colour);
            g.drawRect (juce::Rectangle<float> (0.0f, 0.0f, 1.0f, 1.0f), 0.06f);
        }

        y += rowH;
    }

    // ---- Deck remaining / end timer (top-right).
    juce::String deckText;
    if (state.isGameOver())
        deckText = "Game Over";
    else if (state.allTilesPlaced())
        deckText = "Claim time: " + juce::String ((int) std::ceil (state.getEndTimer())) + "s";
    else
        deckText = "Tiles left: " + juce::String (state.getDeck().size());

    g.setFont (juce::FontOptions (16.0f));
    const juce::Rectangle<int> deckArea {
        (int) (area.getRight() - 220.0f - pad),
        (int) (area.getY() + pad),
        220,
        (int) rowH };

    g.setColour (juce::Colour::fromRGBA (0, 0, 0, 160));
    g.fillRoundedRectangle (deckArea.toFloat(), 6.0f);
    g.setColour (state.allTilesPlaced() && ! state.isGameOver()
                     ? juce::Colours::yellow : juce::Colours::white);
    g.drawText (deckText, deckArea.reduced (10, 0), juce::Justification::centredRight, false);

    // ---- Game-over centred panel.
    if (! state.isGameOver())
        return;

    const int numP = (int) state.getPlayers().size();
    constexpr float headerH   = 55.0f;
    constexpr float colHeaderH = 22.0f;
    constexpr float playerRowH = 30.0f;
    constexpr float footerH   = 36.0f;
    const float panW = 580.0f;
    const float panH = headerH + colHeaderH + playerRowH * (float) numP + footerH + 16.0f;

    juce::Rectangle<float> over { area.getCentreX() - panW * 0.5f,
                                  area.getCentreY() - panH * 0.5f,
                                  panW, panH };

    g.setColour (juce::Colour::fromRGBA (0, 0, 0, 220));
    g.fillRoundedRectangle (over, 12.0f);
    g.setColour (juce::Colours::white);
    g.drawRoundedRectangle (over, 12.0f, 2.0f);

    g.setFont (juce::FontOptions (32.0f, juce::Font::bold));
    g.drawText ("Game Over", over.removeFromTop (headerH),
                juce::Justification::centred, false);

    auto content = over.reduced (20.0f, 0.0f);

    // Column headers
    {
        auto hdr = content.removeFromTop (colHeaderH);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.setColour (juce::Colours::white.withAlpha (0.5f));

        float colX = hdr.getX() + 50.0f;
        float colW = (hdr.getWidth() - 50.0f) / 7.0f;
        const char* labels[] = { "Roads", "Cities", "Incomp", "Cloist", "Farms", "Tiles", "Total" };
        for (int c = 0; c < 7; ++c)
            g.drawText (labels[c], (int)(colX + (float)c * colW), (int)hdr.getY(),
                        (int)colW, (int)colHeaderH, juce::Justification::centred, false);
    }

    // Player rows
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    for (int i = 0; i < numP; ++i)
    {
        const auto& p = state.getPlayers()[(size_t) i];
        auto bd = state.computeScoreBreakdown (i);
        const bool isWinner = i == winnerIdx;

        auto row = content.removeFromTop (playerRowH);

        if (isWinner)
        {
            g.setColour (p.colour.withAlpha (0.15f));
            g.fillRoundedRectangle (row, 4.0f);
        }

        // Color swatch + name
        auto nameArea = row.removeFromLeft (50.0f);
        g.setColour (p.colour);
        g.fillRoundedRectangle (nameArea.removeFromLeft (16.0f).reduced (0.0f, 6.0f), 3.0f);
        g.setColour (juce::Colours::white);
        g.drawText ("P" + juce::String (i + 1), nameArea, juce::Justification::centred, false);

        // Score columns
        float colW = row.getWidth() / 7.0f;
        int vals[] = { bd.roads, bd.citiesComplete, bd.citiesIncomplete,
                       bd.cloisters, bd.farms, bd.tilesPlaced, bd.total() };

        g.setFont (juce::FontOptions (14.0f));
        for (int c = 0; c < 7; ++c)
        {
            if (c == 6)
                g.setFont (juce::FontOptions (15.0f, juce::Font::bold));

            g.setColour (c == 6 ? juce::Colours::white : juce::Colours::white.withAlpha (0.8f));
            g.drawText (juce::String (vals[c]),
                        (int)(row.getX() + (float)c * colW), (int)row.getY(),
                        (int)colW, (int)playerRowH,
                        juce::Justification::centred, false);
        }
    }

    // Winner/tie line
    auto footer = content.removeFromTop (footerH);
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    if (winnerIdx >= 0)
    {
        g.setColour (state.getPlayers()[(size_t) winnerIdx].colour);
        g.drawText ("P" + juce::String (winnerIdx + 1) + " wins!",
                    footer, juce::Justification::centred, false);
    }
    else
    {
        g.setColour (juce::Colours::white);
        g.drawText ("Tied at " + juce::String (topScore),
                    footer, juce::Justification::centred, false);
    }
}

} // namespace view
