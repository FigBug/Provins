#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "game/GameState.h"
#include "view/GameView.h"

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    std::unique_ptr<game::GameState> state;
    std::unique_ptr<view::GameView>  gameView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
