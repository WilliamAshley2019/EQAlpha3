#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ApiLookAndFeel;

class Api550bAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    Api550bAudioProcessorEditor(Api550bAudioProcessor&);
    ~Api550bAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    Api550bAudioProcessor& audioProcessor;

    juce::Slider lowFreqSlider, lowMidFreqSlider, highMidFreqSlider, highFreqSlider;
    juce::Slider lowGainSlider, lowMidGainSlider, highMidGainSlider, highGainSlider;
    juce::Slider satDriveSlider;
    juce::TextButton lowShelfButton, highShelfButton, qModeButton;
    juce::TextButton lowMuteButton, lowBypassButton;
    juce::TextButton lmMuteButton, lmBypassButton;
    juce::TextButton hmMuteButton, hmBypassButton;
    juce::TextButton highMuteButton, highBypassButton;
    juce::Label lowBandLabel, lowMidBandLabel, highMidBandLabel, highBandLabel;
    juce::Label lowMuteLabel, lowBypassLabel;
    juce::Label lmMuteLabel, lmBypassLabel;
    juce::Label hmMuteLabel, hmBypassLabel;
    juce::Label highMuteLabel, highBypassLabel;
    juce::Label satDriveLabel; // Kept for DRIVE label

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> lowFreqAttachment, lowGainAttachment, lowMidFreqAttachment, lowMidGainAttachment;
    std::unique_ptr<SliderAttachment> highMidFreqAttachment, highMidGainAttachment, highFreqAttachment, highGainAttachment;
    std::unique_ptr<SliderAttachment> satDriveAttachment;
    std::unique_ptr<ButtonAttachment> lowShelfAttachment, highShelfAttachment, qModeAttachment;
    std::unique_ptr<ButtonAttachment> lowMuteAttachment, lowBypassAttachment;
    std::unique_ptr<ButtonAttachment> lmMuteAttachment, lmBypassAttachment;
    std::unique_ptr<ButtonAttachment> hmMuteAttachment, hmBypassAttachment;
    std::unique_ptr<ButtonAttachment> highMuteAttachment, highBypassAttachment;

    std::unique_ptr<ApiLookAndFeel> laf;

    void setupSlider(juce::Slider& slider);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Api550bAudioProcessorEditor)
};