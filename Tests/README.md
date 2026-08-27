# 测试

`Automated/CircuitTests.cpp` 是可编译运行的自动数值测试，不是测试音频。它检查：

- 12AX7 解析雅可比与有限差分是否一致；
- Volume/Bright 冲激响应是否有限；
- DK 矩阵、输出数值和 Newton 收敛情况。

通过根目录的 CMake/CTest 命令运行。仓库暂不包含测试音频，因为目前没有定义经过校准且可公开分发的参考录音。
