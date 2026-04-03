#include "TipsOverlay.h"

#include <BinaryData.h>

#include "UIConstants.h"

TipsOverlay::TipsOverlay ()
{
    panel = std::make_unique<TipsPanel> ();
    addAndMakeVisible (*panel);
    setWantsKeyboardFocus (true);
    setVisible (false);
}

void TipsOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.75f));
}

void TipsOverlay::resized ()
{
    const auto panelWidth = 400;
    const auto panelHeight = 380;
    const auto x = (getWidth () - panelWidth) / 2;
    const auto y = (getHeight () - panelHeight) / 2;
    panel->setBounds (x, y, panelWidth, panelHeight);
}

void TipsOverlay::mouseDown (const juce::MouseEvent& event)
{
    if (!panel->getBounds ().contains (event.getPosition ()))
        setVisible (false);
}

bool TipsOverlay::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        setVisible (false);
        return true;
    }
    return false;
}

// ============================================================================

TipsOverlay::TipsPanel::TipsPanel ()
{
    // Title
    addAndMakeVisible (titleLabel);
    titleLabel.setText ("SUB ORBIT", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::FontOptions (18.0f));
    titleLabel.setColour (juce::Label::textColourId, CRTLookAndFeel::getTerminalGreen ());

    // Close button — styled as X in top-right
    addAndMakeVisible (closeButton);
    closeButton.setButtonText ("X");
    closeButton.onClick = [this]
    {
        if (auto* overlay = findParentComponentOfClass<TipsOverlay> ())
            overlay->setVisible (false);
    };

    // Website link
    addAndMakeVisible (linkButton);
    linkButton.setButtonText ("echoeslabmusic.com");
    linkButton.setURL (juce::URL ("https://www.echoeslabmusic.com"));
    linkButton.setColour (juce::HyperlinkButton::textColourId, CRTLookAndFeel::getTerminalGreen ().withAlpha (0.7f));
    linkButton.setFont (juce::FontOptions (10.0f), false);

    // Version label
    addAndMakeVisible (versionLabel);
    versionLabel.setText (juce::String ("v") + JucePlugin_VersionString, juce::dontSendNotification);
    versionLabel.setJustificationType (juce::Justification::centred);
    versionLabel.setFont (juce::FontOptions (10.0f));
    versionLabel.setColour (juce::Label::textColourId, CRTLookAndFeel::getTerminalGreen ().withAlpha (0.5f));
}

void TipsOverlay::TipsPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds ().toFloat ();

    // Background
    g.setColour (CRTLookAndFeel::getVoidBlack ());
    g.fillRoundedRectangle (bounds, 8.0f);

    // Border
    CRTLookAndFeel::drawTerminalBorder (g, bounds.reduced (0.5f), 8.0f);

    // --- Tips content ---
    const auto green = CRTLookAndFeel::getTerminalGreen ();
    const auto dimGreen = green.withAlpha (0.5f);
    const auto left = 28;
    const auto bulletLeft = 20;
    const auto textWidth = getWidth () - 48;
    auto y = 52;

    const juce::StringArray tips = {
        "ORBIT sets stereo width for frequencies below the crossover.",
        "RANGE sets the crossover frequency (60-180 Hz).",
        "Route a sidechain signal to duck width on kick hits.",
        "ATTACK/RELEASE shape how fast ducking responds.",
        "Hold CHECK MONO to audition mono compatibility.",
        "Cmd+1/2/3 to resize the window.",
    };

    g.setFont (juce::FontOptions (11.5f));

    for (const auto& tip : tips)
    {
        // Bullet point
        g.setColour (dimGreen);
        g.drawText (juce::String::charToString (0x2022), bulletLeft, y, 10, 18, juce::Justification::centred, false);

        // Tip text
        g.setColour (green);
        const auto textHeight = 36;
        g.drawFittedText (tip, left, y, textWidth, textHeight, juce::Justification::centredLeft, 2);
        y += textHeight + 6;
    }

    // Separator line above footer
    const auto sepY = static_cast<float> (getHeight () - 56);
    g.setColour (green.withAlpha (0.2f));
    g.drawHorizontalLine (static_cast<int> (sepY), 20.0f, static_cast<float> (getWidth () - 20));
}

void TipsOverlay::TipsPanel::resized ()
{
    auto bounds = getLocalBounds ().reduced (16, 12);

    // Close button (X) in top-right corner
    auto headerArea = bounds.removeFromTop (32);
    closeButton.setBounds (headerArea.removeFromRight (32).withSizeKeepingCentre (28, 28));

    // Title centered in header
    titleLabel.setBounds (headerArea);

    // Footer: version on left, link on right
    auto footerArea = bounds.removeFromBottom (40);
    footerArea.removeFromTop (12); // space below separator

    const auto halfWidth = footerArea.getWidth () / 2;
    versionLabel.setBounds (footerArea.removeFromLeft (halfWidth));
    linkButton.setBounds (footerArea);
}
