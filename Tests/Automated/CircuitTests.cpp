// SPDX-License-Identifier: AGPL-3.0-or-later
#include "DSP/AmpCircuit.h"
#include "DSP/LinearAlgebra.h"
#include "DSP/TriodeModel.h"
#include "DSP/VolumeBrightFilter.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void testTriodeJacobian()
{
    bassman::TriodeModel model;
    constexpr double step = 1.0e-5;
    const double voltages[][2] = {
        { -1.5, 150.0 }, { -0.25, 220.0 }, { 0.2, 80.0 }
    };

    for (const auto& voltage : voltages)
    {
        const auto analytic = model.evaluate(voltage[0], voltage[1]);
        for (int variable = 0; variable < 2; ++variable)
        {
            auto positiveGrid = voltage[0];
            auto positivePlate = voltage[1];
            auto negativeGrid = voltage[0];
            auto negativePlate = voltage[1];
            (variable == 0 ? positiveGrid : positivePlate) += step;
            (variable == 0 ? negativeGrid : negativePlate) -= step;
            const auto positive = model.evaluate(positiveGrid, positivePlate);
            const auto negative = model.evaluate(negativeGrid, negativePlate);
            for (int current = 0; current < 2; ++current)
            {
                const auto numerical = (positive.currents[current] - negative.currents[current])
                    / (2.0 * step);
                const auto expected = analytic.jacobian[current][variable];
                const auto tolerance = 2.0e-6 * std::max(1.0, std::abs(expected));
                expect(std::abs(numerical - expected) <= tolerance,
                       "triode analytic Jacobian must match finite differences");
            }
        }
    }
}

void testColumnPivotedQr()
{
    const bassman::math::Matrix<4, 4> matrix {{
        {{ 1.0e-4, 2.0, -1.0, 0.5 }},
        {{ 2.0e-4, -1.0, 3.0, 1.0 }},
        {{ -1.0e-4, 0.5, 2.0, -2.0 }},
        {{ 3.0e-4, 1.0, 0.25, 4.0 }}
    }};
    const bassman::math::Vector<4> expected {{ 2.0, -1.0, 0.5, 3.0 }};
    const auto rhs = bassman::math::multiply(matrix, expected);

    bassman::math::Vector<4> qrSolution {};
    expect(bassman::math::solveLeastSquaresQr(matrix, rhs, qrSolution),
           "column-pivoted Householder QR must solve a full-rank system");
    for (std::size_t i = 0; i < expected.size(); ++i)
        expect(std::abs(qrSolution[i] - expected[i]) < 1.0e-9,
               "QR solution must recover the known vector");

    const auto reconstructed = bassman::math::multiply(matrix, qrSolution);
    for (std::size_t i = 0; i < rhs.size(); ++i)
        expect(std::abs(reconstructed[i] - rhs[i]) < 1.0e-11,
               "QR solution residual must be near machine precision");

    auto nearRankDeficient = bassman::math::identity<4>();
    nearRankDeficient[3][3] = 1.0e-14;
    const bassman::math::Vector<4> nearRankRhs {{ 1.0, 2.0, 3.0, 4.0e-14 }};
    bassman::math::Vector<4> gaussianSolution {};
    bassman::math::Vector<4> recoveredSolution {};
    expect(!bassman::math::solve(nearRankDeficient, nearRankRhs, gaussianSolution),
           "Gaussian pivot guard must reject the near-rank-deficient test matrix");
    expect(bassman::math::solveLeastSquaresQr(
               nearRankDeficient, nearRankRhs, recoveredSolution),
           "column-pivoted QR must recover a full-rank solve rejected by the fast path");
    expect(std::abs(recoveredSolution[3] - 4.0) < 1.0e-9,
           "QR fallback must recover the small-pivot component");
}

