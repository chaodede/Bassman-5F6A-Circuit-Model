# 研究记录

## 1. 原始目标

研究对象是 5F6-A 风格前级。原始资料包括原理图、SPICE、MATLAB Nodal DK、离线 C++ 和早期 JUCE 工程。

目标是把电路模型放进实时插件，同时保留 Volume、Bright、Treble、Bass、Middle 与元件参数的对应关系。

## 2. 建模过程

1. 根据原理图确定节点、元件值和 12AX7 连接。
2. 使用 MATLAB/Nodal DK 原型从电路拓扑生成状态空间矩阵。
3. 使用 Online Circuit Solver 辅助检查 RC、Volume/Bright 和音调网络的线性公式。
4. 使用 LiveSPICE 的 Bassman 示例和实时仿真思路作对照。
5. 将模型拆为第一前级近似、Volume/Bright IIR、两只 12AX7 与音调网络。
6. 将离线 C++ 求解器移植到 JUCE，加入过采样和插件参数。

![当前 C++ 实现的 Nodal DK 电路](images/dk-circuit-topology.svg)

## 3. 当前求解方式

- 第一前级：工作区线性近似。
- Volume/Bright：由连续域 RC 关系离散得到 IIR。
- 两只 12AX7 与音调网络：12 节点 Nodal DK 系统。
- 非线性：每采样点 Newton 迭代，内部解 4×4 线性系统。
- 抗混叠：整个电路在 4 倍采样率运行。

对应代码：

```text
AmpCircuit       信号链
VolumeBrightFilter
NodalDKModel     电路矩阵、状态和 Newton
TriodeModel      12AX7 电流及雅可比
```

## 4. 整理旧工程时做的修正

- 修正三极管解析雅可比；
- 用带主元的线性求解替代逐次 SVD 伪逆；
- 增加 Newton 阻尼和有限值保护；
- 去掉音频路径中的动态分配；
- 增加双声道状态、参数保存、Dry/Wet 和自动测试。

卷积和 GRU 曾用于替代非线性求解，但声音和泛化不理想，当前版本不包含该分支。

## 5. 限制

当前模型没有实现功放、输出变压器、扬声器和箱体，也没有完成与实机的系统测量校准。下一步应使用同一输入与 LiveSPICE、SPICE 或实机测试点比较扫频、谐波和瞬态。
