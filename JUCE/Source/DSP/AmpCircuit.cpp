// SPDX-License-Identifier: AGPL-3.0-or-later
#include "AmpCircuit.h"

namespace bassman
{
bool AmpCircuit::prepare(double sampleRate) noexcept
{
    volumeBright.prepare(sampleRate);
    volumeBright.reset(firstStageQuiescentVoltage);
    return nonlinearToneStack.prepare(sampleRate);
}

void AmpCircuit::reset() noexcept
{
    volumeBright.reset(firstStageQuiescentVoltage);
    nonlinearToneStack.reset();
}

void AmpCircuit::setBright(bool bright) noexcept
{
    volumeBright.setBright(bright);
}

void AmpCircuit::setVolume(double volume) noexcept
{
    volumeBright.setVolume(volume);
}

bool AmpCircuit::setToneControls(double treble, double bass, double middle) noexcept
{
    return nonlinearToneStack.setToneControls(treble, bass, middle);
}

float AmpCircuit::processSample(float input) noexcept
{
    // The original study fitted the first 12AX7 stage over the intended input range.
    const auto firstStagePlateVoltage = firstStageGain * input
        + firstStageQuiescentVoltage;
    const auto coupledSignal = volumeBright.processSample(firstStagePlateVoltage);
    return nonlinearToneStack.processSample(coupledSignal);
}

const NodalDKModel::SolverStats& AmpCircuit::getSolverStats() const noexcept
{
    return nonlinearToneStack.getSolverStats();
}
} // namespace bassman
