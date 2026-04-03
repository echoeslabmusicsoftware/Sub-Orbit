#include "FieldMonitorPanel.h"

#include "UIConstants.h"

FieldMonitorPanel::FieldMonitorPanel (ScopeDataFifo& fifo) : scopeFifo (fifo), goniometer (scopeFifo)
{
    addAndMakeVisible (goniometer);
    addAndMakeVisible (statusLabel);
    statusLabel.setJustificationType (juce::Justification::centredRight);
    statusLabel.setText ({}, juce::dontSendNotification);
    statusLabel.setFont (juce::FontOptions (11.0f));

    addAndMakeVisible (correlationMeter);

    addAndMakeVisible (correlationRiskLabel);
    correlationRiskLabel.setText ("RISK", juce::dontSendNotification);
    correlationRiskLabel.setFont (juce::FontOptions (9.0f));

    addAndMakeVisible (correlationSafeLabel);
    correlationSafeLabel.setText ("OK", juce::dontSendNotification);
    correlationSafeLabel.setJustificationType (juce::Justification::centredRight);
    correlationSafeLabel.setFont (juce::FontOptions (9.0f));

    addAndMakeVisible (monoCheckButton);
    monoCheckButton.setButtonText ("CHECK MONO");
    monoCheckButton.setTooltip ("Hold to audition the post-plugin output in mono.");
    monoCheckButton.setTriggeredOnMouseDown (true);
    monoCheckButton.setHasFocusOutline (true);
    monoCheckButton.setWantsKeyboardFocus (true);
}

void FieldMonitorPanel::resized ()
{
    auto bounds = getLocalBounds ().reduced (14);
    auto topRow = bounds.removeFromTop (18);
    statusLabel.setBounds (topRow.removeFromRight (140));
    bounds.removeFromTop (6);

    auto meterArea = bounds.removeFromBottom (42);
    auto meterLabels = meterArea.removeFromBottom (12);
    correlationMeter.setBounds (meterArea.removeFromTop (20));

    correlationRiskLabel.setBounds (meterLabels.removeFromLeft (48));
    correlationSafeLabel.setBounds (meterLabels.removeFromRight (48));

    // Center the CHECK MONO button between RISK and SAFE labels
    const auto buttonWidth = 80;
    const auto buttonX = meterLabels.getCentreX () - buttonWidth / 2;
    monoCheckButton.setBounds (buttonX, meterLabels.getY (), buttonWidth, meterLabels.getHeight ());

    goniometer.setBounds (bounds);
}

void FieldMonitorPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds ().toFloat ();
    CRTLookAndFeel::drawTerminalBorder (g, bounds.reduced (0.5f), 8.0f);
}

void FieldMonitorPanel::setAnalysisState (const SubOrbitAnalysisState& newState)
{
    correlationMeter.setCorrelation (newState.lowBandCorrelation);

    if (!newState.sampleRateSupported)
    {
        statusLabel.setText ("UNSUPPORTED RATE", juce::dontSendNotification);
        statusLabel.setVisible (true);
        statusLabel.setColour (juce::Label::textColourId, CRTLookAndFeel::getMagentaInterference ());
        return;
    }

    statusLabel.setVisible (false);
}

FieldMonitorPanel::GoniometerComponent::GoniometerComponent (ScopeDataFifo& fifo) : scopeFifo (fifo)
{
}

void FieldMonitorPanel::GoniometerComponent::resized ()
{
    const auto w = getWidth ();
    const auto h = getHeight ();

    if (w > 0 && h > 0)
    {
        historyBuffer = juce::Image (juce::Image::ARGB, w, h, true);
        rebuildDotMask ();
    }
}

void FieldMonitorPanel::GoniometerComponent::rebuildDotMask ()
{
    const auto w = getWidth ();
    const auto h = getHeight ();
    dotMask = juce::Image (juce::Image::ARGB, w, h, true);

    juce::Image::BitmapData bits (dotMask, juce::Image::BitmapData::writeOnly);
    const auto green = CRTLookAndFeel::getTerminalGreen ();

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const auto cellX = x % 3;
            const auto cellY = y % 3;

            if (cellX == 2 || cellY == 2)
                bits.setPixelColour (x, y, juce::Colours::black.withAlpha (0.15f));
            else
                bits.setPixelColour (x, y, green.withAlpha (0.02f));
        }
    }
}

