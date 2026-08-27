// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <array>

namespace bassman
{
class VolumeBrightFilter
{
public:
    void prepare(double newSampleRate) noexcept;
    void reset(double steadyInput = 0.0) noexcept;
    void setVolume(double newVolume) noexcept;
    void setBright(bool shouldBeBright) noexcept { bright = shouldBeBright; }
    [[nodiscard]] float processSample(float input) noexcept;

private:
    void updateCoefficients() noexcept;

    double sampleRate = 44100.0;
    double volume = 0.5;
    bool bright = true;

    std::array<double, 2> normalB {};
    double normalA1 = 0.0;
    double normalX1 = 0.0;
    double normalY1 = 0.0;

    std::array<double, 3> brightB {};
    std::array<double, 2> brightA {};
    std::array<double, 2> brightX {};
    std::array<double, 2> brightY {};
};
} // namespace bassman
