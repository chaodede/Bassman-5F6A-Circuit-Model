// SPDX-License-Identifier: AGPL-3.0-or-later
#include "DSP/AmpCircuit.h"
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
    testVolumeBrightFilter();
    testAmpCircuit();
    testToneExtremes();

    if (failures != 0)
    {
        std::cerr << failures << " test assertion(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All circuit tests passed\n";
    return EXIT_SUCCESS;
}
