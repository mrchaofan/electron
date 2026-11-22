# webview2 的职责与事件边界

更新：2026-11-22。当前实现与验证状态见 [焦点修改方案](webview2-codex-focus-modification-plan.md)。

## 控制留在主进程

webview2 是独立 WebContents 的显示与附着容器。创建、配置、导航、脚本、输入、编辑等操作由主进程的 WebContents 完成，标签不增加对应的方法或属性代理。

因此，官方 webview 方法没有出现在 webview2 标签上，并不表示底层能力缺失，也不构成补齐标签 API 的任务清单。需要审查的是主进程能力是否可用、目标 frame 是否正确，以及应用是否需要向宿主界面发送通知。

| 能力 | 调用位置与处理方式 |
| --- | --- |
| 标识 | 主进程使用 `contents.id`；标签的 `attached.detail.webContentsId` 表示成功附着的实例，不增加 `getWebContentsId()` |
| 创建与配置 | 主进程 `webContents.create`、session、WebPreferences、窗口打开策略 |
| 导航与历史 | 主进程 `loadURL`、加载状态、`navigationHistory` |
| 脚本与 CSS | 主进程 `executeJavaScript`、`insertCSS`、`removeInsertedCSS` |
| 消息 | 主进程 `send`、`sendToFrame`，以及主进程 IPC 监听；应用按自己的业务协议通信 |
| 输入、文本与编辑 | 主进程输入与编辑接口；后台自动化使用目标 contents 的 debugger 输入。目标 frame 和焦点交接分别验证 |
| 查找 | 主进程 `findInPage`、`stopFindInPage`，监听 `found-in-page` |
| 缩放 | 主进程缩放接口；单独核对当前底层版本支持 |
| 输出 | 主进程截图、打印、PDF 接口 |
| 调试、音频与下载 | 主进程 WebContents 和 session 接口 |
| 平台专属能力 | 主进程调用，并核对对应平台支持 |

这些接口的参数、返回值与异步行为沿用 WebContents。无需增加一层 webview 方法代理，也无需把主进程同步接口改造成标签的 Promise 接口。

上游新增的 `zoomMode` 仍是底层版本兼容性审查项。本地方法绑定未确认支持时，不能靠增加标签代理实现它。

## 标签保留 DOM 职责

标签负责附着、脱离、附着失败通知，以及 DOM 焦点行为。

`element.focus()` 委托给当前内部 iframe 的 `contentWindow.focus()`。这是 DOM 焦点入口：本地主进程 `WebContents::Focus()` 在已附着状态下直接返回，不能用它替代内部 frame 的焦点交接。

`element.blur()` 复用 HTMLElement 的原生行为。Blink 的 `Element::blur()` 使用所在 TreeScope 的调整后焦点元素判断，包括 shadow host；实际 guest 编辑状态及多标签行为仍需要运行验证。

标签的 `focus/blur` 表示宿主 DOM 焦点进入或离开该标签。guest 的 `document.hasFocus()` 在后台焦点模拟时可以为 true，此时宿主仍可能聚焦聊天框。两种状态分别观察。

## 内容事件来自主进程

主进程通过 WebContents 监听 `dom-ready`、`did-fail-load`、`render-process-gone`、导航、查找及其他内容事件。主进程自身能够处理的事件，直接在那里处理。

宿主界面确实需要通知时，由主进程转发所需事件。转发是通知适配，不要求同时增加同名标签方法，也不要求复制官方所有事件。

官方 `guest-view-manager.ts` 的普通事件路径是：

```text
WebContents 事件 → 主进程提取事件参数 → IPC → 宿主派发 DOM 事件
```

官方焦点通知还有一条特殊路径：guest 渲染进程通知主进程，主进程转发 `-focus-change`，宿主再按 `document.activeElement` 判断标签焦点。因此不能直接把 guest 的焦点模拟状态当作标签焦点。

当前 webview2 已有 `attached`、`detached`、`error` 及 DOM 焦点通知；本次没有新增通用 guest 内容事件转发器。

## 附着与内容就绪分开

- `attached`：当前嵌入关系建立。
- `detached`：当前嵌入关系结束；独立 contents 继续存在。
- `error`：附着失败。
- `dom-ready`：对应页面文档就绪，来源是 WebContents。
- `did-fail-load`：页面加载失败。
- `render-process-gone`：渲染进程退出或故障。

contents 可以在附着前加载完成，也可以脱离标签后继续导航。重新附着不会重新产生一次页面 `dom-ready`。应用需要分别维护页面文档状态与显示关系，不从 `attached` 推导内容就绪，也不把附着错误转换成加载错误。

## 转发只服务当前附着关系

若增加事件转发，复用现有原生 attachment 状态作为路由依据，校验当前 `attachmentId`、宿主 WebContents 与宿主 frame/document。异步发送完成前和宿主接收时都检查对应关系。

旧 attachment 的通知及异步结果不得投递到新 attachment。脱离或宿主导航时解除该显示关系的订阅；contents 所有者在主进程的监听可以继续存在。

复用官方事件名称和参数映射时，不复用旧 guest-view-manager 的生命周期所有权。事件转发器不创建或销毁 contents，不因标签移除而销毁独立页面，也不另外维护一套与原生 bridge 竞争的附着状态。

## 实施与验证顺序

1. **P0：验证 DOM 焦点与输入。** focus 委托、原生 blur、可靠焦点通知、Tab / Shift+Tab；观察 guest `document.hasFocus()`、宿主 `activeElement`、真实键盘和输入法。覆盖多个标签、后台窗口、断开与重新附着，以及 sandbox / contextIsolation。
2. **P1：验证主进程能力与必要通知。** 页面内容就绪、加载失败、进程退出；脚本、CSS、消息、输入、编辑、查找、导航历史和缩放保持主进程调用。只为宿主界面实际需要的事件增加转发。
3. **P2：核对主进程版本和平台能力。** 输出、DevTools、音频、下载与平台专属接口；`zoomMode` 单独核对底层支持。

`api-webview2.spec.ts` 已加入专门的焦点、CDP 键盘/文本/组合输入、多页面、跨进程 iframe、多调试会话和附着变化用例。完整构建和该文件 47 项运行回归已通过；现有键盘选路测试不替代真实系统键盘和输入法测试。Tab / Shift+Tab、完整 sandbox 组合及内容事件转发还需进一步验证。

## 依据

- [官方 webview 文档](https://www.electronjs.org/docs/latest/api/webview-tag)
- 本地 `lib/browser/guest-view-manager.ts`：WebContents 事件转发和焦点通知中转。
- 本地 `lib/renderer/web-view/web-view-impl.ts`：DOM 焦点判断及内部 iframe 的 focus 委托。
- 本地 `lib/browser/webview2-manager.ts`、`webview2-internal.ts`、`webview2-element.ts`：独立附着与当前标签行为。
- 本地 `shell/browser/api/electron_api_web_contents.cc`：主进程能力及已附着 contents 的 Focus 行为。
- Chromium `Element::blur()`：按 TreeScope 调整后焦点元素处理原生 blur。
