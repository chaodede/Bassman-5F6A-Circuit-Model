# Bassman 5F6-A Circuit Research Plug-in

[![Circuit core CI](https://github.com/chaodede/simulation-of-guitar-amplifiers/actions/workflows/ci.yml/badge.svg)](https://github.com/chaodede/simulation-of-guitar-amplifiers/actions/workflows/ci.yml)
[![License: AGPL v3+](https://img.shields.io/badge/license-AGPL--3.0%2B-blue.svg)](LICENSE)

这是一个用 JUCE 实现的吉他音箱前级研究插件。目标不是复制整台音箱，也不是训练神经网络拟合声音，而是把 5F6-A 风格前级电路转成可以实时运行的 C++ 模型。

![无商标的 5F6-A 时代 4x10 tweed 音箱概念图](docs/images/target-5f6a-inspired-amp.png)

> 图片仅用于说明研究对象的外形年代和类型，不是官方产品照片。

## 项目做了什么

当前信号路径为：

```text
Input → 第一前级近似 → Volume/Bright → 两级 12AX7 非线性网络
      → Treble/Bass/Middle → 4× oversampling → Mix/Output
```

核心 DSP 包括：

- Volume/Bright 线性 RC 网络；
- 两只 12AX7 和音调网络的 Nodal DK 模型；
- 每采样点 Newton 非线性求解；
- 4 倍过采样、双声道状态、宿主参数和状态保存。

它没有模拟功率放大级、输出变压器、扬声器、箱体和麦克风。因此若用于录音，后面仍需要箱体 IR 或其他扬声器模型。

## 研究路线

这个项目最重要的参考不是某个神经网络结构，而是两类电路工具：

1. [LiveSPICE](https://www.livespice.org/) 是主要的实时电路仿真参考。它能够让真实音频通过 SPICE-like 电路，官方示例中也包含 Fender Bassman 5F6-A 前级。它对本项目最有价值的启发是：在音频开始前尽量完成电路分析和化简，实时阶段只保留必要计算。
2. [Online Circuit Solver](https://onlinecircuitsolver.com/) 用于处理和检查线性电路公式。它通过 Modified Nodal Analysis 计算 RLC/运放网络的拉普拉斯传递函数，适合验证 RC、Volume/Bright 和音调网络的线性关系。
3. 三极管非线性部分参考连续可微的 12AX7 模型，再使用 Nodal DK 和 Newton 法放进逐采样求解器。
4. 研究原型从 MATLAB/离线 C++ 逐步移植到 JUCE。卷积和 GRU 替代方案因效果与泛化不足，没有进入此版本。

更完整但保持简短的说明见[研究脉络](docs/research-process.md)，来源见[核心参考](docs/references.md)。

## 构建

要求 CMake 3.22+、C++17 编译器和 JUCE 8。可以传入本地 JUCE 路径；不传时 CMake 会获取 JUCE 8.0.8。

```powershell
git clone https://github.com/chaodede/simulation-of-guitar-amplifiers.git
cd simulation-of-guitar-amplifiers
cmake -S . -B build -DJUCE_SOURCE_DIR="D:/path/to/JUCE"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

输出包括 VST3 和 Standalone。只测试不依赖 JUCE 的电路核心：

```powershell
cmake -S . -B build/core -DBASSMAN_BUILD_PLUGIN=OFF
cmake --build build/core --config Release --parallel
ctest --test-dir build/core -C Release --output-on-failure
```

插件提供 Input、Bright、Volume、Treble、Bass、Middle、Mix 和 Output 参数，目前使用 JUCE Generic Editor。

## 仓库结构

```text
Source/DSP/             电路、三极管和数值求解
Source/PluginProcessor  JUCE 参数、过采样和音频处理
Tests/                  核心 DSP 自动测试
docs/                   研究脉络和参考来源
```

## 当前定位

这是一个可构建、可测试的研究实现，不是经过实机测量校准的商业级音箱复刻。最值得继续做的是与 LiveSPICE 或实机测试点比较扫频、谐波、瞬态和旋钮轨迹。

代码采用 [GNU AGPL v3 或更高版本](LICENSE)。JUCE、参考论文及外部工具有各自的许可。本项目与 Fender、LiveSPICE 和 Online Circuit Solver 均无隶属或授权关系。
