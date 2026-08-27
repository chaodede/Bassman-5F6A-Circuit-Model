// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <memory>

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

struct BassmanResearchAudioProcessor::ProcessingState
{
    ProcessingState(std::size_t channels,
                    int requestedMaximumBlockSize,
                    double sampleRate,
                    int oversamplingStageCount)
        : oversampling(channels, static_cast<std::size_t>(oversamplingStageCount),
                       juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                       true, true),
          maximumBlockSize(std::max(1, requestedMaximumBlockSize)),
          channelCount(std::min(channels, circuits.size()))
    {
        oversampling.initProcessing(static_cast<std::size_t>(maximumBlockSize));
        oversampling.reset();

        const auto factor = static_cast<int>(oversampling.getOversamplingFactor());
        dry.setSize(static_cast<int>(channelCount), maximumBlockSize * factor,
                    false, false, true);

        const auto circuitSampleRate = sampleRate * static_cast<double>(factor);
        ready = true;
        for (std::size_t channel = 0; channel < channelCount; ++channel)
            ready = circuits[channel].prepare(circuitSampleRate) && ready;
    }

    std::array<bassman::AmpCircuit, 2> circuits;
    juce::dsp::Oversampling<float> oversampling;
    juce::AudioBuffer<float> dry;
    int maximumBlockSize;
    std::size_t channelCount;
    bool ready = false;
};

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
    const auto channels = static_cast<std::size_t>(getTotalNumOutputChannels());
    auto newState = std::make_shared<ProcessingState>(
        channels, maximumExpectedSamplesPerBlock, sampleRate, oversamplingStages);

    if (!newState->ready)
    {
        std::atomic_store_explicit(&processingState, std::shared_ptr<ProcessingState> {},
                                   std::memory_order_release);
        setLatencySamples(0);
        return;
    }

    setLatencySamples(static_cast<int>(
        std::lround(newState->oversampling.getLatencyInSamples())));
    std::atomic_store_explicit(&processingState, std::move(newState),
                               std::memory_order_release);
}

void BassmanResearchAudioProcessor::releaseResources()
{
    // A host may finish an offline render while its worker is returning from
    // processBlock. The worker keeps its own shared snapshot alive until the
    // current block has completed.
    std::atomic_store_explicit(&processingState, std::shared_ptr<ProcessingState> {},
                               std::memory_order_release);
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

    auto state = std::atomic_load_explicit(&processingState, std::memory_order_acquire);
    if (state == nullptr)
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
        static_cast<std::size_t>(buffer.getNumChannels()), state->channelCount);
    for (std::size_t channel = 0; channel < channelCount; ++channel)
    {
        state->circuits[channel].setBright(bright);
        state->circuits[channel].setVolume(volume);
        state->circuits[channel].setToneControls(treble, bass, middle);
    }

    // Some offline hosts provide blocks larger than the maximum reported to
    // prepareToPlay. Process them as bounded chunks instead of clearing the
    // block or overrunning JUCE's oversampling buffers.
    const auto totalSamples = buffer.getNumSamples();
    for (int offset = 0; offset < totalSamples; offset += state->maximumBlockSize)
    {
        const auto blockSamples = std::min(state->maximumBlockSize, totalSamples - offset);
        auto baseBlock = juce::dsp::AudioBlock<float>(buffer).getSubBlock(
            static_cast<std::size_t>(offset), static_cast<std::size_t>(blockSamples));
        auto upsampled = state->oversampling.processSamplesUp(baseBlock);
        const auto upsampledSamples = static_cast<int>(upsampled.getNumSamples());

        for (std::size_t channel = 0; channel < channelCount; ++channel)
        {
            auto* wet = upsampled.getChannelPointer(channel);
            auto* dry = state->dry.getWritePointer(static_cast<int>(channel));
            std::copy_n(wet, upsampledSamples, dry);

            for (int sample = 0; sample < upsampledSamples; ++sample)
            {
                const auto processed = state->circuits[channel].processSample(
                    dry[sample] * inputGain);
                wet[sample] = dry[sample] + mix * (processed - dry[sample]);
            }
        }

        state->oversampling.processSamplesDown(baseBlock);
    }

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
