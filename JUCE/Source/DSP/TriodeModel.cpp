// SPDX-License-Identifier: AGPL-3.0-or-later
#include "TriodeModel.h"

#include <cmath>

namespace bassman
{
double TriodeModel::softplus(double value) noexcept
{
    if (value > 50.0)
        return value;
    if (value < -50.0)
        return std::exp(value);
    return std::log1p(std::exp(value));
}

double TriodeModel::logistic(double value) noexcept
{
    if (value >= 0.0)
        return 1.0 / (1.0 + std::exp(-value));
    const auto exponential = std::exp(value);
    return exponential / (1.0 + exponential);
}

TriodeModel::Evaluation TriodeModel::evaluate(double gridToCathode,
                                              double plateToCathode) const noexcept
{
    const auto gridArgument = gridSoftness * gridToCathode;
    const auto gridBase = softplus(gridArgument) / gridSoftness;
    const auto gridCurrent = -gridGain * std::pow(gridBase, gridExponent);
    const auto dGridCurrentByGrid = -gridGain * gridExponent
        * std::pow(gridBase, gridExponent - 1.0) * logistic(gridArgument);

    const auto plateArgument = plateSoftness
        * (plateToCathode / amplificationFactor + gridToCathode);
    const auto plateBase = softplus(plateArgument) / plateSoftness;
    const auto cathodeCurrent = -plateGain * std::pow(plateBase, plateExponent);
    const auto dCathodeCurrentByGrid = -plateGain * plateExponent
        * std::pow(plateBase, plateExponent - 1.0) * logistic(plateArgument);
    const auto dCathodeCurrentByPlate = dCathodeCurrentByGrid / amplificationFactor;

    Evaluation result;
    result.currents[0] = gridCurrent;
    result.currents[1] = cathodeCurrent - gridCurrent;
    result.jacobian[0][0] = dGridCurrentByGrid;
    result.jacobian[0][1] = 0.0;
    result.jacobian[1][0] = dCathodeCurrentByGrid - dGridCurrentByGrid;
    result.jacobian[1][1] = dCathodeCurrentByPlate;
    return result;
}
} // namespace bassman
