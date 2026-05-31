#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <gin_controllers/gin_controllers.h>

#include "game/GameState.h"
#include "view/GameView.h"
#include "view/Hud.h"

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    gin::GameControllerManager       controllers;
    std::unique_ptr<game::GameState> state;
    std::unique_ptr<view::GameView>  gameView;
    std::unique_ptr<view::Hud>       hud;
    double                           lastTickMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
