<div align=center>
    <h1 align="center">Vulcal Engine</h1>
</div>

<div align=center>

[官方文档](https://virtual-graphics.github.io/VulcalEngine-website/docs/intro/) · [构建指南](https://virtual-graphics.github.io/VulcalEngine-website/docs/building/) · [路线图](https://github.com/orgs/Virtual-Graphics/projects/1) · [开发日志](https://virtual-graphics.github.io/VulcalEngine-website/blog/)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue)
![C++ VERSION](https://img.shields.io/badge/C%2B%2B23-00599C?style=flat&logo=cplusplus&logoColor=white)

<a href="https://github.com/Virtual-Graphics/VulcalEngine/actions/workflows/ci-windows.yml?query=branch%3Amain"><img src="https://github.com/Virtual-Graphics/VulcalEngine/actions/workflows/ci-windows.yml/badge.svg?branch=main" alt="Windows CI"></a>
<a href="https://github.com/Virtual-Graphics/VulcalEngine/actions/workflows/ci-linux.yml?query=branch%3Amain"><img src="https://github.com/Virtual-Graphics/VulcalEngine/actions/workflows/ci-linux.yml/badge.svg?branch=main" alt="Linux CI"></a>

</div>

**Vulcal Engine 是一个面向 Windows 和 Linux 的轻量级游戏引擎。** 引擎以 C++23 和 ECS 为基础，使用 OpenGL 4.6 渲染，支持通过 Lua 编写游戏逻辑。

## 功能概览

- **场景与脚本**。使用 ECS 组织实体和组件，在编辑器中搭建场景，并通过 Lua 驱动游戏逻辑。
- **渲染与资产导入**。提供 OpenGL 4.6 渲染、精灵绘制、OBJ 模型和纹理导入，以及独立的模型查看器。
- **编辑、试玩与打包**。在编辑器中通过 **Play** 试玩场景，通过 **File → Build** 生成可由独立播放器运行的游戏包。

## 示例项目

| 贪吃蛇 | 最后三分钟 |
| --- | --- |
| <img src="./screen_shots/snake.png" width="400" alt="贪吃蛇游戏截图"> | <img src="./screen_shots/arena.png" width="400" alt="最后三分钟游戏截图"> |
| 打开 [snake.vproj](./projects/snake/snake.vproj) 并点击 **Play**。使用 WASD 移动，空格重新开始。 | 打开 [arena.vproj](./projects/arena/arena.vproj) 并点击 **Play**。按 Enter 开始，WASD 移动，方向键射击，空格冲刺。查看[游戏规则与测试](./projects/arena/README.md)。 |

## 快速开始

准备 Git、CMake 3.28 或更高版本，以及支持 C++23 的编译器。首次配置需要联网下载依赖。运行编辑器和播放器需要支持 OpenGL 4.6 的显卡驱动。

从 [Releases](https://github.com/Virtual-Graphics/VulcalEngine/releases) 下载源码，或克隆仓库，然后在源码根目录执行对应平台的命令。

优先使用仓库提供的 CMake 预设。Windows 开发构建使用 `msvc`，Linux 开发构建使用 `linux-debug`，构建分发用播放器使用 `release-static`。运行 `cmake --list-presets=all` 可查看可用预设。

### Windows

安装 Visual Studio 2026 的 C++ 桌面开发工作负载。

```powershell
cmake --preset msvc
cmake --build --preset msvc --parallel
.\build\bin\Debug\vulcal_editor.exe
```

### Linux

安装 Ninja 和[构建指南列出的系统依赖](https://virtual-graphics.github.io/VulcalEngine-website/docs/building/#linux)。

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug --parallel
./build/linux-debug/bin/vulcal_editor
```

编辑器启动后，打开任一示例的 `.vproj` 并点击 **Play**。要打包游戏，在 **File → Build Settings…** 中选择启动场景，再执行 **File → Build**。输出目录包含游戏的 `.vpak` 和播放器可执行文件。

## 文档

- [构建与打包](https://virtual-graphics.github.io/VulcalEngine-website/docs/building/)涵盖 CMake 预设、测试、发布构建和播放器日志。
- [引擎架构](https://virtual-graphics.github.io/VulcalEngine-website/docs/architecture/overview/)介绍核心模块和渲染架构。
- [Tracy 性能分析](https://virtual-graphics.github.io/VulcalEngine-website/docs/profiling/)介绍如何采集 CPU、GPU 和帧统计。
- 文档维护在 [VulcalEngine-website](https://github.com/Virtual-Graphics/VulcalEngine-website) 仓库。

## 贡献与许可证

欢迎通过 [Issues](https://github.com/Virtual-Graphics/VulcalEngine/issues) 报告问题或讨论改进。提交代码前，请阅读 [CONTRIBUTING.md](./CONTRIBUTING.md) 中的改动范围、命名、提交和 CI 要求。

Vulcal Engine 采用 [MIT 许可证](./LICENSE)。
