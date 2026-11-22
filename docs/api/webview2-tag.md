# `<webview2>` Tag

Embeds an independently owned WebContents in an iframe. Enable `webview2Tag`
in the host window's webPreferences. Create the contents in the main process:

```js
const { BrowserWindow, webContents } = require('electron')
const contents = webContents.create({ type: 'webview2' })
const host = new BrowserWindow({ webPreferences: { webview2Tag: true } })
contents.loadURL('https://example.com')
```

The host document uses `<webview2 web-contents-id="42"></webview2>` with the
actual `contents.id`. The tag has no WebContents method proxies. Keep navigation, scripting, CSS,
messages, input, editing, search and other WebContents controls in the main
process. Use `contents.id` there; the attached event also identifies the
contents without adding a tag getter.

Listen for content events such as `dom-ready`, `did-fail-load` and
`render-process-gone` on WebContents in the main process. An existing document
may already be ready before attachment; reattachment does not produce another
`dom-ready`. Attachment and page readiness have separate lifecycles.

`attached` and `detached` are DOM CustomEvents. Their `detail` contains
`webContentsId` and a unique `attachmentId`. Each attachment creates a new empty
iframe, reads its LocalFrameToken, and attaches through synchronous IPC. `attached` means the browser embedding
relationship exists, rather than that the first compositor frame has appeared.
Main-process detachment notifications are delivered asynchronously.

On a connected element, setting a valid `web-contents-id` triggers `attached`
synchronously. An invalid, missing, destroyed, already attached, or unsupported
WebContents triggers an `error` DOM CustomEvent synchronously instead. Setting
an ID before connecting the element attempts attachment when it is connected.
Removing the attribute leaves the element empty without an error.

The `error` event is emitted only by attachment attempts. Its `detail` contains
`webContentsId`, `code`, and `message`. `webContentsId` is `null` for a
malformed ID. The error codes are `invalid-id` for malformed IDs and
`attach-failed` for failed attachment attempts. After a failed attachment,
setting a valid ID or reconnecting the element retries attachment.

Every attachment ends with exactly one `detached` event, whether the element is
removed, `contents.detach()` is called, the host document is replaced, or the
contents are destroyed.

Removing the element, navigating or closing its host, and calling
`contents.detach()` do not destroy the contents. A host navigation detaches
once the old document stops being active, so navigations that do not commit
leave the element attached. `contents.destroy()` does not
remove the element. Detach before attaching the same contents in another window.
The independent contents remain alive until explicitly destroyed or app shutdown,
including after JavaScript references have been collected.

After detachment the element stays empty and does not automatically reclaim the
contents. Reinsert it, or remove and reset `web-contents-id`, to attach again.
The contents retain their own session, which may differ from the host's session.

This API uses Chromium's experimental unowned inner WebContents implementation.

See [`contents.attach(owner, frameToken)`](web-contents.md#contentsattachowner-frametoken)
for attachment, user focus, tab switching and debugger input examples.

## Focus and background input

`element.focus()` delegates to the attached iframe's window. Calling it on an
empty or detached element has no effect. `element.blur()` uses the DOM blur
behavior. The element emits `focus` and `blur` when focus enters or leaves its
embedded page in the host DOM. Background focus emulation can make the guest
document report focus while the tag remains unfocused. Moving focus away retains the page's active element and selection.

Keep contents alive when switching tabs and explicitly focus the selected
one when the user intends to enter it. Applications should check their current
editor focus again before executing a delayed automatic focus request.

Debugger keyboard, text and composition commands target the session's page or
explicit frame independently of the host's real keyboard focus. A page session
uses its own focused frame, falling back to its valid main frame when none is
focused. These commands do not call ordinary focus transfer. Debugger keys skip
browser accelerator handling and unhandled-key forwarding, including native key
codes. Ordinary user input and main-process `sendInputEvent` keep their existing
behavior. `Input.dispatchMouseEvent` mouse clicks transfer real focus when
page focus emulation is disabled, and preserve host focus when it is enabled.
Debugger wheel, direct touch and synthetic gestures retain their source-based
focus handling. UI event conversions retain
that marker, including mouse-to-touch-to-gesture conversion. Ordinary input
keeps the original focus behavior.

Use `Emulation.setFocusEmulationEnabled` independently when automation needs the
page to report focus or to prevent `Input.dispatchMouseEvent` mouse clicks
from taking host focus. Keyboard isolation neither enables nor depends on focus
emulation. Select the intended editable DOM element before inserting text;
selecting a frame alone does not select an input. Focus emulation keeps its
existing implementation and lifecycle; keyboard input adds no page-focus lease.
Debugger text and composition messages explicitly allow the target widget to
receive input without real widget focus; ordinary IME retains its focus check.

Keyboard targets are resolved and checked when the command is dispatched. A
stale or inactive target fails instead of falling back to the host. Existing IPC
and debugger callbacks complete through their original channels; no keyboard
result is forwarded to a new tag attachment. Page-script side effects and native
focus traversal remain governed by Chromium's normal page behavior.
