# Bassman 5F6-A Circuit Research Plug-in

JUCE VST3/Standalone plugin for a circuit-based 5F6-A-style guitar preamp model.

这是一个用电路方程模拟 5F6-A 风格前级的 JUCE 插件。

<p align="center">
  <img src="docs/images/target-5f6a-inspired-amp.png" width="520" alt="5F6-A 时代 4x10 tweed 音箱概念图">
</p>

## LiveSPICE 参考电路

<p align="center">
  <img src="docs/images/livespice-bassman-preamp.png" width="760" alt="LiveSPICE 中的 Fender Bassman 5F6-A 前级和音调网络">
</p>

<p align="center"><sub>来源：<a href="https://www.livespice.org/">LiveSPICE 官方 Bassman 5F6-A preamp 示例</a>。该图用于对照电路范围，不是本项目界面截图。</sub></p>

## 方法适用性

5F6-A 是当前实现案例，不是方法的适用边界。核心流程是：根据电路拓扑生成关联矩阵，将线性网络离散为 DK 状态空间，再逐采样求解非线性器件端口。

| 可复用方法 | 5F6-A 案例配置 |
| --- | --- |
| DK 矩阵构建、Newton/伪逆求解、稳态初始化、过采样 | 节点连接、元件值、12AX7 参数和 Tone Stack |

替换拓扑、元件参数和非线性器件模型后，同一方法可用于其他电子管音箱前级、失真电路和模拟效果器。

## 实现范围

```text
Input → 第一前级近似 → Volume/Bright
      → 两只 12AX7 + Treble/Bass/Middle Nodal DK 网络
      → 4× oversampling → Mix/Output
```

包含 VST3、Standalone、双声道处理、参数保存和核心 DSP 测试。

不包含功率放大级、输出变压器、扬声器、箱体和麦克风模型。

## 构建

要求 CMake 3.22+、C++17 和 JUCE 8。

```powershell
git clone https://github.com/chaodede/Bassman-5F6A-Circuit-Model.git
cd Bassman-5F6A-Circuit-Model
cmake -S . -B build -DJUCE_SOURCE_DIR="D:/path/to/JUCE"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

未指定 `JUCE_SOURCE_DIR` 时，CMake 会获取 JUCE 8.0.8。

只构建和测试电路核心：

```powershell
cmake -S . -B build/core -DBASSMAN_BUILD_PLUGIN=OFF
cmake --build build/core --config Release --parallel
ctest --test-dir build/core -C Release --output-on-failure
```

## 代码入口

- `Source/DSP/AmpCircuit.*`：完整信号链
- `Source/DSP/NodalDKModel.*`：电路矩阵和 Newton 求解
- `Source/DSP/TriodeModel.*`：12AX7 模型
- `Source/PluginProcessor.*`：JUCE、参数和过采样
- `Tests/CircuitTests.cpp`：核心测试

DK Method、三极管方程和非线性求解见[核心算法](docs/research-process.md)，资料来源见[参考资料](docs/references.md)。

代码使用 [AGPL-3.0-or-later](LICENSE)。本项目与 Fender 及所引用的软件项目无关联。
