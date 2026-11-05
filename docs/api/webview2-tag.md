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
actual `contents.id`. The tag has no WebContents method proxies.

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
