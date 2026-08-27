// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "LinearAlgebra.h"
#include "TriodeModel.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace bassman
{
class NodalDKModel
{
public:
    struct SolverStats
    {
        std::uint64_t samples = 0;
        std::uint64_t failedSamples = 0;
        std::uint64_t totalIterations = 0;
        int lastIterations = 0;
        double lastResidual = 0.0;
        bool lastConverged = true;
    };

    bool prepare(double newSampleRate) noexcept;
    void reset() noexcept;
    bool setToneControls(double treble, double bass, double middle) noexcept;
    [[nodiscard]] float processSample(float input) noexcept;

    [[nodiscard]] const SolverStats& getSolverStats() const noexcept { return stats; }

private:
    static constexpr std::size_t resistorCount = 10;
    static constexpr std::size_t capacitorCount = 3;
    static constexpr std::size_t inputCount = 2;
    static constexpr std::size_t outputCount = 1;
    static constexpr std::size_t nonlinearCount = 4;
    static constexpr std::size_t nodeCount = 12;
    static constexpr std::size_t extendedCount = nodeCount + inputCount;

    using StateVector = math::Vector<capacitorCount>;
    using InputVector = math::Vector<inputCount>;
    using NonlinearVector = math::Vector<nonlinearCount>;

    struct Matrices
    {
        math::Matrix<capacitorCount, capacitorCount> A {};
        math::Matrix<capacitorCount, inputCount> B {};
        math::Matrix<capacitorCount, nonlinearCount> C {};
        math::Matrix<outputCount, capacitorCount> D {};
        math::Matrix<outputCount, inputCount> E {};
        math::Matrix<outputCount, nonlinearCount> F {};
        math::Matrix<nonlinearCount, capacitorCount> G {};
        math::Matrix<nonlinearCount, inputCount> H {};
        math::Matrix<nonlinearCount, nonlinearCount> K {};
    };

    struct Nonlinearity
    {
        NonlinearVector currents {};
        math::Matrix<nonlinearCount, nonlinearCount> jacobian {};
    };

    bool buildMatrices(Matrices& result) const noexcept;
    bool initialiseSteadyState() noexcept;
    bool solveNonlinear(const NonlinearVector& source,
                        const math::Matrix<nonlinearCount, nonlinearCount>& coupling,
                        NonlinearVector& currents) noexcept;
    [[nodiscard]] Nonlinearity evaluateNonlinearity(const NonlinearVector& voltage) const noexcept;
    [[nodiscard]] static NonlinearVector residual(
        const NonlinearVector& source,
        const math::Matrix<nonlinearCount, nonlinearCount>& coupling,
        const NonlinearVector& voltage,
        const NonlinearVector& currents) noexcept;

    double sampleRate = 44100.0;
    double trebleControl = 0.5;
    double bassControl = 0.5;
    double middleControl = 0.5;
    Matrices matrices;
    StateVector state {};
    InputVector input { 0.0, 325.0 };
    NonlinearVector nonlinearVoltage {};
    TriodeModel triode;
    SolverStats stats;
};
} // namespace bassman
