#include "MainComponent.h"

#include "BinaryData.h"

namespace
{
    constexpr int kTickHz = 60;
}

MainComponent::MainComponent()
{
    tilesJson = juce::String::fromUTF8 (BinaryData::tiles_json,
                                        BinaryData::tiles_jsonSize);

    titleScreen = std::make_unique<view::TitleScreen> (controllers);
    addAndMakeVisible (*titleScreen);

    setWantsKeyboardFocus (true);
    setSize (1280, 720);
    startTimerHz (kTickHz);
}

MainComponent::~MainComponent() = default;

void MainComponent::startGame()
{
    inGame = true;
    endScreenReady = false;

    int numPlayers     = titleScreen->getNumPlayers();
    int tileMultiplier = titleScreen->getTileMultiplier();
    bool slotHasController[4] {};
    for (int i = 0; i < 4; ++i)
        slotHasController[i] = titleScreen->isControllerConnected (i);

    removeChildComponent (titleScreen.get());
    titleScreen.reset();

    state    = std::make_unique<game::GameState> (tilesJson, numPlayers, tileMultiplier);
    gameView = std::make_unique<view::GameView>  (*state);
    hud      = std::make_unique<view::Hud>       (*state);

    for (int i = 0; i < numPlayers; ++i)
    {
        if (slotHasController[i])
            state->spawnPlayer (i);
        else
            state->spawnAi (i);
    }

    addAndMakeVisible (*gameView);
    addAndMakeVisible (*hud);
    resized();
}

bool MainComponent::keyPressed (const juce::KeyPress&)
{
    if (inGame && state && state->isGameOver())
    {
        returnToTitle();
        return true;
    }
    return false;
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void MainComponent::resized()
{
    if (titleScreen)
        titleScreen->setBounds (getLocalBounds());
    if (gameView)
        gameView->setBounds (getLocalBounds());
    if (hud)
        hud->setBounds (getLocalBounds());
}

void MainComponent::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float  dt  = lastTickMs > 0.0 ? float ((now - lastTickMs) / 1000.0) : 0.0f;
    lastTickMs = now;

    if (! inGame)
    {
        titleScreen->update();
        titleScreen->repaint();

        if (titleScreen->isStartPressed())
            startGame();

        return;
    }

    state->update (dt, controllers);
    gameView->update (dt);
    gameView->repaint();
    hud->repaint();

    if (state->isGameOver())
    {
        bool anyButton = false;
        for (int i = 0; i < 4; ++i)
            if (auto* c = controllers.getController (i))
                if (c->isConnected())
                    if (c->isButtonDown (gin::GameController::Button::faceDown)
                     || c->isButtonDown (gin::GameController::Button::start))
                        anyButton = true;

        if (! anyButton)
            endScreenReady = true;

        if (anyButton && endScreenReady)
            returnToTitle();
    }
}

void MainComponent::returnToTitle()
{
    inGame = false;

    removeChildComponent (gameView.get());
    removeChildComponent (hud.get());
    gameView.reset();
    hud.reset();
    state.reset();

    titleScreen = std::make_unique<view::TitleScreen> (controllers);
    addAndMakeVisible (*titleScreen);
    resized();
    grabKeyboardFocus();
}
