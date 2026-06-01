#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <gin_controllers/gin_controllers.h>

#include "audio/SoundBank.h"
#include "game/GameState.h"
#include "view/GameView.h"
#include "view/Hud.h"
#include "view/TitleScreen.h"

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    void timerCallback() override;

    void startGame();
    void returnToTitle();

    audio::SoundBank                      soundBank;
    gin::GameControllerManager            controllers;
    std::unique_ptr<view::TitleScreen>    titleScreen;
    std::unique_ptr<game::GameState>      state;
    std::unique_ptr<view::GameView>       gameView;
    std::unique_ptr<view::Hud>            hud;
    double                                lastTickMs = 0.0;
    bool                                  inGame     = false;
    bool                                  endScreenReady = false;
    juce::String                          tilesJson;
    juce::ApplicationProperties            appProperties;
    int                                   savedNumPlayers     = 2;
    int                                   savedTileMultiplier = 1;

    void loadSettings();
    void saveSettings();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
