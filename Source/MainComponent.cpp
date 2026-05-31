#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize (1280, 720);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (32.0f));
    g.drawText ("Provins", getLocalBounds(), juce::Justification::centred, false);
}

void MainComponent::resized()
{
}
