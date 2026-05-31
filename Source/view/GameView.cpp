#include "GameView.h"
#include "TileRenderer.h"

namespace view
{

GameView::GameView (const game::GameState& s) : state (s) {}

void GameView::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (24, 26, 30));

    const auto bounds = state.getBoard().bounds();
    if (! bounds.has_value())
        return;

    // Worldspace = tile coordinates with one tile per unit. Inflate the
    // bounding box by the padding margin so unplaced cells surround the play
    // area visually.
    const auto worldBounds = bounds->expanded (kPaddingCells).toFloat();

    // Map worldspace into the component while preserving aspect ratio.
    const auto compArea = getLocalBounds().toFloat();
    const float scale = std::min (compArea.getWidth()  / worldBounds.getWidth(),
                                  compArea.getHeight() / worldBounds.getHeight());

    const float drawnW = worldBounds.getWidth()  * scale;
    const float drawnH = worldBounds.getHeight() * scale;
    const float offsetX = compArea.getX() + (compArea.getWidth()  - drawnW) * 0.5f;
    const float offsetY = compArea.getY() + (compArea.getHeight() - drawnH) * 0.5f;

    juce::Graphics::ScopedSaveState save (g);
    g.addTransform (juce::AffineTransform::translation (offsetX - worldBounds.getX() * scale,
                                                        offsetY - worldBounds.getY() * scale)
                                          .scaled (scale, scale,
                                                   worldBounds.getX() * scale + offsetX,
                                                   worldBounds.getY() * scale + offsetY));

    // Grid: faint lines over the inflated bounding region.
    g.setColour (juce::Colour::fromRGB (40, 44, 50));
    for (int c = (int) worldBounds.getX(); c <= (int) worldBounds.getRight(); ++c)
        g.drawLine ((float) c, worldBounds.getY(), (float) c, worldBounds.getBottom(), 0.02f);
    for (int r = (int) worldBounds.getY(); r <= (int) worldBounds.getBottom(); ++r)
        g.drawLine (worldBounds.getX(), (float) r, worldBounds.getRight(), (float) r, 0.02f);

    // Tiles. Translate-per-tile so the renderer can draw into the unit square.
    for (const auto& [coord, tile] : state.getBoard().placed())
    {
        juce::Graphics::ScopedSaveState tileSave (g);
        g.addTransform (juce::AffineTransform::translation ((float) coord.col, (float) coord.row));
        TileRenderer::draw (g, tile);
    }
}

} // namespace view
