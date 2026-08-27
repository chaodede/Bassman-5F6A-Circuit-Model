// SPDX-License-Identifier: AGPL-3.0-or-later
#include "VolumeBrightFilter.h"

#include <algorithm>

namespace bassman
{
void VolumeBrightFilter::prepare(double newSampleRate) noexcept
{
    sampleRate = std::max(1000.0, newSampleRate);
    updateCoefficients();
    reset();
}

void VolumeBrightFilter::reset(double steadyInput) noexcept
{
    // The coupling capacitor blocks DC. Initialising the input histories to
    // the preceding triode's quiescent plate voltage avoids treating startup
    // as an artificial plate-voltage step.
    normalX1 = steadyInput;
    normalY1 = 0.0;
    brightX = { steadyInput, steadyInput };
    brightY = {};
}

void VolumeBrightFilter::setVolume(double newVolume) noexcept
{
    const auto clamped = std::clamp(newVolume, 0.01, 0.99);
    if (clamped != volume)
    {
        volume = clamped;
        updateCoefficients();
    }
}

void VolumeBrightFilter::updateCoefficients() noexcept
{
    constexpr double potentiometer = 1.0e6;
    constexpr double couplingCapacitor = 20.0e-9;
    constexpr double brightCapacitor = 100.0e-12;
    constexpr double load = 950.0e3;

    const auto left = (1.0 - volume) * potentiometer;
    const auto right = volume * potentiometer;
    const auto c = 2.0 * sampleRate;

    const auto b1 = couplingCapacitor * right * load;
    const auto a0 = right + load;
    const auto a1 = couplingCapacitor * left * right
        + couplingCapacitor * left * load
        + couplingCapacitor * right * load;
    const auto denominator = a0 + a1 * c;
    normalB = { b1 * c / denominator, -b1 * c / denominator };
    normalA1 = (a0 - a1 * c) / denominator;

    const auto brightB1 = couplingCapacitor * right * load;
    const auto brightB2 = couplingCapacitor * brightCapacitor * left * right * load;
    const auto brightA0 = right + load;
    const auto brightA1 = couplingCapacitor * left * right
        + brightCapacitor * left * right
        + couplingCapacitor * left * load
        + brightCapacitor * left * load
        + couplingCapacitor * right * load;
    const auto brightA2 = couplingCapacitor * brightCapacitor * left * right * load;
    const auto cSquared = c * c;
    const auto brightDenominator = brightA0 + brightA1 * c + brightA2 * cSquared;

    brightB = {
        (brightB1 * c + brightB2 * cSquared) / brightDenominator,
        (-2.0 * brightB2 * cSquared) / brightDenominator,
        (brightB2 * cSquared - brightB1 * c) / brightDenominator
    };
    brightA = {
        (2.0 * brightA0 - 2.0 * brightA2 * cSquared) / brightDenominator,
        (brightA0 - brightA1 * c + brightA2 * cSquared) / brightDenominator
    };
}

float VolumeBrightFilter::processSample(float input) noexcept
{
    const auto x = static_cast<double>(input);

    const auto normalOutput = normalB[0] * x + normalB[1] * normalX1
        - normalA1 * normalY1;
    normalX1 = x;
    normalY1 = normalOutput;

    const auto brightOutput = brightB[0] * x + brightB[1] * brightX[0]
        + brightB[2] * brightX[1] - brightA[0] * brightY[0]
        - brightA[1] * brightY[1];
    brightX[1] = brightX[0];
    brightX[0] = x;
    brightY[1] = brightY[0];
    brightY[0] = brightOutput;

    return static_cast<float>(bright ? brightOutput : normalOutput);
}
} // namespace bassman