void FieldMonitorPanel::GoniometerComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF0A0E0B));

    if (!historyBuffer.isValid ())
        return;

    // --- Fade the history buffer for phosphor persistence ---
    {
        juce::Image::BitmapData bits (historyBuffer, juce::Image::BitmapData::readWrite);
        const auto w = bits.width;
        const auto h = bits.height;

        for (int y = 0; y < h; ++y)
        {
            auto* pixel = bits.getLinePointer (y);
            for (int x = 0; x < w; ++x)
            {
                const auto offset = x * bits.pixelStride;
                auto& a = pixel[offset + 3];
                auto& r = pixel[offset + 2];
                auto& gg = pixel[offset + 1];
                auto& b = pixel[offset + 0];

                a = static_cast<juce::uint8> (a * 0.88f);
                r = static_cast<juce::uint8> (r * 0.92f);
                gg = static_cast<juce::uint8> (gg * 0.94f);
                b = static_cast<juce::uint8> (b * 0.88f);
            }
        }
    }

    // --- Draw new trace onto history buffer ---
    if (sampleCount > 0)
    {
        juce::Graphics hg (historyBuffer);
        const auto w = static_cast<float> (getWidth ());
        const auto h = static_cast<float> (getHeight ());

        juce::Path trace;
        constexpr auto kScale = 0.34f;
        const auto side0 = samples[0].left - samples[0].right;
        const auto mid0 = samples[0].left + samples[0].right;
        auto firstPoint = juce::Point<float> (w * (0.5f + kScale * side0), h * (0.5f - kScale * mid0));
        trace.startNewSubPath (firstPoint);

        auto lastPoint = firstPoint;

        for (int index = 1; index < sampleCount; ++index)
        {
            const auto side = samples[static_cast<size_t> (index)].left - samples[static_cast<size_t> (index)].right;
            const auto mid = samples[static_cast<size_t> (index)].left + samples[static_cast<size_t> (index)].right;
            const auto point = juce::Point<float> (w * (0.5f + kScale * side), h * (0.5f - kScale * mid));

            const auto dx = point.x - lastPoint.x;
            const auto dy = point.y - lastPoint.y;

            if (dx * dx + dy * dy >= 1.0f)
            {
                trace.lineTo (point);
                lastPoint = point;
            }
        }

        // Broad glow layer (phosphor bloom)
        hg.setColour (CRTLookAndFeel::getTerminalGreen ().withAlpha (0.12f));
        hg.strokePath (trace, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Mid glow
        hg.setColour (CRTLookAndFeel::getTerminalGreen ().withAlpha (0.25f));
        hg.strokePath (trace, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Hot core trace
        hg.setColour (CRTLookAndFeel::getTerminalGreen ().withAlpha (0.95f));
        hg.strokePath (trace, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // White-hot center where beam is brightest
        hg.setColour (CRTLookAndFeel::getCleanLight ().withAlpha (0.4f));
        hg.strokePath (trace, juce::PathStrokeType (0.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // --- Composite history buffer onto screen ---
    g.drawImageAt (historyBuffer, 0, 0);

    // --- CRT dot mask overlay ---
    if (dotMask.isValid ())
    {
        g.setOpacity (1.0f);
        g.drawImageAt (dotMask, 0, 0);
    }

    // --- Grid lines (on top of everything) ---
    g.setColour (CRTLookAndFeel::getTerminalGreen ().withAlpha (0.05f));
    for (int x = 1; x < 4; ++x)
        g.drawVerticalLine (getWidth () * x / 4, 0.0f, static_cast<float> (getHeight ()));
    for (int y = 1; y < 4; ++y)
        g.drawHorizontalLine (getHeight () * y / 4, 0.0f, static_cast<float> (getWidth ()));

    g.setColour (CRTLookAndFeel::getTerminalGreen ().withAlpha (0.1f));
    g.drawHorizontalLine (getHeight () / 2, 0.0f, static_cast<float> (getWidth ()));
    g.drawVerticalLine (getWidth () / 2, 0.0f, static_cast<float> (getHeight ()));

    // --- Border (outermost) ---
    CRTLookAndFeel::drawTerminalBorder (g, getLocalBounds ().toFloat ().reduced (1.0f), 6.0f);
}

void FieldMonitorPanel::GoniometerComponent::vblankCallback ()
{
    sampleCount = scopeFifo.pull (samples.data (), static_cast<int> (samples.size ()));
    repaint ();
}

void FieldMonitorPanel::CorrelationMeter::setCorrelation (float newCorrelation)
{
    correlation = juce::jlimit (-1.0f, 1.0f, newCorrelation);
    repaint ();
}

void FieldMonitorPanel::CorrelationMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds ().toFloat ();
    g.setColour (CRTLookAndFeel::getVoidBlack ());
    g.fillRoundedRectangle (bounds, 4.0f);
    CRTLookAndFeel::drawTerminalBorder (
        g, bounds.reduced (0.5f), 4.0f, CRTLookAndFeel::getTerminalGreen ().withAlpha (0.45f));

    const auto centerX = bounds.getCentreX ();
    const auto quarterWidth = bounds.getWidth () * 0.25f;
    g.setColour (CRTLookAndFeel::getMagentaInterference ().withAlpha (0.14f));
    g.fillRect (juce::Rectangle<float> (bounds.getX (), bounds.getY (), quarterWidth, bounds.getHeight ()));

    g.setColour (CRTLookAndFeel::getTerminalGreen ().withAlpha (0.10f));
    g.fillRect (juce::Rectangle<float> (centerX, bounds.getY (), bounds.getRight () - centerX, bounds.getHeight ()));

    g.setColour (CRTLookAndFeel::getTerminalGreen ().withAlpha (0.12f));
    g.drawLine (centerX, bounds.getY (), centerX, bounds.getBottom (), 1.0f);
    g.drawLine (
        bounds.getX () + quarterWidth, bounds.getY (), bounds.getX () + quarterWidth, bounds.getBottom (), 1.0f);
    g.drawLine (bounds.getRight () - quarterWidth,
                bounds.getY (),
                bounds.getRight () - quarterWidth,
                bounds.getBottom (),
                1.0f);

    g.setColour (CRTLookAndFeel::getCleanLight ());
    const auto markerX = juce::jmap (correlation, -1.0f, 1.0f, bounds.getX (), bounds.getRight ());
    g.drawLine (markerX, bounds.getY (), markerX, bounds.getBottom (), 2.0f);
}
