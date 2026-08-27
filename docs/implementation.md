# 数学与代码实现

本文把[电路图](circuit-and-model.md)映射到 C++ 文件和实时处理顺序。Nodal DK 的完整数学推导位于[研究过程](research-process.md#4-nodal-dk-方程如何得到)。

## 目录对应关系

| 文件 | 职责 |
| --- | --- |
| `Source/DSP/TriodeModel.*` | 12AX7 电流方程和 2×2 解析雅可比 |
| `Source/DSP/VolumeBrightFilter.*` | 第一放大级后的 RC、Volume、Bright 双线性 IIR |
| `Source/DSP/LinearAlgebra.h` | 固定尺寸矩阵、转置/乘法、带部分主元的求解与求逆 |
| `Source/DSP/NodalDKModel.*` | 12 节点拓扑、A–K 矩阵、稳态、阻尼 Newton、逐样本状态更新 |
| `Source/DSP/AmpCircuit.*` | 第一三极管拟合与两个电路块的串联 |
| `Source/PluginProcessor.*` | JUCE 参数、4× 过采样、多声道、状态保存和干湿混合 |
| `Tests/CircuitTests.cpp` | 不依赖 JUCE 的数值回归测试 |

## 每个采样点的执行顺序

1. 第一放大级线性拟合产生带直流工作点的板极电压。
2. Volume/Bright 高通网络隔离直流并形成电平/高频响应。
3. DK 系统计算 $P=Gx+Hu$。
4. 阻尼 Newton 求 $P+Ki(v)-v=0$。
5. 输出 $y=Dx+Eu+Fi$。
6. 更新 $x\leftarrow Ax+Bu+Ci$。

所有矩阵都是 `std::array` 固定尺寸对象。逐样本路径不创建堆对象，不做矩阵求逆，只求解一个 4×4 线性方程。

## 参数更新

Volume 只重算一阶/二阶 IIR 系数。Treble、Bass、Middle 改变网络电阻，需要重新生成并求逆 14×14 扩展节点矩阵；`NodalDKModel` 先在临时对象中构建，失败时保留旧矩阵。

当前插件在音频块边界读取宿主原子参数，并在检测到音调值变化时重建矩阵。这比旧版逐样本或无条件重建轻得多，但密集自动化时仍会在音频线程产生不恒定耗时。生产化可选方案包括：

- 控制线程预计算矩阵并在块边界无锁交换；
- 对旋钮网格预计算并插值，但需验证稳定性；
- 推导低秩/分块更新，避免完整求逆。

## 多声道和过采样

每个声道有独立的 `AmpCircuit`，因此左右输入不会共享电容状态或非线性端口电压。JUCE `Oversampling` 在 `prepareToPlay` 分配内存；处理时先复制上采样信号作为 dry，运行 wet 电路，再在上采样域混合。两路随后共同降采样，所以不会因 oversampler 延迟产生梳状混合。

宿主块如果异常超过 `prepareToPlay` 声明的最大块长，当前实现会清空该块而不是从音频线程扩容。这是显式的实时安全选择。

## 数值约定

- 电压、电流、电阻、电容分别使用 V、A、Ω、F。
- 内部矩阵和电子管计算使用 `double`，插件音频 I/O 使用 `float`。
- 电路输出沿用旧研究的 1/100 标度；Input/Output 参数承担实际工作电平校准。
- 电位器端点限制在 0.01–0.99，避免零欧姆理想支路使节点矩阵奇异。
- 线性求解阈值基于机器精度；Newton 严格残差阈值为 $10^{-7}$，放宽接受阈值为 $10^{-5}$。

## 可观测性

`NodalDKModel::SolverStats` 记录样本数、失败样本、总迭代数、最近迭代数和残差。当前只供测试/调试读取，没有放进插件 UI，以免实时路径增加同步负担。后续可在非音频线程定时采样这些统计量。
