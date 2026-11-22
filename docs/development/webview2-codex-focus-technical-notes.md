# Codex 焦点机制：逆向记录与当前实现范围

更新：2026-11-22。以下保留原始 Codex 求证记录。当前实现隔离 Debugger 键盘、文字和 composition，并按事件来源抑制指针输入的真实焦点交接，沿用 Chromium 原有焦点模拟。当前补丁为 `patches/chromium/feat_debugger_input_isolation.patch`。

Codex 的指针保焦及原生页面焦点处理是逆向记录；当前来源标记实现仍需按自己的验证结果验收。当前设计见 [输入隔离方案](debugger-input-isolation-plan.md)。

## 证据核对

### 检查对象一致

本机 Codex 发行包为 26.1117.31558，框架为 158.0.8127.37。原始框架 SHA-256：

```text
4a74aa3d505432de57848b30065f17dacffcbfeb932fe7d550708874ac7c3f44
```

该值与用户提供的原生记录一致。重新对同一二进制执行 llvm-objdump 后，以下分支均与参考记录对应：

| 机制 | ARM64 VM 地址 | 求证结果 |
| --- | --- | --- |
| 目标局部选路 | 0x445d9e8–0x445da7c | 从当前 session 的 Emulation 后端读 enabled 状态；命中后从目标 contents 自己的 primary tree 查 focused node/widget |
| 普通宿主选路 | 0x445dae4–0x445db0c | 上述条件不命中时，调用 delegate 的普通 focused-widget 选择 |
| 输入焦点 helper | 0x445d160 | 普通分支转到 0x7382028；模拟分支直接调用 0x7382064 |
| SetPageFocus | 0x7382064 | 由 RenderWidgetHostImpl::SetPageFocus trace 字符串及调用形状对应，不能解释为没有任何页面 focus 操作 |
| debugger 点击识别 | 0x445d0bc–0x445d0c0 | 检查 modifiers 对应 bit 23；公开 WebInputEvent::kFromDebugger 为 `1 << 23` |
| 点击交接抑制 | 0x44770f4–0x4477118 | 匹配输入 helper 条件时跳过 delegate 的 FocusOwningWebContents |
| 关闭模拟恢复 | 0x2cb2d78、0x2cb2df0–0x2cb2e14 | 查询关闭时的 focused contents，比较当前 target，再调用 SetPageFocus；不是恢复开始时保存的旧 bool |

内部符号缺失，函数名来自绑定、trace 字符串、调用形状及公开源码对应；地址是 VM 地址，不含 ASLR。字段偏移只作为证据，不作为移植常量。相同版本、匹配分支和实验结果共同支持结论；不能把伪代码称为还原的原始 C++ 源码。

### 普通 tab 的直接 JavaScript 证据

Codex 创建 `<webview webviewrole="tab">`。宿主按 conversationId + browserTabId 保留页面对象，切页时 parkInDom 隐藏；dispose 才销毁。移动已连接容器时优先使用 moveBefore。

隐藏路径不主动 focus；解除展示时只有 document.activeElement 恰好是该 webview 才 blur。重新呈现有效页面时，仅当宿主 activeElement 为 null、body、documentElement 或另一 webview 才自动 focus。聊天框、地址栏等编辑元素持有焦点时不满足此条件。延迟 focus 请求在执行时重查 predicate 与页面身份。

标签 focus 委托 iframe.contentWindow.focus；guest focus/blur 回传保留。普通 native SetFocusedFrame 先更新本 FrameTree 的局部焦点，再按真实输入归属决定是否经 outer/surface connector 交接宿主 focused tree。

### 同版本框架的动态验证

私有测试包使用本机 Codex 框架副本；仅调整自制实验包的打包完整性摘要与签名，原生 __text 段哈希已验证与原始框架一致。安装目录和正在运行的 Codex 未修改。此实验验证原生能力，不替代原始应用代码证据。

宿主隐藏，聊天框 activeElement=chat、value=user、selection=[1,3]；A/B 各有自己的 input。

| 操作 | Codex 158 框架 | 本地 Electron 43.8.1 / Chromium 150 |
| --- | --- | --- |
| 无模拟，分别对 A/B 执行 input.focus | 两页各自保留 activeElement=input；hasFocus=false | 相同行为 |
| setAgentActive(true) | A hasFocus仍false | 无该定制接口 |
| 两页开启模拟 | A/B hasFocus=true，宿主状态不变 | A/B hasFocus=true，宿主状态暂不变 |
| 向 A/B 发字符 A/B | A.value=A，B.value=B，聊天框仍user/[1,3] | guest未收到；聊天框变为uABr，选区改变 |
| 向 B 执行 Input.insertText(C) | B.value=BC，聊天框不变 | 聊天框变为uABCr |
| 向 B 发 debugger mousePressed/released | 宿主activeElement仍chat，没有新增chat blur | 宿主activeElement变WEBVIEW，chat收到blur |
| 关闭 B，再关闭 A 的模拟 | B、A依次恢复hasFocus=false，聊天框不变 | B因此前点击得到真实归属，关闭后仍hasFocus=true |

这组实验直接复现并区分“CDP目标被真实宿主焦点改送”和“debugger点击交接真实焦点”。宿主隐藏，因此它没有验证系统窗口激活、真人键盘或真实 IME。这里“无模拟”的脚本 focus 也不是对整套 Codex 用户界面的动态操作测试。

公开 chromium/main 的已保存输入源码仍有普通 focused-widget 路径；其 EmulationHandler 有新增 widget 状态，但所检查的 input_handler.cc 没有 Codex 的上述局部 Emulation 分支。移植应按确认的行为与调用点实施，不假设简单升级任意 Chromium 版本就自动解决。
