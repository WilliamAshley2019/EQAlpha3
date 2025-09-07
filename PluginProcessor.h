#pragma once
#include <JuceHeader.h>
#include <atomic>

namespace Params
{
    inline const juce::String LOW_FREQ{ "LOW_FREQ" };
    inline const juce::String LOW_GAIN{ "LOW_GAIN" };
    inline const juce::String LOW_SHELF{ "LOW_SHELF" };
    inline const juce::String LOW_MUTE{ "LOW_MUTE" }; // New: Mute for Low band
    inline const juce::String LOW_BYPASS{ "LOW_BYPASS" }; // New: Bypass for Low band
    inline const juce::String LM_FREQ{ "LM_FREQ" };
    inline const juce::String LM_GAIN{ "LM_GAIN" };
    inline const juce::String LM_MUTE{ "LM_MUTE" }; // New: Mute for Low-Mid band
    inline const juce::String LM_BYPASS{ "LM_BYPASS" }; // New: Bypass for Low-Mid band
    inline const juce::String HM_FREQ{ "HM_FREQ" };
    inline const juce::String HM_GAIN{ "HM_GAIN" };
    inline const juce::String HM_MUTE{ "HM_MUTE" }; // New: Mute for High-Mid band
    inline const juce::String HM_BYPASS{ "HM_BYPASS" }; // New: Bypass for High-Mid band
    inline const juce::String HIGH_FREQ{ "HIGH_FREQ" };
    inline const juce::String HIGH_GAIN{ "HIGH_GAIN" };
    inline const juce::String HIGH_SHELF{ "HIGH_SHELF" };
    inline const juce::String HIGH_MUTE{ "HIGH_MUTE" }; // New: Mute for High band
    inline const juce::String HIGH_BYPASS{ "HIGH_BYPASS" }; // New: Bypass for High band
    inline const juce::String SAT_DRIVE{ "SAT_DRIVE" };
    inline const juce::String Q_MODE{ "Q_MODE" };
}

class Api550bAudioProcessor : public juce::AudioProcessor,
    private juce::AudioProcessorValueTreeState::Listener
{
public:
    Api550bAudioProcessor();
    ~Api550bAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "EQAlpha3"; } // Updated to EQAlpha3
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    class Impl
    {
    public:
        static float applySaturation(float x)
        {
            // Avoid division by zero by clamping saturationDrive away from 0
            return saturationDrive > 0.001f ? std::tanh(saturationDrive * x) / saturationDrive : x;
        }

        static float saturationDrive;
    };

    juce::dsp::IIR::Filter<float> lowFilter, lowMidFilter, highMidFilter, highFilter;
    juce::dsp::WaveShaper<float> saturation;

    std::atomic<bool> parametersChanged{ true };
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void updateFilters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Api550bAudioProcessor)
};