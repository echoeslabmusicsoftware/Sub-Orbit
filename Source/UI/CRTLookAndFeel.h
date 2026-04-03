#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class CRTLookAndFeel : public juce::LookAndFeel_V4
{
  public:
    static juce::Colour getVoidBlack ()
    {
        return juce::Colour (0xFF0C0C0C);
    }
    static juce::Colour getTerminalGreen ()
    {
        return juce::Colour (0xFF00D966);
    }
    static juce::Colour getCleanLight ()
    {
        return juce::Colour (0xFFFFFFFF);
    }
    static juce::Colour getMagentaInterference ()
    {
        return juce::Colour (0xFFE383FA);
    }

    CRTLookAndFeel ();

    std::unique_ptr<juce::FocusOutline> createFocusOutlineForComponent (juce::Component&) override;

    juce::Font getTerminalFont (float height) const;

    juce::Font getLabelFont (juce::Label& label) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawRotarySlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool, bool) override;

    void drawLabel (juce::Graphics& g, juce::Label& label) override;

    static void drawScanlines (juce::Graphics& g, juce::Rectangle<int> bounds);

    static void drawGradientOverlay (juce::Graphics& g, juce::Rectangle<int> bounds);

    static void drawVignette (juce::Graphics& g, juce::Rectangle<int> bounds);

    static void drawPhosphorGlow (juce::Graphics& g,
                                  juce::Rectangle<int> textBounds,
                                  const juce::String& text,
                                  const juce::Font& font);

    static void drawTerminalBorder (juce::Graphics& g,
                                    juce::Rectangle<float> bounds,
                                    float cornerRadius,
                                    juce::Colour colour = getTerminalGreen ());

  private:
    juce::Typeface::Ptr terminalTypeface;
};
