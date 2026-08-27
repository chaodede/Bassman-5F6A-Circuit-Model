# 参考文献与资料边界

下面列出对本项目建模路线最直接的资料。链接指向作者、会议论文库或官方项目；仓库不复制本地收集的 PDF、书籍或官方原理图。

## 核心电路建模

1. Kristjan Dempwolf, Martin Holters, Udo Zölzer, “Discretization of Parametric Analog Circuits for Real-Time Simulations,” DAFx-10, 2010. [DAFx 论文页](https://dafx.de/paper-archive/details/74c2I_PbWhkM6MUukii5IQ) / [PDF](https://www.dafx.de/paper-archive/2010/DAFx10/DempwolfHoltersZoelzer_DAFx10_P7.pdf)。状态空间、梯形离散化和参数电路重计算背景。
2. Kristjan Dempwolf, Udo Zölzer, “A Physically-Motivated Triode Model for Circuit Simulations,” DAFx-11, 2011. [DAFx 论文页](https://dafx.de/paper-archive/details/MGMeIyM6_9oAzOgKCuzEqw) / [PDF](https://dafx.de/paper-archive/2011/Papers/76_e.pdf)。本项目 12AX7 连续可微电流模型及参数来源。
3. David T. Yeh, Julius O. Smith, “Discretization of the ’59 Fender Bassman Tone Stack,” DAFx-06, 2006. [DAFx 论文页](https://www.dafx.de/paper-archive/details/bbvGpyU59H2nHvV3Iamezg) / [PDF](https://dafx.de/paper-archive/2006/papers/p_001.pdf)。Bassman 三段音调网络的符号分析、旋钮耦合与双线性离散化。
4. Martin Holters, Udo Zölzer, “Physical Modelling of a Wah-wah Effect Pedal as a Case Study for Application of the Nodal DK Method to Circuits with Variable Parts,” DAFx-11, 2011. [DAFx 检索页](https://www.dafx.de/paper-archive/search?p=1&years%5B%5D=2011)。Nodal DK 的系统化构建和可变元件问题。
5. Ben Holmes, Maarten van Walstijn, “Improving the Robustness of the Iterative Solver in State-Space Modelling of Guitar Distortion Circuitry,” DAFx-15, 2015. [DAFx 论文页](https://dafx.de/paper-archive/details/eQlmrNEUQPwf9tpZ9Kr1Wg) / [PDF](https://dafx.de/paper-archive/2015/DAFx-15_submission_18.pdf)。非线性状态空间迭代器的稳定性和成本背景。
6. W. Ross Dunkel, Maximilian Rest, Kurt James Werner, Michael Jørgen Olsen, Julius O. Smith III, “The Fender Bassman 5F6-A Family of Preamplifier Circuits—A Wave Digital Filter Case Study,” DAFx-16, 2016. [PDF](https://www.dafx.de/paper-archive/2016/dafxpapers/37-DAFx-16_paper_53-PN.pdf)。另一种完整 5F6-A 前级物理建模路线，可作为后续交叉验证。
7. David T. Yeh, *Digital Implementation of Musical Distortion Circuits by Analysis and Simulation*, PhD dissertation, Stanford University, 2009。旧资料中的 `DavidYehThesissinglesided.pdf`；涉及自动化物理建模、非线性电路与真空管实例。
8. Udo Zölzer (ed.), *DAFX: Digital Audio Effects*, 2nd ed., Wiley, 2011。虚拟模拟、滤波器、非线性处理和抗混叠背景。

## 参考实现与框架

- Jaromir Macak, [NodalDKFramework](https://github.com/jardamacak/NodalDKFramework)。旧研究曾用其 MATLAB 框架探索拓扑到状态空间矩阵的生成；其本地副本许可标注不一致，因此当前仓库没有复制其代码。
- João Rossi Filho, Jaromir Macak, [dkmethod](https://github.com/joaorossi/dkmethod)。JUCE Nodal DK 演示模块；用于理解历史工程来源，不作为当前固定尺寸数学实现的代码依赖。
- [JUCE](https://github.com/juce-framework/JUCE)，跨平台音频插件框架。请同时阅读其[官方许可说明](https://github.com/juce-framework/JUCE/blob/master/LICENSE.md)。

## 神经网络分支的背景阅读

Tara Vanhatalo, Pierrick Legrand, Myriam Desainte-Catherine, Pierre Hanna, Antoine Brusco, Guillaume Pille, Yann Bayle, “A Review of Neural Network-Based Emulation of Guitar Amplifiers,” *Applied Sciences*, 12(12), 5894, 2022. [DOI: 10.3390/app12125894](https://doi.org/10.3390/app12125894)。本项目不实现神经模型；该综述只帮助定位当年 GRU/卷积尝试在领域中的位置。

## 项目自己的历史资料

本次整理还检查了以下未公开资料类别：

- 《电路箱头模拟》DOCX/PDF 与 5 页研究演示；
- 5F6-A MATLAB 全电路和简化 DK 模型；
- 离线 C++、JUCE 插件和嵌入式版本；
- SPICE/原理图文件、扫频导出表格、WAV 和训练实验目录。

这些文件留在原研究资料目录，不进入 GitHub 仓库。若未来公开测量数据，建议另建带数据字典、来源、许可和校准信息的 release 或数据仓库，而不是直接上传整个历史目录。
