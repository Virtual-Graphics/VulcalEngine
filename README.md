<div align=center>
    <h1 align="center">Vulcal Engine</h1>
</div>
<div align=center>

<a href="https://github.com/Virtual-Graphics/MiniEngine/actions/workflows/ci-windows.yml?query=branch%3Amain"><img src="https://github.com/Virtual-Graphics/MiniEngine/actions/workflows/ci-windows.yml/badge.svg?branch=main" alt="Windows CI"></a>
<a href="https://github.com/Virtual-Graphics/MiniEngine/actions/workflows/ci-linux.yml?query=branch%3Amain"><img src="https://github.com/Virtual-Graphics/MiniEngine/actions/workflows/ci-linux.yml/badge.svg?branch=main" alt="Linux CI"></a>

<a href="https://github.com/orgs/Virtual-Graphics/projects/1"> 路线图 </a> 
<a href="https://virtual-graphics.github.io/VulcalEngine-website/"> 关于 </a>


</div>


## INTRODUCTION
Vulcal Engine 是一个架构清晰、代码极简、无 AI Slop、非 Vibe Coding 、轻量、高性能以 ECS 架构为核心、目前使用 OpenGL 4.6 (未来明确支持 Vulkan) 为图形 API 、支持 Lua 脚本语言的游戏引擎。

## START
目前 Vulcal Engine 支持 Windows 与 Linux 两个平台，要求 CMake 版本大于等于 `3.28`，以及支持 C++23 的编译器和 OpenGL 4.6 的显卡驱动。第三方依赖会在配置阶段自动下载，首次配置需要联网。

构建：

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

Windows 也可以直接使用 CMake Preset，例如 `cmake --preset msvc` 或 `cmake --preset clang-debug`。

运行：

```bash
./build/bin/editor                    # 编辑器，可打开 projects/ 下的 .vproj
./build/bin/model_viewer model.obj    # 查看 OBJ 模型
ctest --test-dir build --output-on-failure
```

如果你依然构建失败，可以将如下 Prompt 发送给你的 Agent
```
帮我在电脑本地成功构建 https://github.com/Virtual-Graphics/VulcalEngine 这个项目
```
## 示例小游戏

- `projects/snake/snake.vproj`：贪吃蛇。
- `projects/arena/arena.vproj`：**最后三分钟**，单屏竞技场生存游戏。编辑器打开项目并点击 Play，Enter 开始，WASD 移动，方向键射击，空格冲刺。规则与测试见 [项目说明](projects/arena/README.md)。

## GOALS
我们最终期望 Vulcal Engine 能够做到
- 跨平台游戏引擎，支持 Windows、Linux、MacOS、Android、Web 发布游戏。
- 更适合个人开发者与 AI 配合的游戏开发工作流 
- 支持多渲染管线的渲染器架构，支持实时光线追踪管线，延迟渲染（主要），以及轻量级前向渲染管线。
- 高性能游戏引擎，支持在低端设备上高效运行，包括移动端。
- 可编程渲染管线，暴露低级图形 API 供开发者使用。
- Native C++ 开发模式

## PRINCIPLE
在 AI 时代下，稍微了解一些游戏引擎的程序员都能借助 Agents 开发游戏引擎，互联网上也出现各种所谓的 “自研引擎” 的演示视频，Vulcal Engine 的开发者也是 AI Coding 的重度依赖者，前前后后在 AI 的帮助下开发了 6 个游戏引擎了（不包括此引擎）。无论是作者自己用 AI Coding 的引擎，还是其他人用 AI Coding 出来的引擎，如果没有使用一种合理的开发流程那么最后都会沦落为和作者前 6 个引擎一样的下落逐渐被抛弃（原因是因为在有了 AI 的加持下，我天然的认为自己什么软件都能写出来了，但事实上相反的，我如果借助 AI 开发了我自己理解不了的代码（最终也许能实现出来，但也只能是个玩具），那么项目就会逐渐变得不可维护，而我脑海中存在各种奇思妙想的游戏引擎架构，我没有具体的架构方案（通常他们很复杂））。


当然如果其他开发者没有和我一样的 审美/代码品味/产品品味 他们也许不会抛弃，但结果就是代码成为屎山，软件不可维护。我们需要 [正确使用 AI Coding](https://blog.cglab.top/posts/new/2026-10-25-correct_for_ai_coding/) 。但据我所知大多数其他人的引擎最后都没有继续维护下去，因为他们意识到他们的软件已经变成了屎山，变得不可维护。所以我们不要 Vibe Coding 这只会浪费你的时间，浪费你的 token。

除此之外，程序员们在写一个软件的时候通常不会预先知道他们要开发的软件的全部细节、技术栈、架构等等。我们希望在开发的过程中学习到自己还不会的东西，但使用 Vibe Coding 你只能学会与 AI 扯皮，除此之外几乎什么都学不到。

所以 Vulcal Engine 采用的大致工作流是（不固定）:
1. 开发者调研方案 
2. 与 AI 或者 其他开发者讨论方案，直到确认方案是适合的
3. 使用 Agent Plan Mode 让其给出大知道的代码，开发者进行第一步审查
4. 确认没问题后，Agent 开始修改代码。
5. 开发者对代码进行第 2 次审查
6. 如果没问题合并代码，如果有问题则配合 AI 或其他开发者讨论并回到第 3 步

我知道这种模式非常的低效，完全匹配不了 AI 的编码速度，但只有这样 **我** 才能写出我满意的程序，我们需要工匠精神。

我们需要明确高度约束的是代码质量，系统一致性，架构一致性，如果当代码变得 人类无法维护 则说明已经被 AI 写烂了需要回退代码重构。哈嗯，你也想被 AI 一次 commit `100'000` 行代码，一天上千次 commit 的 AI Slop 淹没吗？


## OTHER
现在初步基本由我一个人完成对 VulcalEngine 的全部工作，并且不支持任何人添加 Pull Request ，如果你想指出一些问题应该直接联系作者，至于联系方式嘛，既然你都能看到这个仓库了，还会没有作者的联系方式？作者将会在业余时间开发，哈哈，其实作者更本没有工作，大三在读学生，应该说作者所有的时间都是业余时间，但是作者除了 VulcalEngine 要开发外，还有更多的其他的事情要做，作者只会在觉得这个时间段是开发 VulcalEngine 合适的时间段进行开发。

## ABOUT
采用 MIT 开源证书，本引擎能随意修改并作为商用，并且引擎开发的游戏也能随意商用，提供最高的自由度。

