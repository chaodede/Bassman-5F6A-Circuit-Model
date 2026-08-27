# Bassman 5F6-A Research Amp

一个面向研究与复现的 JUCE 8 吉他前级插件。它把早期的 Fender Bassman 5F6-A 电路模拟研究整理为可构建、可测试、适合公开到 GitHub 的工程；重点是电路方程、离散化和数值求解，不是黑箱或神经网络拟合。

> 当前状态：VST3 与 Standalone 已在 Windows、JUCE 8.0.8、MSVC 19.44 下构建通过。它是前级研究模型，不包含功放、输出变压器、音箱或麦克风卷积，也不应被视为对某台实机的测量级复刻。

## 信号路径

```mermaid
flowchart LR
    A[输入增益] --> B[第一只 12AX7<br/>工作区线性拟合]
    B --> C[耦合电容 + Volume/Bright<br/>双线性变换 IIR]
    C --> D[两只 12AX7 + 三段 Tone Stack<br/>Nodal DK 状态空间]
    D --> E[4x 过采样域 Dry/Wet]
    E --> F[抗混叠降采样]
    F --> G[输出增益]
```

工程保留了当年研究里最有解释力的一条路线：从原理图和元件值出发，把容易解的线性部分解析化，把强耦合的电子管与音调网络写成非线性状态空间系统，再逐样本求解。曾尝试的卷积/GRU 替代方案只在[研究脉络](docs/research-process.md#一次没有进入最终版本的分支神经网络替代)中作为失败经验记录，不进入插件信号链。

## 与旧工程相比

- 使用现代 CMake，不提交 `.vs`、OBJ、PDB、JUCE 生成文件或本机绝对路径。
- 数学核心独立于 JUCE，可单独编译和测试。
- 用带主元选择的直接线性求解替代每次 Newton 迭代中的 4×4 SVD 伪逆。
- 使用阻尼 Newton、残差判据、有限值保护和失败计数。
- 修正旧实现中三极管板极支路雅可比的参数抄写错误，并补上遗漏的栅流导数项。
- 4 倍过采样包围整条电路模型，降低非线性产生的可闻混叠。
- 左右声道具有独立电路状态，不再把左声道结果复制到所有声道。
- 参数由 `AudioProcessorValueTreeState` 管理，支持宿主自动化和状态保存。
- Mix 是真正的 dry/wet 混合；两路都经过同一过采样滤波链以保持对齐。
- 音频线程中不做逐块 `malloc/free`；音调旋钮改变时仍需重建小型 DK 矩阵，这是当前明确保留的研究版折衷。

详细审计见[旧实现审计与迁移](docs/legacy-audit.md)，数学和实现对应关系见[实现说明](docs/implementation.md)。

## 构建

要求：CMake 3.22+、支持 C++17 的编译器，以及 JUCE 8。默认会通过 `FetchContent` 获取 JUCE 8.0.8；也可以指定本地 JUCE 源码目录。

```powershell
cmake -S . -B build -DJUCE_SOURCE_DIR="D:/path/to/JUCE"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

只验证电路核心、不下载或构建 JUCE：

```powershell
cmake -S . -B build/core -DBASSMAN_BUILD_PLUGIN=OFF
cmake --build build/core --config Release --parallel
ctest --test-dir build/core -C Release --output-on-failure
```

构建产物位于 CMake 构建目录的 `Bassman5F6AResearchAmp_artefacts` 下。仓库不会自动把插件复制到系统插件目录。

## 参数

| 参数 | 作用 |
| --- | --- |
| Input | 进入电路前的增益，−24 至 +24 dB |
| Bright | 切换音量电位器上的亮音电容路径 |
| Volume | 5F6-A 音量网络位置，限制在 0.01–0.99 以避免理想端点奇异性 |
| Treble / Bass / Middle | 音调网络电位器位置，0.01–0.99 |
| Mix | 过采样域内的干湿混合 |
| Output | 输出增益，−24 至 +24 dB |

## 文档

- [完整研究脉络](docs/research-process.md)：从资料、拆分、推导、实现到失败分支和本次重构。
- [数学与代码实现](docs/implementation.md)：元件、矩阵、Newton 求解和实时音频结构。
- [旧实现审计与迁移](docs/legacy-audit.md)：发现的问题、修改理由和仍然存在的限制。
- [参考文献](docs/references.md)：论文、书籍、框架与延伸阅读；仓库不再分发原论文 PDF。

## 验证范围

当前自动测试覆盖：

- Dempwolf–Zölzer 三极管模型解析雅可比与中心有限差分的一致性；
- Volume/Bright 脉冲响应的有限性与有界性；
- 192 kHz 下整条电路对标称 220 Hz 输入的非静音、有限值和 Newton 收敛率。

尚未完成的验证包括：与真实 5F6-A 的扫频/多音/动态测量对齐、不同 12AX7 个体的参数拟合、宿主自动化压力测试、长期 CPU 基准和正式插件验证器测试。欢迎用可公开的测量数据补齐这些部分。

## 许可与商标

本仓库原创代码使用 [GNU AGPL v3 或更高版本](LICENSE)。JUCE 8 自身采用 AGPLv3/商业双许可；若以其他方式发布二进制，请自行确认所选 JUCE 许可和所有第三方 SDK 条款。

“Fender”和“Bassman”是其各自权利人的商标。本项目与 Fender Musical Instruments Corporation 无关联、无授权，也不包含其商标图形、受版权保护的资料或官方音频素材。型号名称只用于说明所研究的历史电路拓扑。
