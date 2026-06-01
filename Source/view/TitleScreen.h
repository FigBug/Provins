#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <gin_controllers/gin_controllers.h>

namespace view
{

class TitleScreen : public juce::Component
{
public:
    TitleScreen (gin::GameControllerManager& controllers);

    void paint (juce::Graphics& g) override;
    void parentHierarchyChanged() override;
    bool keyPressed (const juce::KeyPress& key) override;

    void update();

    int  getNumPlayers()     const noexcept { return numPlayers; }
    int  getTileMultiplier() const noexcept { return tileMultiplier; }
    bool isStartPressed()    const noexcept { return startPressed; }

    bool isControllerConnected (int slot) const noexcept;

    static constexpr int kMinMultiplier = 1;
    static constexpr int kMaxMultiplier = 10;

private:
    gin::GameControllerManager& controllers;
    int  numPlayers     = 2;
    int  tileMultiplier = 1;
    bool startPressed   = false;

    bool prevBumperLeft  = false;
    bool prevBumperRight = false;
    bool prevDpadUp      = false;
    bool prevDpadDown    = false;
    bool prevStart       = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TitleScreen)
};

} // namespace view
