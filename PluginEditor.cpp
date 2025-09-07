#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

// A more modern, detailed LookAndFeel class
class ApiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ApiLookAndFeel()
    {
        // Define a color palette
        setColour(juce::Slider::thumbColourId, juce::Colours::whitesmoke); // Pointer dot
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::black.withAlpha(0.6f)); // Outline
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff2b2b2b)); // Knob face (not used in gradient)
        setColour(juce::Label::textColourId, juce::Colours::lightgrey);

        // Define colors for the new illuminated buttons
        apiBlue = juce::Colour(0xff00529e);
        setColour(juce::TextButton::buttonOnColourId, apiBlue);
        setColour(juce::TextButton::buttonColourId, juce::Colours::black);
    }

    // A more detailed rotary slider with gradients and a modern pointer
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(10.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Knob Body
        juce::ColourGradient knobGradient(juce::Colour(0xff444444), centre.getX(), centre.getY(),
            juce::Colour(0xff222222), centre.getX(), centre.getY() + radius, true);
        g.setGradientFill(knobGradient);
        g.fillEllipse(bounds);

        // Outline
        g.setColour(findColour(juce::Slider::rotarySliderOutlineColourId));
        g.drawEllipse(bounds, 1.5f);

        // Pointer
        juce::Path p;
        auto pointerRadius = radius * 0.8f;
        p.addEllipse(0, -pointerRadius - 2, 4, 4); // A small dot near the edge
        p.applyTransform(juce::AffineTransform::rotation(toAngle, 0, 0).translated(centre));
        g.setColour(findColour(juce::Slider::thumbColourId));
        g.fillPath(p);
    }

    // Custom drawing for illuminated toggle buttons
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/)
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto circleBounds = bounds.withSizeKeepingCentre(bounds.getHeight(), bounds.getHeight()).reduced(2.0f); // Make it a circle

        if (button.getToggleState())
        {
            // --- ON STATE (Illuminated) ---
            juce::Colour onColour = findColour(juce::TextButton::buttonOnColourId);
            // Glow effect
            g.setColour(onColour.withAlpha(0.3f));
            g.fillEllipse(circleBounds.expanded(3.0f));

            // Main light
            juce::ColourGradient lightGradient(juce::Colours::white, circleBounds.getCentreX(), circleBounds.getCentreY() - circleBounds.getHeight() * 0.5f,
                onColour, circleBounds.getCentreX(), circleBounds.getCentreY() + circleBounds.getHeight(), true);
            g.setGradientFill(lightGradient);
            g.fillEllipse(circleBounds);
        }
        else
        {
            // --- OFF STATE ---
            juce::Colour offColour = findColour(juce::TextButton::buttonColourId);
            juce::ColourGradient darkGradient(offColour.brighter(0.1f), circleBounds.getCentreX(), circleBounds.getCentreY() - circleBounds.getHeight() * 0.5f,
                offColour.darker(0.5f), circleBounds.getCentreX(), circleBounds.getCentreY() + circleBounds.getHeight(), true);
            g.setGradientFill(darkGradient);
            g.fillEllipse(circleBounds);

            // Inner shadow to give depth
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.drawEllipse(circleBounds, 1.0f);
        }
    }

private:
    juce::Colour apiBlue;
};


