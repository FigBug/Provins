#pragma once

#include "../game/GameState.h"
#include <JuceHeader.h>

namespace view
{

/** Renders a GameState. Phase 1: just draws placed tiles, auto-fitting the
    camera around the placed-tile bounding box (plus one cell of padding). */
class GameView : public juce::Component
{
public:
    explicit GameView (game::GameState& state);

    void paint (juce::Graphics&) override;
    void update (float dt);

private:
    game::GameState& state;

    static constexpr int kPaddingCells = 1;
    static constexpr float kCameraSmoothing = 3.0f;
    static constexpr float kScoreAnimDuration = 1.5f;

    juce::Rectangle<float> currentWorldBounds { -2.0f, -2.0f, 5.0f, 5.0f };
    bool                   cameraInitialised = false;

    struct ScoreAnim
    {
        juce::Point<float> worldPos;
        juce::Colour       colour;
        int                points;
        float              age = 0.0f;
    };
    std::vector<ScoreAnim> scoreAnims;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GameView)
};

} // namespace view
