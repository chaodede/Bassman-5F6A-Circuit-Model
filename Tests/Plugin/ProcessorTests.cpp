// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace
{
constexpr double pi = 3.14159265358979323846;

void fillSignal(juce::AudioBuffer<float>& buffer, double sampleRate)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            buffer.setSample(channel, sample, static_cast<float>(
                0.08 * std::sin(2.0 * pi * 220.0 * sample / sampleRate)));
}

bool isFiniteAndNonSilent(const juce::AudioBuffer<float>& buffer)
{
    double energy = 0.0;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto value = buffer.getSample(channel, sample);
            if (!std::isfinite(value))
                return false;
            energy += static_cast<double>(value) * value;
        }
    return energy > 1.0e-10;
}
} // namespace

int main()
{
    constexpr auto sampleRate = 48000.0;
    constexpr auto preparedBlockSize = 64;

    BassmanResearchAudioProcessor processor;
    processor.setPlayConfigDetails(2, 2, sampleRate, preparedBlockSize);
    processor.prepareToPlay(sampleRate, preparedBlockSize);

    juce::AudioBuffer<float> startupSilence(2, 4096);
    startupSilence.clear();
    juce::MidiBuffer midi;
    processor.processBlock(startupSilence, midi);
    for (int channel = 0; channel < startupSilence.getNumChannels(); ++channel)
        for (int sample = 0; sample < startupSilence.getNumSamples(); ++sample)
            if (std::abs(startupSilence.getSample(channel, sample)) >= 1.0e-6f)
            {
                std::cerr << "FAIL: zero-input startup contains a transient\n";
                return EXIT_FAILURE;
            }

    // Audition and other offline hosts may send a block larger than the size
    // supplied to prepareToPlay. It must be chunked without silence or overflow.
    juce::AudioBuffer<float> offlineBlock(2, 4096);
    fillSignal(offlineBlock, sampleRate);
    processor.processBlock(offlineBlock, midi);
    if (!isFiniteAndNonSilent(offlineBlock))
    {
        std::cerr << "FAIL: oversized offline block was silent or non-finite\n";
        return EXIT_FAILURE;
    }

    // releaseResources may race with the tail of an offline worker callback.
    // The callback must retain a valid processing-resource snapshot.
    for (int repetition = 0; repetition < 4; ++repetition)
    {
        processor.prepareToPlay(sampleRate, preparedBlockSize);
        juce::AudioBuffer<float> renderBlock(2, 8192);
        fillSignal(renderBlock, sampleRate);
        std::atomic<bool> started { false };
        std::thread worker([&]
        {
            started.store(true, std::memory_order_release);
            processor.processBlock(renderBlock, midi);
        });

        while (!started.load(std::memory_order_acquire))
            std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        processor.releaseResources();
        worker.join();

        for (int channel = 0; channel < renderBlock.getNumChannels(); ++channel)
            for (int sample = 0; sample < renderBlock.getNumSamples(); ++sample)
                if (!std::isfinite(renderBlock.getSample(channel, sample)))
                {
                    std::cerr << "FAIL: release/process race produced non-finite audio\n";
                    return EXIT_FAILURE;
                }
    }

    std::cout << "All processor tests passed\n";
    return EXIT_SUCCESS;
}
