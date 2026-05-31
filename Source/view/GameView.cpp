#include "GameView.h"
#include "TileRenderer.h"
#include "MeepleRenderer.h"

namespace view
{

namespace
{
    const char* featureLabel (game::FeatureType t)
    {
        switch (t)
        {
            case game::FeatureType::road:     return "Road";
            case game::FeatureType::city:     return "City";
            case game::FeatureType::cloister: return "Cloister";
            case game::FeatureType::field:    return "Field";
        }
        return "?";
    }
}

GameView::GameView (const game::GameState& s) : state (s) {}

void GameView::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (24, 26, 30));

    const auto placedBounds = state.getBoard().bounds();
    if (! placedBounds.has_value())
        return;

    // Camera bbox = placed tiles ∪ every player position, padded.
    juce::Rectangle<float> worldBounds = placedBounds->toFloat();
    for (const auto& p : state.getPlayers())
    {
        worldBounds = worldBounds.getUnion (
            juce::Rectangle<float> (p.position.x, p.position.y, 0.0f, 0.0f));
    }
    worldBounds.expand (kPaddingCells, kPaddingCells);

    const auto compArea = getLocalBounds().toFloat();
    const float scale = std::min (compArea.getWidth()  / worldBounds.getWidth(),
                                  compArea.getHeight() / worldBounds.getHeight());

    const float drawnW  = worldBounds.getWidth()  * scale;
    const float drawnH  = worldBounds.getHeight() * scale;
    const float offsetX = compArea.getX() + (compArea.getWidth()  - drawnW) * 0.5f;
    const float offsetY = compArea.getY() + (compArea.getHeight() - drawnH) * 0.5f;

    const auto worldToScreen = juce::AffineTransform::translation (offsetX - worldBounds.getX() * scale,
                                                                   offsetY - worldBounds.getY() * scale)
                                                     .scaled (scale, scale,
                                                              worldBounds.getX() * scale + offsetX,
                                                              worldBounds.getY() * scale + offsetY);

    // ---- All world-coord drawing inside this transform scope.
    {
        juce::Graphics::ScopedSaveState save (g);
        g.addTransform (worldToScreen);

        // Faint grid.
        g.setColour (juce::Colour::fromRGB (40, 44, 50));
        for (int c = (int) worldBounds.getX(); c <= (int) worldBounds.getRight(); ++c)
            g.drawLine ((float) c, worldBounds.getY(), (float) c, worldBounds.getBottom(), 0.02f);
        for (int r = (int) worldBounds.getY(); r <= (int) worldBounds.getBottom(); ++r)
            g.drawLine (worldBounds.getX(), (float) r, worldBounds.getRight(), (float) r, 0.02f);

        // Placed tiles.
        for (const auto& [coord, tile] : state.getBoard().placed())
        {
            juce::Graphics::ScopedSaveState tileSave (g);
            g.addTransform (juce::AffineTransform::translation ((float) coord.col, (float) coord.row));
            TileRenderer::draw (g, tile);
        }

        // Ghost tiles for any player aiming at a target cell.
        for (const auto& p : state.getPlayers())
        {
            if (! p.targetCell.has_value() || p.heldTile == nullptr)
                continue;

            juce::Graphics::ScopedSaveState ghostSave (g);
            g.addTransform (juce::AffineTransform::translation ((float) p.targetCell->col,
                                                                (float) p.targetCell->row));
            g.setOpacity (0.55f);
            TileRenderer::draw (g, game::PlacedTile { p.heldTile, p.heldRotation });

            g.setOpacity (1.0f);
            g.setColour (p.targetValid ? juce::Colours::limegreen : juce::Colours::red);
            g.drawRect (juce::Rectangle<float> (0.0f, 0.0f, 1.0f, 1.0f), 0.06f);
        }

        // Persistent claim flags — one dot per claimed segment, at the tile centre.
        for (const auto& p : state.getPlayers())
        {
            for (const auto& ref : p.claims)
            {
                const float cx = (float) ref.cell.col + 0.5f;
                const float cy = (float) ref.cell.row + 0.5f;
                const float r  = 0.10f;
                g.setColour (p.colour);
                g.fillEllipse (cx - r, cy - r, 2 * r, 2 * r);
                g.setColour (juce::Colours::black);
                g.drawEllipse (cx - r, cy - r, 2 * r, 2 * r, 0.015f);
            }
        }

        // Hover glow around the meeple when the placer is over a claimable feature.
        for (const auto& p : state.getPlayers())
        {
            if (! p.hoveredClaimable.has_value())
                continue;

            const float r1 = 0.30f;
            const float r2 = 0.42f;
            g.setColour (p.colour.withAlpha (0.35f));
            g.fillEllipse (p.position.x - r2, p.position.y - r2, 2 * r2, 2 * r2);
            g.setColour (p.colour);
            g.drawEllipse (p.position.x - r1, p.position.y - r1, 2 * r1, 2 * r1, 0.03f);
        }

        // Players.
        for (const auto& p : state.getPlayers())
            MeepleRenderer::draw (g, p.position, p.colour);

        // Held-tile badges.
        for (const auto& p : state.getPlayers())
        {
            if (p.heldTile == nullptr)
                continue;

            juce::Graphics::ScopedSaveState badgeSave (g);
            constexpr float kBadge   = 0.45f;
            constexpr float kOffsetY = -0.55f;
            g.addTransform (juce::AffineTransform::scale (kBadge)
                                                  .translated (p.position.x - kBadge * 0.5f,
                                                               p.position.y + kOffsetY));
            TileRenderer::draw (g, game::PlacedTile { p.heldTile, p.heldRotation });

            g.setColour (p.colour);
            g.drawRect (juce::Rectangle<float> (0.0f, 0.0f, 1.0f, 1.0f), 0.06f);
        }
    }

    // ---- Screen-coord overlays (text hints).
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    for (const auto& p : state.getPlayers())
    {
        if (! p.hoveredClaimable.has_value())
            continue;

        const auto* tile = state.getBoard().at (p.hoveredClaimable->cell);
        if (tile == nullptr)
            continue;

        const auto& feat = tile->type->features[(size_t) p.hoveredClaimable->featureIndex];

        juce::Point<float> meepleScreen (p.position.x, p.position.y);
        meepleScreen.applyTransform (worldToScreen);

        const juce::String label = juce::String ("B: Claim ") + featureLabel (feat.type);
        const int textW = 160;
        const int textH = 22;
        const juce::Rectangle<int> r ((int) meepleScreen.x - textW / 2,
                                      (int) meepleScreen.y - 60,
                                      textW, textH);
        g.setColour (juce::Colour::fromRGBA (0, 0, 0, 180));
        g.fillRoundedRectangle (r.toFloat(), 4.0f);
        g.setColour (p.colour);
        g.drawText (label, r, juce::Justification::centred, false);
    }
}

} // namespace view
