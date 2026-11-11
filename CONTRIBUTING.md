# 为 VulcalEngine 贡献代码

感谢你的贡献！

## 开始之前

在开启新讨论前，请先搜索现有的 issue 和讨论，确保你想贡献的内容没有被覆盖。

## 范围约束

- 请确保改动集中在某一个具体的功能上，尽量不要跨模块修改，除非该问题非常严重。
- 禁止任何重构性质的修改，除非你是核心开发者。
- 单个 PR 禁止修改超过 5000 行代码，这会大大增加代码审查的难度。对于这种规模的 PR，仓库维护者一般会直接拒绝合并。
  - 行数以 `git diff --stat` 的总变更行数（新增 + 删除）为准。
  - 自动生成的文件与第三方代码不计入。

### AI 的使用

我们的项目并不禁止使用 AI，但我们禁止 vibe coding。你必须先弄清楚自己修改的代码，并且需要对此负责。

## 命名规范

- **文件与目录**：采用小写加下划线的 Snake Case，特殊文件除外（如 `README.md`、`CMakeLists.txt`）。
- **自定义类型**（`class`、`struct`、`enum` 等）：采用 PascalCase，如 `RenderPass`、`VertexBuffer`。
- **函数**：采用 Snake Case。
- **成员变量**：采用 Snake Case，并在末尾额外添加一个 `_`，如 `vertex_count_`。
- **专有名词与缩写**：全部大写，如 `AABB`、`UUID`。
- 灵活使用合适的命名方法，在特殊位置（如构造函数、运算符重载、宏等）可不遵守上述规则。

## Commit 规范

使用标准的 [Angular 规范](https://github.com/angular/angular/blob/main/contributing-docs/commit-message-guidelines.md)。

请确保每一个 Commit 只代表一个意图，即尽可能保证单个 Commit 的原子性，并清晰地描述其目的。

示例：

```
feat(renderer): add AABB frustum culling
fix(core): prevent double free in Buffer destructor
docs(contributing): clarify commit message rules
```

## 代码注释

应该使用简单易懂且**简短**的英文描述。

## 提交 Issue

我们的 issue 规范很简单：只要你能把问题说清楚就行。不限语言，什么语言都行，只要是 AI 能翻译的。

为便于定位问题，建议至少包含：

- 环境与版本（OS、编译器、VulcalEngine 版本或 commit）
- 复现步骤
- 期望结果与实际结果

对于让维护者看不懂的 issue，通常会和提出者进一步讨论。

## 提交 PR

我们的 PR 规范很简单：只要你能把你做了哪些修改说清楚就行。不限语言，什么语言都行，只要是 AI 能翻译的。

我们会审查你的 PR，**符合以下条件即可合并**：

- 遵守本文件中的全部规范（范围、命名、Commit、注释）
- 通过 CI（构建与测试）
- 审查中无阻塞性意见