Api550bAudioProcessorEditor::Api550bAudioProcessorEditor(Api550bAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    laf = std::make_unique<ApiLookAndFeel>();
    setLookAndFeel(laf.get());

    setupSlider(lowFreqSlider);
    setupSlider(lowMidFreqSlider);
    setupSlider(highMidFreqSlider);
    setupSlider(highFreqSlider);
    setupSlider(lowGainSlider);
    setupSlider(lowMidGainSlider);
    setupSlider(highMidGainSlider);
    setupSlider(highGainSlider);
    setupSlider(satDriveSlider);

    auto setupLabel = [&](juce::Label& label, const juce::String& text, float fontSize = 14.0f, juce::Justification just = juce::Justification::centred) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(just);
        label.setFont(juce::FontOptions(fontSize));
        addAndMakeVisible(label);
        };

    setupLabel(lowBandLabel, "LOW", 16.0f);
    setupLabel(lowMidBandLabel, "LOW-MID", 16.0f);
    setupLabel(highMidBandLabel, "HIGH-MID", 16.0f);
    setupLabel(highBandLabel, "HIGH", 16.0f);

    // Setup new labels for mute/bypass
    setupLabel(lowMuteLabel, "MUTE", 12.0f);
    setupLabel(lowBypassLabel, "BYPASS", 12.0f);
    setupLabel(lmMuteLabel, "MUTE", 12.0f);
    setupLabel(lmBypassLabel, "BYPASS", 12.0f);
    setupLabel(hmMuteLabel, "MUTE", 12.0f);
    setupLabel(hmBypassLabel, "BYPASS", 12.0f);
    setupLabel(highMuteLabel, "MUTE", 12.0f);
    setupLabel(highBypassLabel, "BYPASS", 12.0f);

    auto setupButton = [&](juce::Button& button, const juce::String& text) {
        addAndMakeVisible(button);
        button.setButtonText(text); // Text is not drawn, but useful for accessibility/debugging
        button.setClickingTogglesState(true);
        button.setToggleState(false, juce::dontSendNotification);
        };

    setupButton(lowShelfButton, "SHELF");
    setupButton(highShelfButton, "SHELF");
    setupButton(qModeButton, "Q MODE");
    setupButton(lowMuteButton, "MUTE");
    setupButton(lowBypassButton, "BYPASS");
    setupButton(lmMuteButton, "MUTE");
    setupButton(lmBypassButton, "BYPASS");
    setupButton(hmMuteButton, "MUTE");
    setupButton(hmBypassButton, "BYPASS");
    setupButton(highMuteButton, "MUTE");
    setupButton(highBypassButton, "BYPASS");

    // Attachments remain the same
    lowFreqAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, Params::LOW_FREQ, lowFreqSlider);
    lowGainAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, Params::LOW_GAIN, lowGainSlider);
    lowShelfAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, Params::LOW_SHELF, lowShelfButton);
    lowMuteAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, Params::LOW_MUTE, lowMuteButton);
    lowBypassAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, Params::LOW_BYPASS, lowBypassButton);
    lowMidFreqAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, Params::LM_FREQ, lowMidFreqSlider);
    lowMidGainAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, Params::LM_GAIN, lowMidGainSlider);
    lmMuteAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, Params::LM_MUTE, lmMuteButton);
    lmBypassAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, Params::LM_BYPASS, lmBypassButton);
    highMidFreqAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, Params::HM_FREQ, highMidFreqSlider);
    highMidGainAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, Params::HM_GAIN, highMidGainSlider);
    hmMuteAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, Params::HM_MUTE, hmMuteButton);
    hmBypassAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, Params::HM_BYPASS, hmBypassButton);
    highFreqAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, Params::HIGH_FREQ, highFreqSlider);
    highGainAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, Params::HIGH_GAIN, highGainSlider);
    highShelfAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, Params::HIGH_SHELF, highShelfButton);
    highMuteAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, Params::HIGH_MUTE, highMuteButton);
    highBypassAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, Params::HIGH_BYPASS, highBypassButton);
    satDriveAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, Params::SAT_DRIVE, satDriveSlider);
    qModeAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, Params::Q_MODE, qModeButton);

    setSize(700, 480);
    setResizable(true, true);
    setResizeLimits(600, 420, 1200, 840);
}

Api550bAudioProcessorEditor::~Api550bAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void Api550bAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff00529e)); // Dark charcoal background

    auto bounds = getLocalBounds();
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("EQ Alpha 3", bounds.removeFromTop(50), juce::Justification::centred, true);

    auto panelBounds = bounds.reduced(10);
    g.setColour(juce::Colour(0xff1a1a1a));

    const int numBands = 4;
    const int panelWidth = panelBounds.getWidth() / numBands;

    for (int i = 0; i < numBands; ++i)
    {
        auto panel = panelBounds.withX(panelBounds.getX() + i * panelWidth).withWidth(panelWidth - 5).toFloat();
        g.fillRoundedRectangle(panel, 10.0f);
    }
}

