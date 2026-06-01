#include "MeepleRenderer.h"
#include "BinaryData.h"

namespace view
{

namespace
{
    juce::Path buildMeeplePath()
    {
        auto svg = juce::XmlDocument::parse (
            juce::String::fromUTF8 (BinaryData::meeple_svg,
                                    BinaryData::meeple_svgSize));

        if (svg == nullptr)
            return {};

        auto* pathElement = svg->getChildByName ("path");
        if (pathElement == nullptr)
            return {};

        auto path = juce::Drawable::parseSVGPath (pathElement->getStringAttribute ("d"));

        auto bounds = path.getBounds();
        if (bounds.isEmpty())
            return {};

        float scale = 1.0f / juce::jmax (bounds.getWidth(), bounds.getHeight());

        path.applyTransform (juce::AffineTransform::translation (-bounds.getCentreX(),
                                                                  -bounds.getCentreY())
                                 .scaled (scale));
        return path;
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
