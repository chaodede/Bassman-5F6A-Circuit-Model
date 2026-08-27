// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <array>

namespace bassman
{
class TriodeModel
{
public:
    struct Evaluation
    {
        std::array<double, 2> currents {};
        std::array<std::array<double, 2>, 2> jacobian {};
    };

    [[nodiscard]] Evaluation evaluate(double gridToCathode,
                                      double plateToCathode) const noexcept;

private:
    static double softplus(double value) noexcept;
    static double logistic(double value) noexcept;

    static constexpr double gridGain = 606.0e-6;
    static constexpr double gridExponent = 1.354;
    static constexpr double gridSoftness = 13.9;
    static constexpr double plateGain = 2.14e-3;
    static constexpr double plateExponent = 1.303;
    static constexpr double plateSoftness = 3.04;
    static constexpr double amplificationFactor = 100.8;
};
} // namespace bassman