void testVolumeBrightFilter()
{
    bassman::VolumeBrightFilter filter;
    filter.prepare(192000.0);
    filter.setVolume(0.5);
    double energy = 0.0;
    for (int sample = 0; sample < 4096; ++sample)
    {
        const auto output = filter.processSample(sample == 0 ? 1.0f : 0.0f);
        expect(std::isfinite(output), "volume/bright filter output must remain finite");
        energy += static_cast<double>(output) * output;
    }
    expect(energy > 0.0 && energy < 10.0, "volume/bright impulse energy must be bounded");
}

void testAmpCircuit()
{
    bassman::AmpCircuit circuit;
    expect(circuit.prepare(192000.0), "DK matrices must be invertible");
    circuit.setBright(true);
    circuit.setVolume(0.5);
    expect(circuit.setToneControls(0.5, 0.5, 0.5), "tone-stack matrix rebuild must succeed");

    constexpr double pi = 3.14159265358979323846;
    double energy = 0.0;
    for (int sample = 0; sample < 24000; ++sample)
    {
        const auto input = static_cast<float>(0.08 * std::sin(2.0 * pi * 220.0 * sample / 192000.0));
        const auto output = circuit.processSample(input);
        expect(std::isfinite(output), "amp output must remain finite");
        energy += static_cast<double>(output) * output;
    }

    const auto& stats = circuit.getSolverStats();
    expect(energy > 1.0e-8, "amp must produce a non-silent signal");
    expect(stats.samples == 24000, "solver sample counter must be exact");
    expect(stats.failedSamples < 24, "fewer than 0.1% of nominal samples may fail Newton convergence");
    std::cout << "Nominal solve: failed=" << stats.failedSamples
              << ", QR fallbacks=" << stats.qrFallbacks << '\n';
}

void testStartupSteadyState()
{
    bassman::AmpCircuit circuit;
    expect(circuit.prepare(192000.0), "startup circuit must prepare");

    double maximumOutput = 0.0;
    for (int sample = 0; sample < 4096; ++sample)
        maximumOutput = std::max(maximumOutput,
                                 std::abs(static_cast<double>(circuit.processSample(0.0f))));

    const auto& stats = circuit.getSolverStats();
    std::cout << "Startup silence: max=" << maximumOutput
              << ", failed Newton samples=" << stats.failedSamples << '\n';
    expect(maximumOutput < 1.0e-6,
           "zero-input startup must begin at the circuit DC steady state");
    expect(stats.failedSamples == 0,
           "zero-input startup must not contain failed Newton samples");
}

void testToneExtremes()
{
    constexpr double pi = 3.14159265358979323846;
    const double controls[][3] = {
        { 0.01, 0.01, 0.01 },
        { 0.99, 0.99, 0.99 },
        { 0.99, 0.01, 0.5 },
        { 0.01, 0.99, 0.5 }
    };

    for (const auto& control : controls)
    {
        bassman::AmpCircuit circuit;
        expect(circuit.prepare(192000.0), "extreme-control circuit must prepare");
        expect(circuit.setToneControls(control[0], control[1], control[2]),
               "extreme tone-stack matrix rebuild must succeed");
        for (int sample = 0; sample < 8192; ++sample)
        {
            const auto phase = 2.0 * pi * (80.0 + 1200.0 * sample / 8192.0)
                * sample / 192000.0;
            const auto output = circuit.processSample(static_cast<float>(0.2 * std::sin(phase)));
            expect(std::isfinite(output), "extreme-control output must remain finite");
        }
        const auto& stats = circuit.getSolverStats();
        expect(stats.failedSamples < 82,
               "fewer than 1% of extreme-control samples may fail Newton convergence");
    }
}
} // namespace

int main()
{
    testTriodeJacobian();
    testColumnPivotedQr();
    testVolumeBrightFilter();
    testAmpCircuit();
    testStartupSteadyState();
    testToneExtremes();

    if (failures != 0)
    {
        std::cerr << failures << " test assertion(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All circuit tests passed\n";
    return EXIT_SUCCESS;
}
