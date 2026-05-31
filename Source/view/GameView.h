#pragma once

#include "../game/GameState.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace view
{

/** Renders a GameState. Phase 1: just draws placed tiles, auto-fitting the
    camera around the placed-tile bounding box (plus one cell of padding). */
class GameView : public juce::Component
{
public:
    explicit GameView (const game::GameState& state);

    void paint (juce::Graphics&) override;

private:
    const game::GameState& state;

    /** Cells of padding around the placed-tile bounding box. */
    static constexpr int kPaddingCells = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GameView)
};

} // namespace view
