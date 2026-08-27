# Bassman 5F6-A Research Amp

[![Circuit core CI](https://github.com/chaodede/simulation-of-guitar-amplifiers/actions/workflows/ci.yml/badge.svg)](https://github.com/chaodede/simulation-of-guitar-amplifiers/actions/workflows/ci.yml)
[![License: AGPL v3+](https://img.shields.io/badge/license-AGPL--3.0%2B-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C.svg)](CMakeLists.txt)
[![JUCE 8](https://img.shields.io/badge/JUCE-8-f39c12.svg)](https://github.com/juce-framework/JUCE)

**A circuit-derived virtual-analog model of the 1959-era 5F6-A guitar-amplifier preamp and tone stack, packaged as a JUCE VST3/Standalone plug-in.**

这是一个从电子管电路拓扑和元件参数出发构建的吉他音箱前级模拟项目。它研究的对象是经典 5F6-A 架构中的三个 12AX7 前级、Volume/Bright 网络和 Treble/Bass/Middle 音调网络，而不是用神经网络或一条静态失真曲线拟合整台音箱。

![原创的 5F6-A 时代 4x10 tweed 组合音箱概念图](docs/images/target-5f6a-inspired-amp.png)

<p align="center"><sub>原创无商标概念图，用来说明研究对象的 1950 年代 4×10 tweed 组合音箱形态；不是官方产品照片。</sub></p>

## 这个项目模拟了什么？

真实 5F6-A 是一整台电子管组合音箱。当前插件只模拟产生前级增益、非线性和音调交互的部分：

![模型覆盖范围](docs/images/model-scope.svg)

| 已实现 | 当前未实现 |
| --- | --- |
| 第一前级的工作区线性拟合 | 功率放大级与相位反相器 |
| Volume、Bright 与耦合电容 | 输出变压器、电源下陷与反馈环路 |
| 两只 12AX7 的连续可微非线性模型 | 4×10 扬声器、箱体和麦克风响应 |
| 5F6-A Treble/Bass/Middle 网络 | 元件容差、电子管个体差异和噪声 |
| Nodal DK 状态空间与逐采样 Newton 求解 | 对某台实机的测量级校准 |
| 4× 过采样、Dry/Wet 和立体声独立状态 | AU、AAX、LV2 和安装程序 |

因此它更适合做以下事情：

- 学习电子管放大器的虚拟模拟、状态空间离散化和非线性数值求解；
- 对比旋钮位置、元件值、采样率和 Newton 策略对输出的影响；
- 作为可继续测量、验证和改进的 JUCE 研究插件。

如果要得到完整的录音音色，通常还要在插件后接一个合法来源的箱体 IR 或扬声器/麦克风模型。

## 电路到代码

信号先经过第一前级的局部拟合，再进入解析离散化的 Volume/Bright RC 网络。后两只 12AX7 和音调网络组成一个 12 节点、3 状态、4 非线性端口的 Nodal DK 系统；每个采样点使用带阻尼的 Newton 法求端口电压。

![代码中实现的 Nodal DK 电路拓扑](docs/images/dk-circuit-topology.svg)

完整元件表、节点含义和模型边界见[电路与模型说明](docs/circuit-and-model.md)。详细推导见[研究过程](docs/research-process.md)，代码对应关系见[实现说明](docs/implementation.md)。

## 插件参数

| 参数 | 作用 |
| --- | --- |
| Input | 进入前级模型的增益，−24 至 +24 dB |
| Bright | 切换音量电位器上的 100 pF 亮音电容路径 |
| Volume | 1 MΩ 音量电位器位置，范围 0.01–0.99 |
| Treble | 250 kΩ Treble 电位器位置 |
| Bass | 500 kΩ Bass 电位器位置 |
| Middle | 25 kΩ Middle 电位器位置 |
| Mix | 过采样域中的干湿比例 |
| Output | 插件总输出增益，−24 至 +24 dB |

默认界面使用 JUCE Generic Editor，重点是让宿主自动化、状态保存和 DSP 核心先保持清晰可复现。自定义面板不是当前研究结论的一部分。

## 获取与构建

目前仓库提供源码，没有预编译 Release。需要 CMake 3.22+、支持 C++17 的编译器和 JUCE 8。若不指定 `JUCE_SOURCE_DIR`，CMake 会通过 `FetchContent` 获取 JUCE 8.0.8。

```powershell
git clone https://github.com/chaodede/simulation-of-guitar-amplifiers.git
cd simulation-of-guitar-amplifiers
cmake -S . -B build -DJUCE_SOURCE_DIR="D:/path/to/JUCE"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

构建产物位于：

```text
build/Bassman5F6AResearchAmp_artefacts/Release/
├── Standalone/Bassman 5F6-A Research Amp.exe
└── VST3/Bassman 5F6-A Research Amp.vst3/
```

只构建不依赖 JUCE 的电路核心和测试：

```powershell
cmake -S . -B build/core -DBASSMAN_BUILD_PLUGIN=OFF
cmake --build build/core --config Release --parallel
ctest --test-dir build/core -C Release --output-on-failure
```

## 实现特点

- 固定尺寸 `std::array` 矩阵，DSP 核心不依赖 JUCE。
- Dempwolf–Zölzer 12AX7 电流模型及解析雅可比。
- 带部分主元的 4×4 线性求解，不在每次 Newton 迭代中计算 SVD 伪逆。
- 阻尼线搜索、残差/步长双判据、有限值保护和失败统计。
- 4× 半带 IIR 过采样降低强非线性产生的混叠。
- 左右声道各自保存电容状态和三极管端口电压。
- `AudioProcessorValueTreeState` 管理宿主参数和状态恢复。
- 音频处理路径没有逐块 `malloc/free`。

## 验证状态

自动测试目前覆盖：

- 三极管解析雅可比与中心有限差分的一致性；
- Volume/Bright 网络冲激响应的有限性和有界性；
- 192 kHz 内部采样率下，标称输入及四组极端 Tone 设置的输出有限性和 Newton 收敛率。

Windows 上已使用 JUCE 8.0.8 和 MSVC 19.44 构建 VST3 与 Standalone。核心测试还通过 GitHub Actions 在 Windows、macOS 和 Linux 上运行。

这并不等于已经完成实机验证。后续最重要的工作是用明确校准的实机或 SPICE 数据比较扫频、谐波、瞬态、旋钮轨迹与过采样倍率。详见[验证计划](docs/research-process.md#8-应如何验证这个模型)。

## 文档导航

| 文档 | 适合谁看 | 内容 |
| --- | --- | --- |
| [电路与模型说明](docs/circuit-and-model.md) | 第一次看到项目的人 | 硬件目标、模型范围、元件表和电路图 |
| [完整研究过程](docs/research-process.md) | DSP/虚拟模拟研究者 | 拆分思路、离散化、DK 方程、三极管和 Newton |
| [实现说明](docs/implementation.md) | C++/JUCE 开发者 | 文件结构、实时路径、参数更新和数值约定 |
| [开发历史与旧实现审计](docs/legacy-audit.md) | 维护者 | 历史工程问题、修复理由和许可边界 |
| [参考文献](docs/references.md) | 需要追溯来源的读者 | DAFx 论文、书籍、框架与延伸阅读 |

卷积/GRU 替代曾作为降低计算量的实验分支，但音质和泛化没有达到预期，因此不包含在当前插件中；详情只保留在研究过程里。

## 许可、图片与商标

原创代码使用 [GNU AGPL v3 或更高版本](LICENSE)。JUCE 8 采用 AGPLv3/商业双许可；发布二进制前请确认所采用的 JUCE 和插件 SDK 许可。

仓库中的音箱概念图由本项目生成，不是 Fender 官方图片。电路 SVG 根据本仓库实现重新绘制。仓库不分发收集到的论文 PDF、书籍、官方原理图扫描、训练数据或来源不清的音频。

“Fender”和“Bassman”是其各自权利人的商标。本项目与 Fender Musical Instruments Corporation 无关联、无授权；型号名称仅用于说明被研究的历史电路拓扑。
