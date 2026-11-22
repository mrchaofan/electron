# Debugger 输入来源与焦点

更新：2026-11-23。

Debugger 键盘输入始终定向隔离。Input.dispatchMouseEvent 的鼠标点击与受测 Codex 一致：关闭焦点模拟时交接真实焦点，开启时保留宿主焦点。

实现复用 kFromDebugger 和合成手势的 from_devtools_debugger，通过 UI 的 EF_FROM_DEBUGGER 保留转换后的来源。在 InputInjector、RWH 预派发、子 frame 触摸入口与 Aura 入口按当前事件标记决定是否请求真实聚焦；非 Debugger 保持原逻辑。

键盘局部选路、skip_if_unhandled 和单次定向 IME 参数沿用原实现。CDP 合成手势坐标在入队前转换到根视图坐标。

不新增输入上下文、全局状态、焦点租约或焦点恢复。补丁为 patches/chromium/feat_debugger_input_isolation.patch。

使用方式：用户交接焦点调用 webview2.focus；Agent 使用 guest.debugger，在使用 Input.dispatchMouseEvent 点击并要求保留宿主焦点时开启页面焦点模拟。

设计和验证见 [输入隔离方案](debugger-input-isolation-plan.md)。Codex 原生求证记录见 [技术附件](webview2-codex-focus-technical-notes.md)。
