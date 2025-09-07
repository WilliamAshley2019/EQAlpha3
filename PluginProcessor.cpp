#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Api550bAudioProcessor();
}

float Api550bAudioProcessor::Impl::saturationDrive = 2.0f;

Api550bAudioProcessor::Api550bAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo())
        .withOutput("Output", juce::AudioChannelSet::stereo())),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    apvts.addParameterListener(Params::LOW_FREQ, this);
    apvts.addParameterListener(Params::LOW_GAIN, this);
    apvts.addParameterListener(Params::LOW_SHELF, this);
    apvts.addParameterListener(Params::LOW_MUTE, this); // New
    apvts.addParameterListener(Params::LOW_BYPASS, this); // New
    apvts.addParameterListener(Params::LM_FREQ, this);
    apvts.addParameterListener(Params::LM_GAIN, this);
    apvts.addParameterListener(Params::LM_MUTE, this); // New
    apvts.addParameterListener(Params::LM_BYPASS, this); // New
    apvts.addParameterListener(Params::HM_FREQ, this);
    apvts.addParameterListener(Params::HM_GAIN, this);
    apvts.addParameterListener(Params::HM_MUTE, this); // New
    apvts.addParameterListener(Params::HM_BYPASS, this); // New
    apvts.addParameterListener(Params::HIGH_FREQ, this);
    apvts.addParameterListener(Params::HIGH_GAIN, this);
    apvts.addParameterListener(Params::HIGH_SHELF, this);
    apvts.addParameterListener(Params::HIGH_MUTE, this); // New
    apvts.addParameterListener(Params::HIGH_BYPASS, this); // New
    apvts.addParameterListener(Params::SAT_DRIVE, this);
    apvts.addParameterListener(Params::Q_MODE, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout Api550bAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::StringArray lowFreqChoices{ "40", "75", "150", "300", "600", "1.2k", "2.4k" };
    juce::StringArray highFreqChoices{ "800", "1.5k", "3k", "5k", "7k", "10k", "12.5k" };
    juce::StringArray gainChoices{ "-12", "-9", "-6", "-3", "0", "3", "6", "9", "12" };

    auto addBand = [&](const juce::String& prefix, const juce::String& name, const juce::StringArray& freqs, int defaultFreqIndex) {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "_FREQ", name + " Freq", freqs, defaultFreqIndex));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "_GAIN", name + " Gain", gainChoices, 4));
        params.push_back(std::make_unique<juce::AudioParameterBool>(prefix + "_MUTE", name + " Mute", false)); // New: Mute, default off (unmuted)
        params.push_back(std::make_unique<juce::AudioParameterBool>(prefix + "_BYPASS", name + " Bypass", false)); // New: Bypass, default off (not bypassed)
        };

    addBand("LOW", "Low", lowFreqChoices, 3); // Default 300 Hz
    params.push_back(std::make_unique<juce::AudioParameterBool>(Params::LOW_SHELF, "Low Shelf", false));
    addBand("LM", "Low Mid", lowFreqChoices, 4); // Default 600 Hz
    addBand("HM", "High Mid", highFreqChoices, 2); // Default 3k Hz
    addBand("HIGH", "High", highFreqChoices, 3); // Default 5k Hz
    params.push_back(std::make_unique<juce::AudioParameterBool>(Params::HIGH_SHELF, "High Shelf", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(Params::SAT_DRIVE, "Saturation Drive", 0.0f, 10.0f, 2.0f)); // Updated: 0.0–10.0
    params.push_back(std::make_unique<juce::AudioParameterBool>(Params::Q_MODE, "Proportional Q", false)); // False = Fixed Q

    return { params.begin(), params.end() };
}

void Api550bAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec{ sampleRate, static_cast<juce::uint32>(samplesPerBlock), static_cast<juce::uint32>(getTotalNumInputChannels()) };
    lowFilter.prepare(spec);
    lowMidFilter.prepare(spec);
    highMidFilter.prepare(spec);
    highFilter.prepare(spec);
    saturation.prepare(spec);

    updateFilters(); // Initial update to set saturation and filters
}

void Api550bAudioProcessor::parameterChanged(const juce::String&, float)
{
    parametersChanged = true;
}

