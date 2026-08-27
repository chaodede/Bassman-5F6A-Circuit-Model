// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <JuceHeader.h>

#include "DSP/AmpCircuit.h"

#include <memory>

class BassmanResearchAudioProcessor final : public juce::AudioProcessor
{
public:
    BassmanResearchAudioProcessor();
    ~BassmanResearchAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destinationData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

private:
    struct ProcessingState;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

#if JUCE_DEBUG
    static constexpr int oversamplingStages = 0; // 1x keeps debug builds usable in a DAW.
#else
    static constexpr int oversamplingStages = 2; // 2^2 = 4x in release builds.
#endif

    std::shared_ptr<ProcessingState> processingState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassmanResearchAudioProcessor)
};