void Api550bAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(50); // Space for title
    bounds.reduce(15, 15);

    const int numBands = 4;
    const int bandWidth = (bounds.getWidth() - (numBands - 1) * 10) / numBands;

    auto layoutBand = [&](juce::Rectangle<int> area, juce::Label& bandLabel,
        juce::Slider& freqSlider, juce::Slider& gainSlider, juce::Component* extraControl,
        juce::Label* extraLabel,
        juce::Button* muteButton, juce::Label* muteLabel,
        juce::Button* bypassButton, juce::Label* bypassLabel)
        {
            const int labelHeight = 25;
            const int knobSize = 70;
            const int textBoxHeight = 20;
            const int sliderHeight = knobSize + textBoxHeight;
            const int buttonSize = 20;
            const int buttonLabelHeight = 15;
            const int spacing = 10;

            bandLabel.setBounds(area.removeFromTop(labelHeight));
            area.removeFromTop(spacing);

            freqSlider.setBounds(area.removeFromTop(sliderHeight).withSizeKeepingCentre(knobSize, sliderHeight));
            gainSlider.setBounds(area.removeFromTop(sliderHeight).withSizeKeepingCentre(knobSize, sliderHeight));
            area.removeFromTop(spacing);

            if (extraControl)
            {
                if (extraLabel)
                    extraLabel->setBounds(area.removeFromTop(labelHeight));

                if (dynamic_cast<juce::Slider*>(extraControl))
                    extraControl->setBounds(area.removeFromTop(sliderHeight).withSizeKeepingCentre(knobSize, sliderHeight));
                else // For buttons (shelf, qMode)
                    extraControl->setBounds(area.removeFromTop(labelHeight).reduced(area.getWidth() / 5, 0));

                area.removeFromTop(spacing);
            }

            // Layout for new illuminated buttons and their labels
            if (muteButton && bypassButton && muteLabel && bypassLabel)
            {
                auto buttonArea = area.removeFromTop(buttonSize + buttonLabelHeight);
                int halfWidth = buttonArea.getWidth() / 2;

                auto muteArea = buttonArea.removeFromLeft(halfWidth);
                muteButton->setBounds(muteArea.removeFromTop(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize));
                muteLabel->setBounds(muteArea);

                auto bypassArea = buttonArea.removeFromRight(halfWidth);
                bypassButton->setBounds(bypassArea.removeFromTop(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize));
                bypassLabel->setBounds(bypassArea);
            }
        };

    for (int i = 0; i < numBands; ++i)
    {
        auto column = bounds.withX(bounds.getX() + i * (bandWidth + 10)).withWidth(bandWidth);

        if (i == 0) // Low Band
        {
            layoutBand(column, lowBandLabel, lowFreqSlider, lowGainSlider, &lowShelfButton, nullptr, &lowMuteButton, &lowMuteLabel, &lowBypassButton, &lowBypassLabel);
        }
        else if (i == 1) // Low-Mid Band
        {
            juce::Label satDriveLabel; // Temporary label for layout
            satDriveLabel.setText("DRIVE", juce::dontSendNotification);
            satDriveLabel.setJustificationType(juce::Justification::centred);
            layoutBand(column, lowMidBandLabel, lowMidFreqSlider, lowMidGainSlider, &satDriveSlider, &satDriveLabel, &lmMuteButton, &lmMuteLabel, &lmBypassButton, &lmBypassLabel);
        }
        else if (i == 2) // High-Mid Band
        {
            juce::Label qModeLabel;
            qModeLabel.setText("PROPORTIONAL Q", juce::dontSendNotification);
            qModeLabel.setJustificationType(juce::Justification::centred);
            layoutBand(column, highMidBandLabel, highMidFreqSlider, highMidGainSlider, &qModeButton, &qModeLabel, &hmMuteButton, &hmMuteLabel, &hmBypassButton, &hmBypassLabel);
        }
        else if (i == 3) // High Band
        {
            layoutBand(column, highBandLabel, highFreqSlider, highGainSlider, &highShelfButton, nullptr, &highMuteButton, &highMuteLabel, &highBypassButton, &highBypassLabel);
        }
    }
}

void Api550bAudioProcessorEditor::setupSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    slider.setTextValueSuffix("");
    addAndMakeVisible(slider);
}