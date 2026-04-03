#include "PluginEditor.h"

#include <BinaryData.h>

#include "SubOrbitParameters.h"
#include "UI/UIConstants.h"

SubOrbitAudioProcessorEditor::SubOrbitAudioProcessorEditor (SubOrbitAudioProcessor& audioProcessor)
    : juce::AudioProcessorEditor (audioProcessor), processorRef (audioProcessor),
      fieldMonitorPanel (processorRef.getScopeDataFifo ())
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);
    setWantsKeyboardFocus (true);
    setResizable (false, false);

    logoGreen =
        juce::Drawable::createFromImageData (BinaryData::echoeslabeyegreen_svg, BinaryData::echoeslabeyegreen_svgSize);

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("SUB ORBIT", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (28.0f));
    titleLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (fieldMonitorPanel);
    addAndMakeVisible (orbitPanel);
    addAndMakeVisible (sidechainPanel);

    tipsOverlay = std::make_unique<TipsOverlay> ();
    addChildComponent (*tipsOverlay);

    addAndMakeVisible (tipsButton);
    tipsButton.setButtonText ("?");
    tipsButton.setTooltip ("Show tips and usage information");
    tipsButton.setHasFocusOutline (true);
    tipsButton.setWantsKeyboardFocus (true);
    tipsButton.onClick = [this] { tipsOverlay->setVisible (!tipsOverlay->isVisible ()); };

    auto& valueTreeState = processorRef.getValueTreeState ();
    orbitAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        valueTreeState, std::string{SubOrbitParameters::kOrbitId}, orbitPanel.getOrbitSlider ());
    rangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        valueTreeState, std::string{SubOrbitParameters::kRangeHzId}, orbitPanel.getRangeSlider ());
    sidechainAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        valueTreeState, std::string{SubOrbitParameters::kSidechainAttackMsId}, sidechainPanel.getAttackSlider ());
    sidechainReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        valueTreeState, std::string{SubOrbitParameters::kSidechainReleaseMsId}, sidechainPanel.getReleaseSlider ());

    orbitPanel.getOrbitSlider ().setHasFocusOutline (true);
    orbitPanel.getOrbitSlider ().setWantsKeyboardFocus (true);
    orbitPanel.getOrbitSlider ().setExplicitFocusOrder (1);
    orbitPanel.getRangeSlider ().setHasFocusOutline (true);
    orbitPanel.getRangeSlider ().setWantsKeyboardFocus (true);
    orbitPanel.getRangeSlider ().setExplicitFocusOrder (2);

    sidechainPanel.getAttackSlider ().setHasFocusOutline (true);
    sidechainPanel.getAttackSlider ().setWantsKeyboardFocus (true);
    sidechainPanel.getAttackSlider ().setExplicitFocusOrder (3);
    sidechainPanel.getReleaseSlider ().setHasFocusOutline (true);
    sidechainPanel.getReleaseSlider ().setWantsKeyboardFocus (true);
    sidechainPanel.getReleaseSlider ().setExplicitFocusOrder (4);

    orbitPanel.getOrbitSlider ().onValueChange = [this]
    { orbitPanel.setOrbitStateLabel (static_cast<float> (orbitPanel.getOrbitSlider ().getValue ())); };

    orbitPanel.getRangeSlider ().onValueChange = [this]
    { orbitPanel.setRangeValueLabel (static_cast<float> (orbitPanel.getRangeSlider ().getValue ())); };

    orbitPanel.setOrbitStateLabel (static_cast<float> (orbitPanel.getOrbitSlider ().getValue ()));
    orbitPanel.setRangeValueLabel (static_cast<float> (orbitPanel.getRangeSlider ().getValue ()));

    fieldMonitorPanel.getMonoCheckButton ().onStateChange = [this]
    { processorRef.setMonoCheckActive (fieldMonitorPanel.getMonoCheckButton ().isDown ()); };

    applyScale (processorRef.getUiScale ());
}

SubOrbitAudioProcessorEditor::~SubOrbitAudioProcessorEditor ()
{
    processorRef.setMonoCheckActive (false);
    setLookAndFeel (nullptr);
}

void SubOrbitAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (CRTLookAndFeel::getVoidBlack ());

    if (cachedBackground.isValid ())
        g.drawImageAt (cachedBackground, 0, 0);

    g.setColour (CRTLookAndFeel::getTerminalGreen ().withAlpha (0.2f));
    g.drawHorizontalLine (UIConstants::outerPadding + UIConstants::headerHeight + 8,
                          static_cast<float> (UIConstants::outerPadding),
                          static_cast<float> (getWidth () - UIConstants::outerPadding));

    if (!logoBounds.isEmpty () && logoGreen != nullptr)
    {
        logoGreen->drawWithin (g, logoBounds.toFloat (), juce::RectanglePlacement::centred, 1.0f);
    }
}

