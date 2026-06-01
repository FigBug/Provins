#include "TitleScreen.h"
#include "MeepleRenderer.h"

namespace view
{

namespace
{
    const juce::Colour kSlotColours[4] {
        juce::Colour::fromRGB (230,  70,  70),
        juce::Colour::fromRGB ( 70, 140, 230),
        juce::Colour::fromRGB ( 80, 200,  90),
        juce::Colour::fromRGB (240, 200,  60),
    };
}

TitleScreen::TitleScreen (gin::GameControllerManager& controllers_)
    : controllers (controllers_)
{
    setWantsKeyboardFocus (true);
    juce::Timer::callAfterDelay (100, [this]
    {
        grabKeyboardFocus();
    });
}

void TitleScreen::parentHierarchyChanged()
{
}

bool TitleScreen::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::leftKey)
        numPlayers = juce::jmax (numPlayers - 1, 2);
    else if (key == juce::KeyPress::rightKey)
        numPlayers = juce::jmax (juce::jmin (numPlayers + 1, 4), 2);
    else if (key == juce::KeyPress::upKey)
        tileMultiplier = juce::jmin (tileMultiplier + 1, kMaxMultiplier);
    else if (key == juce::KeyPress::downKey)
        tileMultiplier = juce::jmax (tileMultiplier - 1, kMinMultiplier);
    else
        startPressed = true;

    repaint();
    return true;
}

bool TitleScreen::isControllerConnected (int slot) const noexcept
{
    if (auto* c = controllers.getController (slot))
        return c->isConnected();
    return false;
}

void TitleScreen::update()
{
    using B = gin::GameController::Button;

    bool anyStart    = false;
    bool anyLeft     = false;
    bool anyRight    = false;
    bool anyDpadUp   = false;
    bool anyDpadDown = false;

    for (int i = 0; i < 4; ++i)
    {
        if (auto* c = controllers.getController (i))
        {
            if (! c->isConnected()) continue;

            if (c->isButtonDown (B::faceDown)
             || c->isButtonDown (B::start))
                anyStart = true;

            if (c->isButtonDown (B::leftShoulder))
                anyLeft = true;

            if (c->isButtonDown (B::rightShoulder))
                anyRight = true;

            if (c->isButtonDown (B::dpadUp))
                anyDpadUp = true;

            if (c->isButtonDown (B::dpadDown))
                anyDpadDown = true;
        }
    }

    if (anyRight && ! prevBumperRight)
        numPlayers = juce::jmin (numPlayers + 1, 4);
    if (anyLeft && ! prevBumperLeft)
        numPlayers = juce::jmax (numPlayers - 1, 2);

    if (anyDpadUp && ! prevDpadUp)
        tileMultiplier = juce::jmin (tileMultiplier + 1, kMaxMultiplier);
    if (anyDpadDown && ! prevDpadDown)
        tileMultiplier = juce::jmax (tileMultiplier - 1, kMinMultiplier);

    if (anyStart && ! prevStart)
        startPressed = true;

    prevBumperRight = anyRight;
    prevBumperLeft  = anyLeft;
    prevDpadUp      = anyDpadUp;
    prevDpadDown    = anyDpadDown;
    prevStart       = anyStart;
}

void TitleScreen::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll (juce::Colour::fromRGB (30, 30, 35));

    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    // Title
    g.setColour (juce::Colour::fromRGB (240, 220, 180));
    g.setFont (juce::Font (juce::FontOptions().withHeight (72.0f)).boldened());
    g.drawText ("PROVINS", bounds.removeFromTop (cy * 0.6f), juce::Justification::centredBottom);

    // Player count
    g.setFont (juce::Font (juce::FontOptions().withHeight (24.0f)));
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    juce::String countText = juce::String (numPlayers) + " Players";
    g.drawText (countText, (int)bounds.getX(), (int)(cy * 0.7f), (int)bounds.getWidth(), 30,
                juce::Justification::centred);

    // Tile multiplier
    g.setFont (juce::Font (juce::FontOptions().withHeight (20.0f)));
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    juce::String multText = juce::String (tileMultiplier) + "x tiles";
    g.drawText (multText, (int)bounds.getX(), (int)(cy * 0.8f), (int)bounds.getWidth(), 26,
                juce::Justification::centred);

    // Control hints under the multiplier readout.
    g.setFont (juce::Font (juce::FontOptions().withHeight (13.0f)));
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawText (juce::String::fromUTF8 ("LB/RB or \xe2\x86\x90\xe2\x86\x92  Players")
                + juce::String::fromUTF8 ("   \xe2\x80\xa2   DPad U/D or \xe2\x86\x91\xe2\x86\x93  Tiles"),
                (int)bounds.getX(), (int)(cy * 0.9f), (int)bounds.getWidth(), 22,
                juce::Justification::centred);

    // Meeples row
    float meepleY = cy * 1.05f;
    float spacing = 120.0f;
    float startX  = cx - (float)(numPlayers - 1) * spacing * 0.5f;

    float meepleScale = juce::jmin (bounds.getWidth() / 800.0f, 1.0f);

    float meepleWorldSize = 60.0f * meepleScale;

    for (int i = 0; i < numPlayers; ++i)
    {
        float mx = startX + (float)i * spacing;

        MeepleRenderer::draw (g, { mx, meepleY }, kSlotColours[i], meepleWorldSize);

        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.setFont (juce::Font (juce::FontOptions().withHeight (18.0f)));
        juce::String label = isControllerConnected (i)
            ? ("P" + juce::String (i + 1))
            : "AI";
        g.drawText (label, (int)(mx - 30), (int)(meepleY + meepleWorldSize * 0.55f), 60, 24,
                    juce::Justification::centred);
    }

    // Start prompt
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.setFont (juce::Font (juce::FontOptions().withHeight (20.0f)));
    g.drawText ("Press any button to start", (int)bounds.getX(), (int)(bounds.getBottom() - 80),
                (int)bounds.getWidth(), 30, juce::Justification::centred);
}

} // namespace view
