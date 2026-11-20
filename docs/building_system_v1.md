# Building System v1

Editor 保存构建配置并启动 `vulcal_build`；后者生成 ZIP 格式的 `.vpak` 并复制 Player。
Player 通过资源 Provider 按逻辑路径读取场景、cooked 资产与 Lua，在内存中解压单个 ZIP 条目。

## 使用

打开 `.vproj` 后，通过 **File → Build Settings...** 选择项目资源目录内现有的 `.vscene`。
输出目录可留空，默认使用项目文件旁的 `build/`；手动选择的目录保存为绝对路径。
点击 **Save** 将配置写回项目，然后使用 **File → Build**。
Build 会阻塞等待 CLI 结束，非零退出码会显示为编辑器操作错误。
构建使用已经保存的场景、脚本与 cooked 文件；构建前应先保存编辑中的场景。

配置示例：

```yaml
name: snake
description: ""
author: ""
assets_path: assets
last_edited_scene: assets/scenes/snake.vscene
build_info:
  open_scene: assets/scenes/snake.vscene
  # build_dir: C:/Games/snake
```

旧项目没有 `build_info` 时仍可打开。
`open_scene` 相对于 `.vproj`，必填；`build_dir` 为可选绝对路径。
输出不能放在 `.vulcal_engine/` 内。

以 Windows Ninja 构建目录为例：

```powershell
cmake --build build --target editor vulcal_build vulcal_player --parallel
build/bin/vulcal_build.exe projects/snake/snake.vproj build/bin/vulcal_player.exe
projects/snake/build/vulcal_player.exe
# 也可显式选择包：
build/bin/vulcal_player.exe projects/snake/build/snake.vpak
```

Linux 使用相同参数，并去掉 `.exe`；Visual Studio 多配置构建的程序位于 `build/bin/<配置>/`。
Editor 传给 Build CLI 的两个路径都是绝对路径，通过 SDL 参数数组启动，不拼接 shell 命令。
Player 显式参数优先；没有参数时要求可执行文件目录中恰好有一个 `.vpak`，零个或多个均报错。

## 包内容与接口

打包内容保留项目相对路径：

- `.vulcal_engine/` 下的普通文件。
- `assets_path` 下的 `.vscene` 与 `.lua`。
- 项目内其他目录的 `.lua`，用于兼容 Editor 已有的项目相对脚本赋值；跳过 `.git/` 与已扫描的资源目录。
- 根目录的 `vulcal_game.vmanifest`。

原始 OBJ、图片、`.vmat` 和导入元数据不作为运行资产收进包。
构建不重新导入资产，也不检查场景或脚本所引用资源的完整性。
构建器遇到扫描范围内的 symlink 会明确失败，避免静默漏包。

```yaml
version: 1
engine_version: 0.0.5
name: snake
open_scene: assets/scenes/snake.vscene
```

逻辑路径是 UTF-8、项目或 Provider 根目录相对路径、使用 `/`，并且区分大小写。
DirectoryProvider 和 PakProvider 均拒绝绝对路径、控制字符、空路径组件以及 `.`、`..`。
DirectoryProvider 校验真实文件仍位于根目录内；PakProvider 拒绝损坏 ZIP、危险或重复条目。
文件扩展名识别忽略大小写，但引用中的完整路径必须匹配实际名称。

`SceneSerializer` 位于 `vulcal_engine/scene`。
SceneSerializer、AssetLibrary/AssetManager 与 ScriptRuntime 支持 Provider；三个 cooked 解码器支持字节数据。
旧文件路径入口保留为目录读取包装。

产物先写临时文件，再备份旧包与旧 Player 并依次替换；失败时尝试恢复备份。
拒绝目录或 symlink 输出、源 Player 与输出指向同一文件、输出之间的大小写或硬链接别名。
这是进程内错误回滚，不提供断电或进程被强制结束时的事务恢复。

## 原文核对与首版边界