void SubOrbitAudioProcessorEditor::resized ()
{
    rebuildBackground ();

    // Cover the entire editor with the overlay when visible
    tipsOverlay->setBounds (getLocalBounds ());

    auto bounds = getLocalBounds ().reduced (UIConstants::outerPadding);
    auto header = bounds.removeFromTop (UIConstants::headerHeight);
    bounds.removeFromTop (8);
    bounds.removeFromBottom (4);

    const auto logoSize = UIConstants::headerHeight - 4;
    logoBounds = header.removeFromLeft (logoSize).reduced (0, 2);
    header.removeFromLeft (6);

    // Tips button in header, far right
    const auto tipsButtonSize = 26;
    auto tipsArea = header.removeFromRight (tipsButtonSize);
    tipsButton.setBounds (tipsArea.withSizeKeepingCentre (tipsButtonSize, tipsButtonSize));

    const auto titleWidth = 280;
    const auto titleX = header.getX () + (header.getWidth () - titleWidth) / 2;
    titleLabel.setBounds (titleX, header.getY (), titleWidth, header.getHeight ());

    const auto rightPanelWidth = juce::jlimit (292, 336, bounds.getWidth () / 3 + 8);
    auto rightRail = bounds.removeFromRight (rightPanelWidth);
    bounds.removeFromRight (UIConstants::panelGap);

    fieldMonitorPanel.setBounds (bounds);

    // OrbitPanel gets ~60-65% of the right rail height, SidechainPanel gets fixed ~150px
    const auto sidechainHeight = 150;
    const auto orbitHeight = rightRail.getHeight () - sidechainHeight - 8;

    orbitPanel.setBounds (rightRail.removeFromTop (orbitHeight));
    rightRail.removeFromTop (8);
    sidechainPanel.setBounds (rightRail);

}

bool SubOrbitAudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress ('1', juce::ModifierKeys::commandModifier, 0))
    {
        applyScale (UiScale::small);
        return true;
    }

    if (key == juce::KeyPress ('2', juce::ModifierKeys::commandModifier, 0))
    {
        applyScale (UiScale::medium);
        return true;
    }

    if (key == juce::KeyPress ('3', juce::ModifierKeys::commandModifier, 0))
    {
        applyScale (UiScale::large);
        return true;
    }

    return false;
}

void SubOrbitAudioProcessorEditor::mouseUp (const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu ())
    {
        juce::PopupMenu menu;
        menu.addItem (1, juce::String ("Small") + juce::String::charToString (0x2318) + "1");
        menu.addItem (2, juce::String ("Medium") + juce::String::charToString (0x2318) + "2");
        menu.addItem (3, juce::String ("Large") + juce::String::charToString (0x2318) + "3");
        menu.showMenuAsync (juce::PopupMenu::Options ().withTargetComponent (this),
                            [this] (int result)
                            {
                                if (result == 1)
                                    applyScale (UiScale::small);
                                if (result == 2)
                                    applyScale (UiScale::medium);
                                if (result == 3)
                                    applyScale (UiScale::large);
                            });
    }
}

void SubOrbitAudioProcessorEditor::vblankCallback ()
{
    const auto state = processorRef.getAnalysisState ();
    fieldMonitorPanel.setAnalysisState (state);
}

void SubOrbitAudioProcessorEditor::rebuildBackground ()
{
    cachedBackground = juce::Image (juce::Image::ARGB, getWidth (), getHeight (), true);
    juce::Graphics g (cachedBackground);
    CRTLookAndFeel::drawVignette (g, getLocalBounds ());
    CRTLookAndFeel::drawGradientOverlay (g, getLocalBounds ());
    CRTLookAndFeel::drawScanlines (g, getLocalBounds ());

    CRTLookAndFeel::drawPhosphorGlow (
        g, titleLabel.getBounds (), titleLabel.getText ().toUpperCase (), lookAndFeel.getTerminalFont (28.0f));
}

void SubOrbitAudioProcessorEditor::applyScale (UiScale scale)
{
    processorRef.setUiScale (scale);

    switch (scale)
    {
    case UiScale::small:
        setSize (760, 430);
        break;
    case UiScale::medium:
        setSize (UIConstants::defaultWidth, UIConstants::defaultHeight);
        break;
    case UiScale::large:
        setSize (1040, 600);
        break;
    default:
        setSize (UIConstants::defaultWidth, UIConstants::defaultHeight);
        break;
    }
}
