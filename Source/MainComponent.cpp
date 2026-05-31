#include "MainComponent.h"

namespace
{
    const char* buttonName (gin::GameController::Button b)
    {
        using B = gin::GameController::Button;
        switch (b)
        {
            case B::dpadUp:        return "DPad Up";
            case B::dpadDown:      return "DPad Down";
            case B::dpadLeft:      return "DPad Left";
            case B::dpadRight:     return "DPad Right";
            case B::faceDown:      return "A / Cross";
            case B::faceRight:     return "B / Circle";
            case B::faceUp:        return "Y / Triangle";
            case B::faceLeft:      return "X / Square";
            case B::leftShoulder:  return "L Shoulder";
            case B::rightShoulder: return "R Shoulder";
            case B::leftTrigger:   return "L Trigger";
            case B::rightTrigger:  return "R Trigger";
            case B::select:        return "Select";
            case B::start:         return "Start";
            case B::home:          return "Home";
            case B::leftStick:     return "L Stick Click";
            case B::rightStick:    return "R Stick Click";
            default:               return "?";
        }
    }
}

MainComponent::MainComponent()
{
    setSize (1280, 720);
    controllers.addListener (this);
}

MainComponent::~MainComponent()
{
    controllers.removeListener (this);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (32.0f));
    g.drawText ("Provins", getLocalBounds().reduced (0, 60),
                juce::Justification::centredTop, false);

    g.setFont (juce::FontOptions (16.0f));
    g.drawText (status, getLocalBounds(), juce::Justification::centred, false);
}

void MainComponent::resized() {}

void MainComponent::controllerConnected (gin::GameController& c)
{
    status = "Connected: " + c.getName() + " (port " + juce::String (c.getIndex()) + ")";
    repaint();
}

void MainComponent::controllerDisconnected (gin::GameController& c)
{
    status = "Disconnected: port " + juce::String (c.getIndex());
    repaint();
}

void MainComponent::controllerButtonPressed (gin::GameController& c, gin::GameController::Button b)
{
    status = "Port " + juce::String (c.getIndex()) + ": " + buttonName (b) + " pressed";
    repaint();
}

void MainComponent::controllerButtonReleased (gin::GameController&, gin::GameController::Button) {}

void MainComponent::controllerAxisMoved (gin::GameController&, gin::GameController::Axis, float) {}
