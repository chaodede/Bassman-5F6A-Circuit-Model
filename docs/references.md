# 核心参考

这里只保留直接影响当前实现的来源。

## 主要工具

1. [LiveSPICE](https://www.livespice.org/)：实时、低延迟的 SPICE-like 音频电路仿真器，也是本项目最重要的实践参考。官网说明了 CAS 预分析、JIT、简化器件模型以及 Bassman 5F6-A 前级示例。
2. [Online Circuit Solver](https://onlinecircuitsolver.com/)：使用 Modified Nodal Analysis 计算线性 RLC/运放网络的拉普拉斯传递函数。本研究用它处理和检查线性块公式。

## 数学与器件模型

3. Kristjan Dempwolf, Martin Holters, Udo Zölzer, “Discretization of Parametric Analog Circuits for Real-Time Simulations,” DAFx-10, 2010. [论文](https://www.dafx.de/paper-archive/2010/DAFx10/DempwolfHoltersZoelzer_DAFx10_P7.pdf)
4. Kristjan Dempwolf, Udo Zölzer, “A Physically-Motivated Triode Model for Circuit Simulations,” DAFx-11, 2011. [论文](https://dafx.de/paper-archive/2011/Papers/76_e.pdf)
5. David T. Yeh, Julius O. Smith, “Discretization of the ’59 Fender Bassman Tone Stack,” DAFx-06, 2006. [论文](https://dafx.de/paper-archive/2006/papers/p_001.pdf)
6. Martin Holters, Udo Zölzer, “Physical Modelling of a Wah-wah Effect Pedal as a Case Study for Application of the Nodal DK Method to Circuits with Variable Parts,” DAFx-11, 2011. [论文库](https://www.dafx.de/paper-archive/search?p=1&years%5B%5D=2011)

## 软件框架

7. [JUCE](https://github.com/juce-framework/JUCE)：VST3/Standalone、参数、状态管理和过采样框架。

外部网站、论文和 JUCE 不属于本仓库，并受各自许可与版权约束。
