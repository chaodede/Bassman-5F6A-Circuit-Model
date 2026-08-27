# 测试

`Automated/CircuitTests.cpp` 是电路核心的自动数值测试。它检查：

- 12AX7 解析雅可比与有限差分是否一致；
- 列主元 Householder QR 的解与残差；
- Volume/Bright 冲激响应是否有限；
- DK 矩阵、启动稳态、输出数值和 Newton 收敛情况。

`Plugin/ProcessorTests.cpp` 检查插件层的无瞬态静音启动、超大离线音频块，以及
`processBlock()` 与 `releaseResources()` 交错时的资源生命周期。

通过根目录的 CMake/CTest 命令运行。仓库暂不包含测试音频，因为目前没有定义经过校准且可公开分发的参考录音。
