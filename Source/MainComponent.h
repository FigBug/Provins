#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <gin_controllers/gin_controllers.h>

class MainComponent : public juce::Component,
                      private gin::GameControllerManager::Listener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void controllerConnected      (gin::GameController&) override;
    void controllerDisconnected   (gin::GameController&) override;
    void controllerButtonPressed  (gin::GameController&, gin::GameController::Button) override;
    void controllerButtonReleased (gin::GameController&, gin::GameController::Button) override;
    void controllerAxisMoved      (gin::GameController&, gin::GameController::Axis, float) override;

    gin::GameControllerManager controllers;
    juce::String status { "No controller - connect one" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
