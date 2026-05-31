#include "MainComponent.h"

#include "BinaryData.h"

namespace
{
    constexpr int kTickHz = 60;
}

MainComponent::MainComponent()
{
    const juce::String tilesJson = juce::String::fromUTF8 (BinaryData::tiles_json,
                                                           BinaryData::tiles_jsonSize);
    state    = std::make_unique<game::GameState> (tilesJson);
    gameView = std::make_unique<view::GameView>  (*state);
    hud      = std::make_unique<view::Hud>       (*state);

    // Players spawn dynamically as controllers connect — handled in GameState::update.

    addAndMakeVisible (*gameView);
    addAndMakeVisible (*hud);
    setSize (1280, 720);

    startTimerHz (kTickHz);
}

MainComponent::~MainComponent() = default;

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void MainComponent::resized()
{
    gameView->setBounds (getLocalBounds());
    hud->setBounds      (getLocalBounds());
}

void MainComponent::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float  dt  = lastTickMs > 0.0 ? float ((now - lastTickMs) / 1000.0) : 0.0f;
    lastTickMs = now;

    state->update (dt, controllers);
    gameView->repaint();
    hud->repaint();
}
