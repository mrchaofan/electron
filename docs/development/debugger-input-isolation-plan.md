# Debugger 输入与宿主焦点隔离

更新：2026-11-23。键盘隔离依赖输入来源；Input.dispatchMouseEvent 的鼠标点击按照焦点模拟状态决定是否交接真实焦点。

## 契约

- Debugger 键盘、文字和 composition 使用 session 指定页面或 frame。
- Input.dispatchMouseEvent 的鼠标点击：关闭焦点模拟时交接真实焦点，开启时保留宿主焦点，与受测 Codex 框架一致。
- Debugger 滚轮、直接触摸和合成手势保持既有来源处理；本次调整仅涉及上述鼠标注入入口。
- 非 Debugger 来源保持原有聚焦与事件处理。
- 开启 debugger 或焦点模拟不改变真人输入的策略；判断必须来自当前事件。

此契约限制浏览器附加的输入聚焦行为。网页脚本显式 window.focus、弹窗、原生菜单或 OS 拖放是独立请求，不据此承诺全部被隔离。

## 来源标记

Chromium 的 CDP 输入已有 WebInputEvent::kFromDebugger。合成手势也已有 from_devtools_debugger，并在生成事件时设置来源。
新增 UI EventFlags 的 EF_FROM_DEBUGGER，只用于在 WebInputEvent 与 UI event flags 间保留同一来源。它与 EF_IS_SYNTHESIZED 不同，普通合成输入不能因此成为 Debugger 输入。

Web → UI → Web 的转换保留该标记，所以鼠标转触摸、手势识别和异步 ACK 不需要全局“最近事件来源”状态。Mojo 的键盘 flags 验证单独允许新来源位，其余原有合法性限制保持。

## 聚焦入口

1. CDP InputInjector 的鼠标入口读取当前 session 的既有 EmulationHandler 状态；同一 WebContents 的页面 session 可为子 frame session 提供模拟状态。关闭模拟时调用普通 RWH::Focus，并在 MouseDown 调用现有 FocusOwningWebContents；开启模拟时跳过。原有弱引用检查防止聚焦回调销毁目标后继续注入。滚轮与直接触摸入口不改变。
2. RWH::OnInputEventPreDispatch：Debugger MouseDown、TouchStart、GestureTap 不调用 FocusOwningWebContents。Debugger 未消费的鼠标 ACK 不交给浏览器处理，避免 Back/Forward 回退导航 outer。
3. ChildFrame 的触摸预处理：Debugger TouchStart 不调用 FocusRootView。
4. macOS 合成触摸板 pinch：带标记时将原 Web 事件交给现有 PinchEvent，避免重建 NSEvent 丢失来源。非 Debugger 沿用 Cocoa 路径。
5. Aura 事件入口：按 UI 来源标记跳过 SetKeyboardFocus；不向父宿主转交鼠标事件，不改变真人的鼠标捕获、光标位置缓存或光标回中，也不结束真人 IME composition。

注入器只等待带 Debugger 标记且有对应键盘/鼠标 ACK 的事件类型；鼠标转触摸后不等待不存在的 Mouse ACK，普通事件不改变 Debugger 请求的等待状态。

事件后续仍正常发送。Blink 选择网页内部编辑元素，FrameTree 保留原有局部焦点同步；普通 GetFocusedFrameTree/SetFocusedFrameTree 的 inner/outer 规则保持。
没有输入策略管理器、焦点租约、全局开关或焦点保存/恢复。

## 键盘与文字

沿用同一补丁中的键盘实现：InputHandler 的 ResolveKeyboardWidget 从 session 自己的 FrameTree 选择 frame，失效目标失败，不回退宿主。
Debugger 按键使用已有 skip_if_unhandled，跳过浏览器快捷键和未处理回退；不读写真人键盘抑制状态，不改变触摸模拟器的键盘状态。
文字和 composition 的现有 Mojo 消息携带单次 allow_unfocused；Debugger 传 true，普通 IME 传 false，不修改真实 Widget 焦点。

## 坐标与生命周期

CDP 合成手势使用根视图执行，但其命令坐标属于 session 的页面。入队前将 guest/frame 坐标转换为根视图坐标，以免嵌入偏移导致点击宿主；不改变非 CDP 手势坐标。
事件来源与坐标目标是两种信息。来源标记不代替目标检查，也不提供已派发 IPC 的撤回能力。
继续沿用既有弱引用与事件队列；脱离、导航和关闭后的输入应按现有目标有效性检查失败或终止，不能以宿主作为替代目标。

## 使用

普通用户切页调用标签 focus 交接真人焦点。Agent 使用 guest 的 debugger；若用 Input.dispatchMouseEvent 点击并需要保留宿主焦点，先开启 Emulation.setFocusEmulationEnabled。仅发送键盘、文字和 composition 时不必开启模拟。
非 Debugger 的 sendInputEvent 保持普通输入行为，不通过 attach debugger 自动切换策略。

## 验证与边界

新增回归包含模拟开/关、多个 guest 点击、滚轮、鼠标转触摸、直接触摸、合成鼠标/触摸、OOPIF 和非 Debugger 点击对照。
来源转换测试包含 Web/UI 往返、Mojo 序列化以及 Debugger 鼠标转触摸/手势之后再输入普通鼠标，确认普通序列不继承来源。

调整后 macOS arm64 完整构建成功。82 项集成用例分组全部覆盖通过：69 项不依赖窗口激活的用例，以及 13 项真实焦点用例；另有 8 项普通键盘回归通过。13 项焦点用例包含普通 frame/OOPIF 的模拟开关和真实焦点选路。

焦点测试先用普通宿主鼠标输入建立实际焦点，再检查共享 focusedFrame、DOM 编辑目标及页面焦点。sendInputEvent 直接注入指定 WebContents 的 widget，不能当作系统键盘的焦点选路；测试不修改该 API。锁屏或用户键鼠抢占导致的失败没有计为通过，已在解锁和键鼠空闲时重跑成功。上述普通 API 及焦点选路证据仍不等同于系统 IME 实测。

合成点击与 OOPIF 测试曾在仅等待 DOM 就绪后出现点击未送达。测试现在先等待 Page.captureScreenshot 完成，再检查点击路由；这是渲染前置条件，未修改点击、编辑目标或宿主焦点断言，也未加入固定延迟或点击重试。
这证明渲染后的受测路径正常，不据此承诺在内容尚未渲染时输入也能命中。业务无需为隔离功能逐次截图。

3 个新增 C++ 测试源使用实际编译参数的语法检查通过；未运行完整 Chromium 单元测试目标。
JS 与 Markdown lint、补丁正向和反向应用检查通过，补丁与当前 27 个原生源文件一致。
Windows Aura 路径已接入来源判断，但尚未进行 Windows 构建与实机验证；真实系统 IME、原生菜单、OS 拖放与 pointer lock 需按实际业务单独验收。

Windows 通用期望契约见 [输入范围约束](debugger-input-windows-validation.md)。
