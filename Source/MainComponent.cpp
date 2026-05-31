#include "MainComponent.h"

#include "BinaryData.h"

MainComponent::MainComponent()
{
    const juce::String tilesJson = juce::String::fromUTF8 (BinaryData::tiles_json,
                                                           BinaryData::tiles_jsonSize);
    state    = std::make_unique<game::GameState> (tilesJson);
    gameView = std::make_unique<view::GameView>  (*state);

    addAndMakeVisible (*gameView);
    setSize (1280, 720);
}

MainComponent::~MainComponent() = default;

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void MainComponent::resized()
{
    gameView->setBounds (getLocalBounds());
}