void Api550bAudioProcessor::updateFilters()
{
    parametersChanged = false;
    double sampleRate = getSampleRate();

    static constexpr float lowFreqValues[] = { 40.f, 75.f, 150.f, 300.f, 600.f, 1200.f, 2400.f };
    static constexpr float highFreqValues[] = { 800.f, 1500.f, 3000.f, 5000.f, 7000.f, 10000.f, 12500.f };
    static constexpr float gainDbValues[] = { -12.f, -9.f, -6.f, -3.f, 0.f, 3.f, 6.f, 9.f, 12.f };

    auto getParamValue = [&](const juce::String& paramID, const float values[], size_t size) {
        auto index = static_cast<size_t>(apvts.getRawParameterValue(paramID)->load());
        return values[juce::jmin(index, size - 1)];
        };

    bool useProportionalQ = apvts.getRawParameterValue(Params::Q_MODE)->load() > 0.5f;
    constexpr float fixedQ = 1.5f;

    // Update saturation
    Impl::saturationDrive = apvts.getRawParameterValue(Params::SAT_DRIVE)->load();
    saturation.functionToUse = &Impl::applySaturation;

    // Low Band
    bool lowMute = apvts.getRawParameterValue(Params::LOW_MUTE)->load() > 0.5f;
    bool lowBypass = apvts.getRawParameterValue(Params::LOW_BYPASS)->load() > 0.5f;
    float lowFreq = getParamValue(Params::LOW_FREQ, lowFreqValues, std::size(lowFreqValues));
    float lowGain = lowMute ? 0.0f : getParamValue(Params::LOW_GAIN, gainDbValues, std::size(gainDbValues));
    float lowQ = useProportionalQ ? juce::jlimit(0.7f, 2.2f, 1.0f + 0.2f * std::abs(lowGain)) : fixedQ;
    bool lowShelf = apvts.getRawParameterValue(Params::LOW_SHELF)->load() > 0.5f;
    lowFilter.coefficients = lowBypass ? juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, lowFreq)
        : lowShelf ? juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, lowFreq, lowQ, juce::Decibels::decibelsToGain(lowGain))
        : juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, lowFreq, lowQ, juce::Decibels::decibelsToGain(lowGain));

    // Low-Mid Band
    bool lmMute = apvts.getRawParameterValue(Params::LM_MUTE)->load() > 0.5f;
    bool lmBypass = apvts.getRawParameterValue(Params::LM_BYPASS)->load() > 0.5f;
    float lmFreq = getParamValue(Params::LM_FREQ, lowFreqValues, std::size(lowFreqValues));
    float lmGain = lmMute ? 0.0f : getParamValue(Params::LM_GAIN, gainDbValues, std::size(gainDbValues));
    float lmQ = useProportionalQ ? juce::jlimit(0.7f, 2.2f, 1.0f + 0.2f * std::abs(lmGain)) : fixedQ;
    lowMidFilter.coefficients = lmBypass ? juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, lmFreq)
        : juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, lmFreq, lmQ, juce::Decibels::decibelsToGain(lmGain));

    // High-Mid Band
    bool hmMute = apvts.getRawParameterValue(Params::HM_MUTE)->load() > 0.5f;
    bool hmBypass = apvts.getRawParameterValue(Params::HM_BYPASS)->load() > 0.5f;
    float hmFreq = getParamValue(Params::HM_FREQ, highFreqValues, std::size(highFreqValues));
    float hmGain = hmMute ? 0.0f : getParamValue(Params::HM_GAIN, gainDbValues, std::size(gainDbValues));
    float hmQ = useProportionalQ ? juce::jlimit(0.7f, 2.2f, 1.0f + 0.2f * std::abs(hmGain)) : fixedQ;
    highMidFilter.coefficients = hmBypass ? juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, hmFreq)
        : juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, hmFreq, hmQ, juce::Decibels::decibelsToGain(hmGain));

    // High Band
    bool highMute = apvts.getRawParameterValue(Params::HIGH_MUTE)->load() > 0.5f;
    bool highBypass = apvts.getRawParameterValue(Params::HIGH_BYPASS)->load() > 0.5f;
    float highFreq = getParamValue(Params::HIGH_FREQ, highFreqValues, std::size(highFreqValues));
    float highGain = highMute ? 0.0f : getParamValue(Params::HIGH_GAIN, gainDbValues, std::size(gainDbValues));
    float highQ = useProportionalQ ? juce::jlimit(0.7f, 2.2f, 1.0f + 0.2f * std::abs(highGain)) : fixedQ;
    bool highShelf = apvts.getRawParameterValue(Params::HIGH_SHELF)->load() > 0.5f;
    highFilter.coefficients = highBypass ? juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, highFreq)
        : highShelf ? juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, highFreq * 1.3f, highQ, juce::Decibels::decibelsToGain(highGain))
        : juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, highFreq, highQ, juce::Decibels::decibelsToGain(highGain));
}

void Api550bAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumChannels() == 0 || getTotalNumInputChannels() != getTotalNumOutputChannels())
        return;

    if (parametersChanged)
        updateFilters();

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    if (!(apvts.getRawParameterValue(Params::LOW_BYPASS)->load() > 0.5f))
        lowFilter.process(context);
    if (!(apvts.getRawParameterValue(Params::LM_BYPASS)->load() > 0.5f))
        lowMidFilter.process(context);
    if (!(apvts.getRawParameterValue(Params::HM_BYPASS)->load() > 0.5f))
        highMidFilter.process(context);
    if (!(apvts.getRawParameterValue(Params::HIGH_BYPASS)->load() > 0.5f))
        highFilter.process(context);
    saturation.process(context);
}

void Api550bAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void Api550bAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorEditor* Api550bAudioProcessor::createEditor()
{
    return new Api550bAudioProcessorEditor(*this);
}