依据：[Design Building System](https://virtual-graphics.github.io/VulcalEngine-website/blog/2026/11/18/build/main/)。
以下是实现所用解释、补充及尚未覆盖的发布工作：

| 项目 | 当前行为与原因 |
| --- | --- |
| 项目字段 | 使用仓库实际已有的 `assets_path`；文章交替使用 `asset_dir`、`assets_dir` 等名称。 |
| manifest 版本 | 格式版本定义为整数 `1`，引擎版本使用当前项目的 `0.0.5`；文章示例为 `1.0` 和 `0.1.0`。 |
| 包布局 | 包根直接存放 `.vulcal_engine/`、`assets/` 和 manifest；文章树的 `vulcal_game/` 理解为项目示意名。 |
| 归档阶段 | 直接把源文件写入临时 ZIP，不先在输出目录生成完整散文件副本。 |
| 项目外层脚本 | 额外收集资源目录外、仍位于项目内的 Lua，保留 Editor 现有行为；文章仅描述标准资源目录。 |
| 自动选包 | 多个包时要求显式指定；文章只明确了零个包失败与显式参数优先。 |
| 更严格的路径 | 明确大小写匹配、拒绝 symlink 构建输入及 cooked 目录内输出；文章未定义这些边界。 |
| Editor 交互 | Build 入口为 File 菜单项，缺少配置时提示先打开 Build Settings；没有构建进度或取消界面。 |
| 引用检查 | 按文章首版范围暂不做资源引用完整性、加密、场景/Lua 二进制化和跨平台构建。 |
| Native C++ | 开发者自行编译与发布。 |
| 依赖分发 | 当前只复制 Player 可执行文件，没有自动收集 DLL 或系统运行库。只有静态链接所需依赖或目标机已安装依赖时，才能只分发 `.vpak` 与 Player 两个文件。 |
| SDK 分发 | 新增了本地 CMake 程序目标；尚未增加编辑器下载包、安装器或发布 CI。 |

当前 Windows 开发环境的 SDL 是动态库，Debug 构建还依赖 Debug C++ 运行库。
本机启动成功不代表这些开发产物能直接拷到一台干净的电脑发布。
发布应使用 Release 构建，并配置静态依赖或补齐对应运行依赖。

## 实施中修复的问题

- 资源读取从路径迁移到 Provider 后，无项目的默认场景无法 Play：保留无脚本场景不需要目录的原有行为。
- `tests/build/` 被仓库 `build/` 忽略规则覆盖：改为 `tests/build_pipeline/`，使干净检出能获得测试源码。
- Windows 的本地编码与 UTF-8 混用：新增统一路径转换，配置通过原生路径文件流读写，CLI 使用 SDL 的 UTF-8 参数入口。
- Windows 目录忽略大小写而 ZIP 条目精确匹配：两个 Provider 统一精确匹配，避免 Editor 正常、Player 丢脚本。
- 非法名称、目录输出、symlink 与硬链接别名可能覆盖或移动旧文件：发布前拒绝这些目标，并验证实际文件锁失败的恢复行为。
- 复制 Player 时缓冲写入失败和 Unix 执行权限可能丢失：改用检查错误码的文件复制。
- 输出目录与项目脚本目录重叠导致漏包：脚本扫描保留该目录；输出位于 cooked 目录时直接拒绝。
- Editor 接受大写场景扩展名而 CLI 拒绝：统一扩展名识别，完整逻辑路径仍精确匹配。

## 验证

```powershell
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

测试覆盖目录/ZIP 读取、路径边界、字节资产解码、场景和 Lua 从包中执行、Unicode 路径、manifest、
构建文件范围、旧产物保留、Windows 文件锁导致发布失败的回滚，以及无项目默认场景的 Play 兼容性。
symlink 测试在 Windows 未启用相关权限时跳过；POSIX 可执行权限测试在 Windows 跳过。

提交 PR 前的 Windows / clang-cl Debug 验证：全量目标构建成功。
完整 CTest 运行 279 项，267 通过、6 跳过，6 项现有视口测试在窗口输入焦点断言处失败；
`ctest --test-dir build --rerun-failed --output-on-failure` 单独复跑 6 项全部通过。
这些测试需要操作系统授予窗口输入焦点，仍存在环境相关的不稳定性。
完整输出保存在本地 `build/ctest-building-system-v1-pr.log` 和 `build/ctest-building-system-v1-pr-rerun.log`。

实际启动验证使用现有 sandbox 项目的中文及 emoji 路径副本，以及 snake 示例。
前者显式指定包，后者无参数自动选包，均进入主循环且无错误日志；
sandbox 日志确认从包内载入 mesh、material 与 texture。
这次验收验证了启动和资源加载，没有完成游戏全过程的人工视觉与输入验收。
