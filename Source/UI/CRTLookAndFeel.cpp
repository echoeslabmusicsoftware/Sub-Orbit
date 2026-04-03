#include "CRTLookAndFeel.h"

#include <BinaryData.h>

namespace
{
struct CRTFocusOutlineWindowProperties final : public juce::FocusOutline::OutlineWindowProperties
{
    juce::Rectangle<int> getOutlineBounds (juce::Component& focusedComponent) override
    {
        return focusedComponent.getScreenBounds ().expanded (4);
    }

    void drawOutline (juce::Graphics& g, int width, int height) override
    {
        const auto bounds =
            juce::Rectangle<float> (static_cast<float> (width), static_cast<float> (height)).reduced (1.0f);
        g.setColour (CRTLookAndFeel::getCleanLight ().withAlpha (0.95f));
        g.drawRoundedRectangle (bounds, 6.0f, 1.2f);
        g.setColour (CRTLookAndFeel::getTerminalGreen ().withAlpha (0.85f));
        g.drawRoundedRectangle (bounds.reduced (2.0f), 5.0f, 1.6f);
    }
};
} // namespace

CRTLookAndFeel::CRTLookAndFeel ()
{
    if (BinaryData::PxPlus_IBM_VGA_9x16_ttfSize > 0)
        terminalTypeface = juce::Typeface::createSystemTypefaceFor (BinaryData::PxPlus_IBM_VGA_9x16_ttf,
                                                                    BinaryData::PxPlus_IBM_VGA_9x16_ttfSize);

    setColour (juce::ResizableWindow::backgroundColourId, getVoidBlack ());
    setColour (juce::Label::textColourId, getTerminalGreen ());
    setColour (juce::Slider::rotarySliderFillColourId, getTerminalGreen ());
    setColour (juce::Slider::rotarySliderOutlineColourId, getTerminalGreen ().withAlpha (0.2f));
    setColour (juce::Slider::thumbColourId, getCleanLight ());
    setColour (juce::TextButton::buttonColourId, getVoidBlack ());
    setColour (juce::TextButton::buttonOnColourId, getTerminalGreen ().withAlpha (0.2f));
    setColour (juce::TextButton::textColourOffId, getTerminalGreen ());
    setColour (juce::TextButton::textColourOnId, getTerminalGreen ());
    setColour (juce::ToggleButton::textColourId, getTerminalGreen ());
}

std::unique_ptr<juce::FocusOutline> CRTLookAndFeel::createFocusOutlineForComponent (juce::Component&)
{
    return std::make_unique<juce::FocusOutline> (std::make_unique<CRTFocusOutlineWindowProperties> ());
}

juce::Font CRTLookAndFeel::getTerminalFont (float height) const
{
    if (terminalTypeface != nullptr)
        return juce::Font (juce::FontOptions (terminalTypeface).withHeight (height));

    return juce::Font (juce::FontOptions ("Menlo", height, juce::Font::plain));
}

juce::Font CRTLookAndFeel::getLabelFont (juce::Label& label)
{
    return getTerminalFont (label.getFont ().getHeight ());
}

juce::Font CRTLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return getTerminalFont (std::min (14.0f, static_cast<float> (buttonHeight) * 0.65f));
}

void CRTLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                       int x,
                                       int y,
                                       int width,
                                       int height,
                                       float sliderPosProportional,
                                       float rotaryStartAngle,
                                       float rotaryEndAngle,
                                       juce::Slider& slider)
{
    juce::ignoreUnused (slider);

    const auto bounds = juce::Rectangle<float> (
        static_cast<float> (x), static_cast<float> (y), static_cast<float> (width), static_cast<float> (height));
    const auto fullRadius = juce::jmin (bounds.getWidth (), bounds.getHeight ()) * 0.5f;
    const auto radius = fullRadius * 0.84f;
    const auto centre = bounds.getCentre ();
    const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    juce::Path backgroundArc;
    backgroundArc.addCentredArc (
        centre.x, centre.y, radius * 0.82f, radius * 0.82f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (getTerminalGreen ().withAlpha (0.18f));
    g.strokePath (backgroundArc,
                  juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, radius * 0.82f, radius * 0.82f, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (getTerminalGreen ().withAlpha (0.24f));
    g.strokePath (valueArc, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (getTerminalGreen ());
    g.strokePath (valueArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto tickRadius = radius * 0.97f;
    g.setColour (getTerminalGreen ().withAlpha (0.35f));
    for (int index = 0; index < 11; ++index)
    {
        const auto tickAngle =
            rotaryStartAngle + static_cast<float> (index) / 10.0f * (rotaryEndAngle - rotaryStartAngle);
        const auto point =
            juce::Point<float> (centre.x + tickRadius * std::cos (tickAngle - juce::MathConstants<float>::halfPi),
                                centre.y + tickRadius * std::sin (tickAngle - juce::MathConstants<float>::halfPi));
        g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre (point));
    }

    juce::Path pointer;
    pointer.addRectangle (-1.0f, -radius * 0.58f, 2.0f, radius * 0.58f);
    g.setColour (getCleanLight ());
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre (centre));
}

void CRTLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool, bool)
{
    auto bounds = button.getLocalBounds ().toFloat ().reduced (0.5f);
    const auto isActive = button.getToggleState () || button.isDown ();
    g.setColour (isActive ? getTerminalGreen ().withAlpha (0.14f) : getVoidBlack ());
    g.fillRoundedRectangle (bounds, 4.0f);
    drawTerminalBorder (g, bounds, 4.0f, isActive ? getTerminalGreen () : getTerminalGreen ().withAlpha (0.5f));
}

void CRTLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (getTerminalFont (label.getFont ().getHeight ()));
    g.drawFittedText (label.getText ().toUpperCase (), label.getLocalBounds (), label.getJustificationType (), 1);
}

void CRTLookAndFeel::drawScanlines (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const auto left = static_cast<float> (bounds.getX ());
    const auto right = static_cast<float> (bounds.getRight ());
    for (int y = bounds.getY (); y < bounds.getBottom (); y += 3)
    {
        g.setColour (getTerminalGreen ().withAlpha (0.04f));
        g.drawHorizontalLine (y, left, right);
        g.drawHorizontalLine (y + 1, left, right);
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.drawHorizontalLine (y + 2, left, right);
    }
}

void CRTLookAndFeel::drawGradientOverlay (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    juce::ColourGradient gradient (getTerminalGreen ().withAlpha (0.04f),
                                   static_cast<float> (bounds.getCentreX ()),
                                   static_cast<float> (bounds.getY ()),
                                   juce::Colours::transparentBlack,
                                   static_cast<float> (bounds.getCentreX ()),
                                   static_cast<float> (bounds.getBottom ()),
                                   false);
    g.setGradientFill (gradient);
    g.fillRect (bounds);
}

void CRTLookAndFeel::drawVignette (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const auto cx = static_cast<float> (bounds.getCentreX ());
    const auto cy = static_cast<float> (bounds.getCentreY ());
    const auto radius =
        std::hypot (static_cast<float> (bounds.getWidth ()), static_cast<float> (bounds.getHeight ())) * 0.5f;

    juce::ColourGradient vignette (
        juce::Colours::transparentBlack, cx, cy, juce::Colours::black.withAlpha (0.85f), cx, cy + radius, true);
    vignette.addColour (0.4, juce::Colours::transparentBlack);
    vignette.addColour (0.7, juce::Colours::black.withAlpha (0.4f));
    g.setGradientFill (vignette);
    g.fillRect (bounds);
}

void CRTLookAndFeel::drawPhosphorGlow (juce::Graphics& g,
                                       juce::Rectangle<int> textBounds,
                                       const juce::String& text,
                                       const juce::Font& font)
{
    const auto expanded = textBounds.expanded (30, 20);
    juce::Image glowImage (juce::Image::ARGB, expanded.getWidth (), expanded.getHeight (), true);
    {
        juce::Graphics glowG (glowImage);
        glowG.setColour (getTerminalGreen ().withAlpha (0.7f));
        glowG.setFont (font);
        const auto localTextBounds = textBounds.translated (-expanded.getX (), -expanded.getY ());
        glowG.drawFittedText (text, localTextBounds, juce::Justification::centred, 1);
    }

    juce::ImageConvolutionKernel kernel (15);
    kernel.createGaussianBlur (12.0f);
    kernel.applyToImage (glowImage, glowImage, glowImage.getBounds ());

    g.drawImageAt (glowImage, expanded.getX (), expanded.getY ());
}

void CRTLookAndFeel::drawTerminalBorder (juce::Graphics& g,
                                         juce::Rectangle<float> bounds,
                                         float cornerRadius,
                                         juce::Colour colour)
{
    g.setColour (colour);
    g.drawRoundedRectangle (bounds, cornerRadius, 1.2f);
}
