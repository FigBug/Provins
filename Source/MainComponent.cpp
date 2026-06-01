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

    loadSettings();

    titleScreen = std::make_unique<view::TitleScreen> (controllers,
                                                       savedNumPlayers,
                                                       savedTileMultiplier,
                                                       soundBank.getVolume());
    addAndMakeVisible (*titleScreen);

    setWantsKeyboardFocus (true);
    setSize (1280, 720);
    startTimerHz (kTickHz);
}

MainComponent::~MainComponent() = default;

void MainComponent::loadSettings()
{
    juce::PropertiesFile::Options opts;
    opts.applicationName     = "Provins";
    opts.folderName          = "Provins";
    opts.filenameSuffix       = ".settings";
    opts.osxLibrarySubFolder = "Application Support";
    appProperties.setStorageParameters (opts);

    if (auto* props = appProperties.getUserSettings())
    {
        savedNumPlayers     = props->getIntValue ("numPlayers", 2);
        savedTileMultiplier = props->getIntValue ("tileMultiplier", 1);
        soundBank.setVolume ((float) props->getDoubleValue ("volume", 1.0));
    }
}

void MainComponent::saveSettings()
{
    if (auto* props = appProperties.getUserSettings())
    {
        props->setValue ("numPlayers",     savedNumPlayers);
        props->setValue ("tileMultiplier", savedTileMultiplier);
        props->setValue ("volume",         (double) soundBank.getVolume());
        props->saveIfNeeded();
    }
}

void MainComponent::startGame()
{
    inGame = true;
    endScreenReady = false;

    int numPlayers     = titleScreen->getNumPlayers();
    int tileMultiplier = titleScreen->getTileMultiplier();

    savedNumPlayers     = numPlayers;
    savedTileMultiplier = tileMultiplier;
    soundBank.setVolume (titleScreen->getVolume());
    saveSettings();
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

    for (const auto& ev : state->getSoundEvents())
    {
        using SE = game::GameState::SoundEvent;

        float pan = 0.0f;
        auto bounds = state->getBoard().bounds();
        if (bounds.has_value() && bounds->getWidth() > 0)
        {
            float centre = bounds->toFloat().getCentreX();
            float halfW  = bounds->toFloat().getWidth() * 0.5f + 1.0f;
            pan = juce::jlimit (-1.0f, 1.0f, (ev.worldPos.x - centre) / halfW);
        }

        switch (ev.type)
        {
            case SE::tilePlace: soundBank.play (audio::SoundID::tilePlace, 1.0f, pan); break;
            case SE::claim:     soundBank.play (audio::SoundID::claim,     1.0f, pan); break;
            case SE::complete:  soundBank.play (audio::SoundID::complete,  1.0f, pan); break;
            case SE::rotate:    soundBank.play (audio::SoundID::rotate,    0.5f, pan); break;
            case SE::gameOver:  soundBank.play (audio::SoundID::gameOver);             break;
        }
    }
    state->clearSoundEvents();

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

    titleScreen = std::make_unique<view::TitleScreen> (controllers,
                                                      savedNumPlayers,
                                                      savedTileMultiplier,
                                                      soundBank.getVolume());
    addAndMakeVisible (*titleScreen);
    resized();
    grabKeyboardFocus();
}
