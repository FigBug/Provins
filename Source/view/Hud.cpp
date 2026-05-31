#include "Hud.h"

#include <algorithm>

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
    const float     panelW     = 240.0f;
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
            (int) (panel.getRight() - pad - sw.getRight() - 10.0f),
            (int) rowH };

        const juce::String label = "P" + juce::String (i + 1);
        const int score = state.computeScore ((int) i);

        g.drawText (label, textArea, juce::Justification::centredLeft, false);
        g.drawText (juce::String (score), textArea, juce::Justification::centredRight, false);

        y += rowH;
    }

    // ---- Deck remaining (top-right).
    const juce::String deckText = state.isGameOver()
                                    ? juce::String ("Final tile placed")
                                    : ("Tiles left: " + juce::String (state.getDeck().size()));
    g.setFont (juce::FontOptions (16.0f));
    const juce::Rectangle<int> deckArea {
        (int) (area.getRight() - 220.0f - pad),
        (int) (area.getY() + pad),
        220,
        (int) rowH };

    g.setColour (juce::Colour::fromRGBA (0, 0, 0, 160));
    g.fillRoundedRectangle (deckArea.toFloat(), 6.0f);
    g.setColour (juce::Colours::white);
    g.drawText (deckText, deckArea.reduced (10, 0), juce::Justification::centredRight, false);

    // ---- Game-over centred panel.
    if (! state.isGameOver())
        return;

    const float panW = 520.0f;
    const float panH = 200.0f;
    juce::Rectangle<float> over { area.getCentreX() - panW * 0.5f,
                                  area.getCentreY() - panH * 0.5f,
                                  panW, panH };

    g.setColour (juce::Colour::fromRGBA (0, 0, 0, 220));
    g.fillRoundedRectangle (over, 12.0f);
    g.setColour (juce::Colours::white);
    g.drawRoundedRectangle (over, 12.0f, 2.0f);

    g.setFont (juce::FontOptions (36.0f, juce::Font::bold));
    g.drawText ("Game Over", over.removeFromTop (70.0f),
                juce::Justification::centred, false);

    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    if (winnerIdx >= 0)
    {
        g.setColour (state.getPlayers()[(size_t) winnerIdx].colour);
        const juce::String msg = "P" + juce::String (winnerIdx + 1)
                                 + " wins with " + juce::String (topScore);
        g.drawText (msg, over.removeFromTop (60.0f), juce::Justification::centred, false);
    }
    else
    {
        g.setColour (juce::Colours::white);
        g.drawText ("Tied at " + juce::String (topScore),
                    over.removeFromTop (60.0f), juce::Justification::centred, false);
    }
}

} // namespace view
