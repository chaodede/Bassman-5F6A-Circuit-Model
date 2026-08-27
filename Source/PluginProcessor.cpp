// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>

namespace parameterId
{
constexpr auto input = "input";
constexpr auto bright = "bright";
constexpr auto volume = "volume";
constexpr auto treble = "treble";
constexpr auto bass = "bass";
constexpr auto middle = "middle";
constexpr auto mix = "mix";
constexpr auto output = "output";
} // namespace parameterId

BassmanResearchAudioProcessor::BassmanResearchAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
BassmanResearchAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(parameterId::input, 1), "Input",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(parameterId::bright, 1), "Bright", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(parameterId::volume, 1), "Volume",
        juce::NormalisableRange<float>(0.01f, 0.99f, 0.001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(parameterId::treble, 1), "Treble",
        juce::NormalisableRange<float>(0.01f, 0.99f, 0.001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(parameterId::bass, 1), "Bass",
        juce::NormalisableRange<float>(0.01f, 0.99f, 0.001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(parameterId::middle, 1), "Middle",
        juce::NormalisableRange<float>(0.01f, 0.99f, 0.001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(parameterId::mix, 1), "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(parameterId::output, 1), "Output",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    return layout;
}

void BassmanResearchAudioProcessor::prepareToPlay(double sampleRate,
                                                  int maximumExpectedSamplesPerBlock)
{
    preparedMaximumBlockSize = std::max(1, maximumExpectedSamplesPerBlock);
    const auto channels = static_cast<std::size_t>(getTotalNumOutputChannels());
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        channels, oversamplingStages,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true, true);
    oversampling->initProcessing(static_cast<std::size_t>(preparedMaximumBlockSize));
    oversampling->reset();
    setLatencySamples(static_cast<int>(std::lround(oversampling->getLatencyInSamples())));

    const auto factor = static_cast<int>(oversampling->getOversamplingFactor());
    oversampledDry.setSize(static_cast<int>(channels),
                           preparedMaximumBlockSize * factor,
                           false, false, true);

    const auto circuitSampleRate = sampleRate * static_cast<double>(factor);
    for (std::size_t channel = 0; channel < channels && channel < circuits.size(); ++channel)
        circuits[channel].prepare(circuitSampleRate);
}

void BassmanResearchAudioProcessor::releaseResources()
{
    oversampling.reset();
    oversampledDry.setSize(0, 0);
    preparedMaximumBlockSize = 0;
}

bool BassmanResearchAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    if (output != juce::AudioChannelSet::mono() && output != juce::AudioChannelSet::stereo())
        return false;
    return output == layouts.getMainInputChannelSet();
}

void BassmanResearchAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto inputChannels = getTotalNumInputChannels();
    const auto outputChannels = getTotalNumOutputChannels();
    for (auto channel = inputChannels; channel < outputChannels; ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    if (oversampling == nullptr || buffer.getNumSamples() > preparedMaximumBlockSize)
    {
        buffer.clear();
        return;
    }

    const auto bright = parameters.getRawParameterValue(parameterId::bright)->load() >= 0.5f;
    const auto volume = parameters.getRawParameterValue(parameterId::volume)->load();
    const auto treble = parameters.getRawParameterValue(parameterId::treble)->load();
    const auto bass = parameters.getRawParameterValue(parameterId::bass)->load();
    const auto middle = parameters.getRawParameterValue(parameterId::middle)->load();
    const auto mix = parameters.getRawParameterValue(parameterId::mix)->load();
    const auto inputGain = juce::Decibels::decibelsToGain(
        parameters.getRawParameterValue(parameterId::input)->load());
    const auto outputGain = juce::Decibels::decibelsToGain(
        parameters.getRawParameterValue(parameterId::output)->load());

    const auto channelCount = std::min<std::size_t>(
        static_cast<std::size_t>(buffer.getNumChannels()), circuits.size());
    for (std::size_t channel = 0; channel < channelCount; ++channel)
    {
        circuits[channel].setBright(bright);
        circuits[channel].setVolume(volume);
        circuits[channel].setToneControls(treble, bass, middle);
    }

    juce::dsp::AudioBlock<float> baseBlock(buffer);
    auto upsampled = oversampling->processSamplesUp(baseBlock);
    const auto upsampledSamples = static_cast<int>(upsampled.getNumSamples());

    for (std::size_t channel = 0; channel < channelCount; ++channel)
    {
        auto* wet = upsampled.getChannelPointer(channel);
        auto* dry = oversampledDry.getWritePointer(static_cast<int>(channel));
        std::copy_n(wet, upsampledSamples, dry);

        for (int sample = 0; sample < upsampledSamples; ++sample)
        {
            const auto processed = circuits[channel].processSample(dry[sample] * inputGain);
            wet[sample] = dry[sample] + mix * (processed - dry[sample]);
        }
    }

    oversampling->processSamplesDown(baseBlock);
    buffer.applyGain(outputGain);
}

juce::AudioProcessorEditor* BassmanResearchAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void BassmanResearchAudioProcessor::getStateInformation(juce::MemoryBlock& destinationData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destinationData);
}

void BassmanResearchAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassmanResearchAudioProcessor();
}
