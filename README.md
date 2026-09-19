# DivisionHud4SA

GTA: San Andreas (1.0 US) ASI 插件，基于 [plugin-sdk](https://github.com/DK22Pac/plugin-sdk) 编写。

## 构建

### 环境要求

- Visual Studio 2022 Build Tools（MSVC v143 + Windows SDK）
- CMake 3.21+
- 环境变量 `PLUGIN_SDK_DIR`、`GTA_SA_DIR`
- 已构建 plugin-sdk 的 Release 静态库 `%PLUGIN_SDK_DIR%\output\lib\Plugin.lib`

### 步骤

```powershell
cmake -S . -B build
cmake --build build --config Release -j 12
```

产物：`%GTA_SA_DIR%\scripts\DivisionHud4SA.asi`
