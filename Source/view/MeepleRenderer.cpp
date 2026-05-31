#include "MeepleRenderer.h"

namespace view
{

namespace
{
    // Builds a stylised meeple silhouette inscribed in a [-0.5, -0.5]..[0.5, 0.5] box.
    // Centred so (0, 0) is the meeple's logical "feet centre" reference point.
    juce::Path buildMeeplePath()
    {
        juce::Path p;

        // Head — small ellipse at the top.
        const float headW = 0.34f, headH = 0.36f;
        const float headCY = -0.28f;
        p.addEllipse (-headW * 0.5f, headCY - headH * 0.5f, headW, headH);

        // Body silhouette with arms out and slightly flared base.
        juce::Path body;
        body.startNewSubPath (-0.12f, -0.12f);   // left side of neck
        body.lineTo          (-0.42f, -0.02f);   // out to left shoulder
        body.lineTo          (-0.34f,  0.18f);   // wrist tucks under
        body.lineTo          (-0.18f,  0.18f);   // armpit
        body.lineTo          (-0.34f,  0.46f);   // outer foot
        body.lineTo          ( 0.34f,  0.46f);   // outer foot right
        body.lineTo          ( 0.18f,  0.18f);   // armpit right
        body.lineTo          ( 0.34f,  0.18f);   // wrist right
        body.lineTo          ( 0.42f, -0.02f);   // right shoulder
        body.lineTo          ( 0.12f, -0.12f);   // right side of neck
        body.closeSubPath();

        p.addPath (body);
        return p;
    }
}

void MeepleRenderer::draw (juce::Graphics& g,
                           juce::Point<float> position,
                           juce::Colour colour,
                           float worldSize)
{
    static const auto pathTemplate = buildMeeplePath();

    juce::Graphics::ScopedSaveState save (g);
    g.addTransform (juce::AffineTransform::scale (worldSize)
                                          .translated (position.x, position.y));

    g.setColour (colour);
    g.fillPath (pathTemplate);

    g.setColour (juce::Colours::black);
    g.strokePath (pathTemplate, juce::PathStrokeType (0.05f));
}

} // namespace view
