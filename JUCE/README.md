# JUCE 插件工程

使用 Projucer 打开 `Bassman5F6ACircuitModel.jucer`，然后选择 **Save Project and Open in IDE**。

工程包含 VST3 和 Standalone targets。默认模块路径假定 JUCE Framework 与本仓库位于同一个父目录：

```text
parent/
├── Bassman-5F6A-Circuit-Model/
└── JUCE/
    └── modules/
```

如果 JUCE 位于其他目录，在保存工程前通过 Projucer 设置本机 `JUCE/modules` 路径。
