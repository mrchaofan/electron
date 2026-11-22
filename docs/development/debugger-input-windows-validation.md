# Debugger 焦点模拟下的输入范围约束

目标：在 Windows 和 macOS 实现以下输入处理契约，使 Debugger 在操控嵌入网页时不会借用宿主的焦点或输入通道，同时保持普通用户输入的原有行为。本规范定义期望行为，不代表两个平台均已通过验证。

设：

- `s` 为 Debugger session，`W = Target(s)` 为其连接的 inner WebContents。
- `D(e, s)` 表示鼠标或键盘输入事件 `e`（含文字提交和 composition） 由 session `s` 提交；仅连接 Debugger 不满足此条件。
- `F(s)` 表示适用于该 session 目标的有效焦点模拟已开启。根 session 的焦点模拟仅能在同一个 WebContents 内继承。
- `I(e)` 表示 `e` 为普通鼠标悬停或左键序列、`Input.dispatchKeyEvent`、`Input.insertText` 或 `Input.imeSetComposition`。普通左键产生的 DOM `click` 在范围内。滚轮、其他鼠标键、鼠标转触摸、触摸、合成手势和显式拖放沿用原有路径。
- `Frames(W)` 为 W 自身 FrameTree 中的 frame 集合，包括同树内的普通 iframe 和 OOPIF，不包括 outer 或其他 WebContents 的 FrameTree。
- `P₀(e, σ)` 为状态 `σ` 下、未应用本次隔离约束时的既有事件处理逻辑。

## 条件契约

**仅当 W 已嵌入，且在接受命令时满足 `D(e, s) ∧ F(s) ∧ I(e)`，本条命令才进入隔离路径。**

1. 输入目标的解析与派发仅允许作用于 `Frames(W)`。焦点查询不得沿 outer WebContents 查找，也不得借用 outer 的获焦树、frame 或 widget。
2. 不得将事件直接转发、回退或重新选路至 W 之外。无法得到有效目标时终止或返回错误，不得以 outer 作为替代目标。
3. 允许改变 W 内部的 DOM 编辑焦点和树内 frame 焦点。W 之外的真实焦点、真人键盘及系统 IME 归属不得因该原生输入处理而改变。
4. 隔离左键序列支持按住移动和抬起，但不得启动系统拖放或跨 WebContents 拖放。目标失效时，只在 W 内清理已开始的序列。

隔离的 Tab / Shift+Tab 默认遍历限于当前渲染进程内的 frame，不走跨进程焦点接力或浏览器焦点交接。位于同一 inner FrameTree 中的 OOPIF 仍可通过命中接收鼠标事件，并在被选为目标后接收键盘和编辑输入。

**当焦点模拟未开启、W 未嵌入，或命令不属于隔离输入时：**

```text
P(e, σ) = P₀(e, σ)
```

这些命令保留原有的选路、聚焦及未处理事件回退，因此可能到达 outer 或改变 outer 焦点。

**当事件不是 Debugger 来源时，无论模拟状态如何：**

```text
(¬∃s: D(e, s)) ⇒ P(e, σ) = P₀(e, σ)
```

普通用户输入不得因为 Debugger 已连接、焦点模拟已开启或此前处理过 Debugger 事件而改变处理逻辑。

上述作用范围约束针对原生输入的目标选择、派发、焦点交接和未处理回退；DOM 脚本调用不自动等同于 Debugger 原生输入事件。
