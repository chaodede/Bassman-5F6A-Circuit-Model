// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "NodalDKModel.h"
#include "VolumeBrightFilter.h"

namespace bassman
{
class AmpCircuit
{
public:
    bool prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setBright(bool bright) noexcept;
    void setVolume(double volume) noexcept;
    bool setToneControls(double treble, double bass, double middle) noexcept;
    [[nodiscard]] float processSample(float input) noexcept;
    [[nodiscard]] const NodalDKModel::SolverStats& getSolverStats() const noexcept;

private:
    VolumeBrightFilter volumeBright;
    NodalDKModel nonlinearToneStack;
};
} // namespace bassman
