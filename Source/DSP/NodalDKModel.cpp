// SPDX-License-Identifier: AGPL-3.0-or-later
#include "NodalDKModel.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace bassman
{
namespace
{
struct Branch
{
    int positiveNode;
    int negativeNode;
    double value;
};

template <std::size_t Branches, std::size_t Nodes>
void stampIncidence(math::Matrix<Branches, Nodes>& incidence,
                    std::size_t branch,
                    int positiveNode,
                    int negativeNode) noexcept
{
    if (positiveNode > 0)
        incidence[branch][static_cast<std::size_t>(positiveNode - 1)] = 1.0;
    if (negativeNode > 0)
        incidence[branch][static_cast<std::size_t>(negativeNode - 1)] = -1.0;
}
} // namespace

bool NodalDKModel::prepare(double newSampleRate) noexcept
{
    sampleRate = std::max(1000.0, newSampleRate);
    Matrices candidate;
    if (!buildMatrices(candidate))
        return false;
    matrices = candidate;
    state = {};
    nonlinearVoltage = {};
    stats = {};
    initialiseSteadyState();
    return true;
}

void NodalDKModel::reset() noexcept
{
    state = {};
    nonlinearVoltage = {};
    stats = {};
    initialiseSteadyState();
}

bool NodalDKModel::setToneControls(double treble, double bass, double middle) noexcept
{
    const auto newTreble = std::clamp(treble, 0.01, 0.99);
    const auto newBass = std::clamp(bass, 0.01, 0.99);
    const auto newMiddle = std::clamp(middle, 0.01, 0.99);

    if (newTreble == trebleControl && newBass == bassControl && newMiddle == middleControl)
        return true;

    const auto oldTreble = trebleControl;
    const auto oldBass = bassControl;
    const auto oldMiddle = middleControl;
    trebleControl = newTreble;
    bassControl = newBass;
    middleControl = newMiddle;

    Matrices candidate;
    if (!buildMatrices(candidate))
    {
        trebleControl = oldTreble;
        bassControl = oldBass;
        middleControl = oldMiddle;
        return false;
    }

    matrices = candidate;
    return true;
}

bool NodalDKModel::buildMatrices(Matrices& result) const noexcept
{
    const std::array<Branch, resistorCount> resistors {{
        { 1, 2, 270.0e3 },
        { 3, 0, 820.0 },
        { 5, 4, 100.0e3 },
        { 6, 0, 100.0e3 },
        { 6, 7, 56.0e3 },
        { 8, 9, 250.0e3 * (1.0 - trebleControl) },
        { 9, 10, 250.0e3 * trebleControl },
        { 10, 11, 500.0e3 * bassControl },
        { 11, 12, 25.0e3 * (1.0 - middleControl) },
        { 12, 0, 25.0e3 * middleControl }
    }};
    const std::array<Branch, capacitorCount> capacitors {{
        { 6, 8, 250.0e-12 },
        { 7, 10, 20.0e-9 },
        { 7, 12, 20.0e-9 }
    }};

    math::Matrix<resistorCount, nodeCount> resistorIncidence {};
    math::Matrix<resistorCount, resistorCount> resistorConductance {};
    for (std::size_t branch = 0; branch < resistorCount; ++branch)
    {
        stampIncidence(resistorIncidence, branch,
                       resistors[branch].positiveNode, resistors[branch].negativeNode);
        resistorConductance[branch][branch] = 1.0 / resistors[branch].value;
    }

    math::Matrix<capacitorCount, nodeCount> capacitorIncidence {};
    math::Matrix<capacitorCount, capacitorCount> capacitorConductance {};
    const auto timeStep = 1.0 / sampleRate;
    for (std::size_t branch = 0; branch < capacitorCount; ++branch)
    {
        stampIncidence(capacitorIncidence, branch,
                       capacitors[branch].positiveNode, capacitors[branch].negativeNode);
        capacitorConductance[branch][branch] = 2.0 * capacitors[branch].value / timeStep;
    }

    math::Matrix<inputCount, nodeCount> inputIncidence {};
    stampIncidence(inputIncidence, 0, 1, 0);
    stampIncidence(inputIncidence, 1, 5, 0);

    math::Matrix<outputCount, nodeCount> outputIncidence {};
    stampIncidence(outputIncidence, 0, 9, 0);

    math::Matrix<nonlinearCount, nodeCount> nonlinearIncidence {};
    stampIncidence(nonlinearIncidence, 0, 2, 3);
    stampIncidence(nonlinearIncidence, 1, 4, 3);
    stampIncidence(nonlinearIncidence, 2, 4, 6);
    stampIncidence(nonlinearIncidence, 3, 5, 6);

    const auto resistorNodal = math::multiply(
        math::multiply(math::transpose(resistorIncidence), resistorConductance),
        resistorIncidence);
    const auto capacitorNodal = math::multiply(
        math::multiply(math::transpose(capacitorIncidence), capacitorConductance),
        capacitorIncidence);
    const auto nodalConductance = math::add(resistorNodal, capacitorNodal);

    math::Matrix<extendedCount, extendedCount> system {};
    for (std::size_t row = 0; row < nodeCount; ++row)
    {
        for (std::size_t col = 0; col < nodeCount; ++col)
            system[row][col] = nodalConductance[row][col];
        for (std::size_t source = 0; source < inputCount; ++source)
        {
            system[row][nodeCount + source] = inputIncidence[source][row];
            system[nodeCount + source][row] = inputIncidence[source][row];
        }
    }

    math::Matrix<extendedCount, extendedCount> inverseSystem {};
    if (!math::invert(system, inverseSystem))
        return false;

    math::Matrix<capacitorCount, extendedCount> extendedCapacitorIncidence {};
    math::Matrix<outputCount, extendedCount> extendedOutputIncidence {};
    math::Matrix<nonlinearCount, extendedCount> extendedNonlinearIncidence {};
    for (std::size_t col = 0; col < nodeCount; ++col)
    {
        for (std::size_t row = 0; row < capacitorCount; ++row)
            extendedCapacitorIncidence[row][col] = capacitorIncidence[row][col];
        for (std::size_t row = 0; row < outputCount; ++row)
            extendedOutputIncidence[row][col] = outputIncidence[row][col];
        for (std::size_t row = 0; row < nonlinearCount; ++row)
            extendedNonlinearIncidence[row][col] = nonlinearIncidence[row][col];
    }

    math::Matrix<extendedCount, inputCount> extendedInputSelector {};
    for (std::size_t source = 0; source < inputCount; ++source)
        extendedInputSelector[nodeCount + source][source] = 1.0;

    const auto capacitorPath = math::scale(
        math::multiply(math::multiply(capacitorConductance, extendedCapacitorIncidence),
                       inverseSystem),
        2.0);
    const auto outputPath = math::multiply(extendedOutputIncidence, inverseSystem);
    const auto nonlinearPath = math::multiply(extendedNonlinearIncidence, inverseSystem);
    const auto capacitorTranspose = math::transpose(extendedCapacitorIncidence);
    const auto nonlinearTranspose = math::transpose(extendedNonlinearIncidence);

    result.A = math::subtract(math::multiply(capacitorPath, capacitorTranspose),
                              math::identity<capacitorCount>());
    result.B = math::multiply(capacitorPath, extendedInputSelector);
    result.C = math::multiply(capacitorPath, nonlinearTranspose);
    result.D = math::multiply(outputPath, capacitorTranspose);
    result.E = math::multiply(outputPath, extendedInputSelector);
    result.F = math::multiply(outputPath, nonlinearTranspose);
    result.G = math::multiply(nonlinearPath, capacitorTranspose);
    result.H = math::multiply(nonlinearPath, extendedInputSelector);
    result.K = math::multiply(nonlinearPath, nonlinearTranspose);
    return true;
}

NodalDKModel::Nonlinearity NodalDKModel::evaluateNonlinearity(
    const NonlinearVector& voltage) const noexcept
{
    Nonlinearity result;
    for (std::size_t tube = 0; tube < 2; ++tube)
    {
        const auto offset = tube * 2;
        const auto evaluation = triode.evaluate(voltage[offset], voltage[offset + 1]);
        result.currents[offset] = evaluation.currents[0];
        result.currents[offset + 1] = evaluation.currents[1];
        result.jacobian[offset][offset] = evaluation.jacobian[0][0];
        result.jacobian[offset][offset + 1] = evaluation.jacobian[0][1];
        result.jacobian[offset + 1][offset] = evaluation.jacobian[1][0];
        result.jacobian[offset + 1][offset + 1] = evaluation.jacobian[1][1];
    }
    return result;
}

NodalDKModel::NonlinearVector NodalDKModel::residual(
    const NonlinearVector& source,
    const math::Matrix<nonlinearCount, nonlinearCount>& coupling,
    const NonlinearVector& voltage,
    const NonlinearVector& currents) noexcept
{
    auto result = math::multiply(coupling, currents);
    for (std::size_t i = 0; i < nonlinearCount; ++i)
        result[i] += source[i] - voltage[i];
    return result;
}

bool NodalDKModel::solveNonlinear(
    const NonlinearVector& source,
    const math::Matrix<nonlinearCount, nonlinearCount>& coupling,
    NonlinearVector& currents) noexcept
{
    constexpr int maximumIterations = 20;
    constexpr int maximumLineSearchSteps = 10;
    constexpr double residualTolerance = 1.0e-7;
    constexpr double relaxedResidualTolerance = 1.0e-5;
    constexpr double stepTolerance = 1.0e-7;

    auto voltage = nonlinearVoltage;
    auto evaluation = evaluateNonlinearity(voltage);
    auto error = residual(source, coupling, voltage, evaluation.currents);
    auto errorNorm = math::maxAbs(error);
    bool converged = false;
    int iterations = 0;

    for (; iterations < maximumIterations; ++iterations)
    {
        if (!std::isfinite(errorNorm))
            break;
        if (errorNorm <= residualTolerance)
        {
            converged = true;
            break;
        }

        auto jacobian = math::multiply(coupling, evaluation.jacobian);
        for (std::size_t i = 0; i < nonlinearCount; ++i)
            jacobian[i][i] -= 1.0;

        NonlinearVector newtonStep {};
        if (!math::solve(jacobian, error, newtonStep))
            break;

        auto accepted = false;
        auto damping = 1.0;
        NonlinearVector candidateVoltage {};
        Nonlinearity candidateEvaluation;
        NonlinearVector candidateError {};
        double candidateNorm = errorNorm;
        for (int lineSearch = 0; lineSearch < maximumLineSearchSteps; ++lineSearch)
        {
            for (std::size_t i = 0; i < nonlinearCount; ++i)
                candidateVoltage[i] = voltage[i] - damping * newtonStep[i];
            candidateEvaluation = evaluateNonlinearity(candidateVoltage);
            candidateError = residual(source, coupling, candidateVoltage,
                                      candidateEvaluation.currents);
            candidateNorm = math::maxAbs(candidateError);
            if (std::isfinite(candidateNorm) && candidateNorm < errorNorm)
            {
                accepted = true;
                break;
            }
            damping *= 0.5;
        }

        if (!accepted)
            break;

        voltage = candidateVoltage;
        evaluation = candidateEvaluation;
        error = candidateError;
        errorNorm = candidateNorm;

        if (damping * math::maxAbs(newtonStep) <= stepTolerance
            && errorNorm <= relaxedResidualTolerance)
        {
            converged = true;
            ++iterations;
            break;
        }
    }

    if (!converged && std::isfinite(errorNorm) && errorNorm <= relaxedResidualTolerance)
        converged = true;

    if (std::isfinite(errorNorm))
    {
        nonlinearVoltage = voltage;
        currents = evaluation.currents;
    }
    else
    {
        currents = evaluateNonlinearity(nonlinearVoltage).currents;
    }

    stats.lastIterations = iterations;
    stats.lastResidual = errorNorm;
    stats.lastConverged = converged;
    stats.totalIterations += static_cast<std::uint64_t>(iterations);
    return converged;
}

bool NodalDKModel::initialiseSteadyState() noexcept
{
    auto steadyMatrix = math::subtract(math::identity<capacitorCount>(), matrices.A);
    math::Matrix<capacitorCount, capacitorCount> inverseSteadyMatrix {};
    if (!math::invert(steadyMatrix, inverseSteadyMatrix))
        return false;

    const auto stateFromCurrents = math::multiply(inverseSteadyMatrix, matrices.C);
    const auto stateFromInputs = math::multiply(inverseSteadyMatrix, matrices.B);
    const auto steadyCoupling = math::add(
        matrices.K, math::multiply(matrices.G, stateFromCurrents));
    const auto steadyInputPath = math::add(
        matrices.H, math::multiply(matrices.G, stateFromInputs));
    const auto source = math::multiply(steadyInputPath, input);

    nonlinearVoltage = source;
    NonlinearVector currents {};
    const auto converged = solveNonlinear(source, steadyCoupling, currents);
    auto nextState = math::multiply(stateFromInputs, input);
    const auto currentContribution = math::multiply(stateFromCurrents, currents);
    for (std::size_t i = 0; i < capacitorCount; ++i)
        nextState[i] += currentContribution[i];
    state = nextState;
    return converged;
}

float NodalDKModel::processSample(float signal) noexcept
{
    input[0] = static_cast<double>(signal);

    auto source = math::multiply(matrices.G, state);
    const auto inputSource = math::multiply(matrices.H, input);
    for (std::size_t i = 0; i < nonlinearCount; ++i)
        source[i] += inputSource[i];

    NonlinearVector currents {};
    const auto converged = solveNonlinear(source, matrices.K, currents);
    ++stats.samples;
    if (!converged)
        ++stats.failedSamples;

    auto output = math::multiply(matrices.D, state)[0]
        + math::multiply(matrices.E, input)[0]
        + math::multiply(matrices.F, currents)[0];

    auto nextState = math::multiply(matrices.A, state);
    const auto inputContribution = math::multiply(matrices.B, input);
    const auto currentContribution = math::multiply(matrices.C, currents);
    for (std::size_t i = 0; i < capacitorCount; ++i)
        nextState[i] += inputContribution[i] + currentContribution[i];

    if (!std::isfinite(output))
    {
        reset();
        return 0.0f;
    }

    state = nextState;
    return static_cast<float>(output / 100.0);
}
} // namespace bassman
