import { BrowserWindow, Menu, session, webContents, webFrameMain } from 'electron/main';

import { afterEach, describe, expect, it } from 'vitest';

import { once } from 'node:events';
import * as http from 'node:http';
import { setTimeout } from 'node:timers/promises';

import { defer, listen } from './lib/spec-helpers.ts';

const windows = new Set<BrowserWindow>();
const contents = new Set<Electron.WebContents>();
async function waitFor(predicate: () => boolean | Promise<boolean>) {
  for (let i = 0; i < 400; ++i) {
    if (await predicate()) return;
    await setTimeout(25);
  }
  throw new Error('webview2 state transition timed out');
}
async function createContents() {
  const page = webContents.create({ type: 'webview2' });
  contents.add(page);
  await page.loadURL('data:text/html,<script>window.persistentMarker=123</script>guest');
  return page;
}
async function createHost(
  preferences: Electron.WebPreferences = {},
  url = 'data:text/html,<body>host<script>window.transitions=[]</script>'
) {
  const window = new BrowserWindow({ show: false, webPreferences: { webview2Tag: true, ...preferences } });
  windows.add(window);
  await window.loadURL(url);
  await waitFor(() => window.webContents.executeJavaScript("!!customElements.get('webview2')"));
  return window;
}
async function attach(window: BrowserWindow, page: Electron.WebContents) {
  await window.webContents.executeJavaScript(`(() => {
    const element = document.createElement('webview2');
    element.setAttribute('web-contents-id', '${page.id}');
    for (const name of ['attached', 'detached', 'error']) element.addEventListener(name,
      event => window.transitions.push({name, ...event.detail}));
    window.view = element;
    document.body.append(element);
  })()`);
  await waitFor(() => page.isAttached());
  await waitFor(() => window.webContents.executeJavaScript("transitions.some(event => event.name === 'attached')"));
}
afterEach(() => {
  for (const window of windows) if (!window.isDestroyed()) window.destroy();
  for (const page of contents) if (!page.isDestroyed()) page.destroy();
  windows.clear();
  contents.clear();
});

describe('webview2 independent lifetime', () => {
  it('emits attached synchronously when a valid id is set on a connected element', async () => {
    const page = await createContents();
    const host = await createHost();
    const events = await host.webContents.executeJavaScript(`(() => {
      const element = document.createElement('webview2');
      const events = [];
      let setting = true;
      for (const type of ['attached', 'detached', 'error']) element.addEventListener(type,
        event => events.push({ type, synchronous: setting, ...event.detail }));
      document.body.append(element);
      element.setAttribute('web-contents-id', '${page.id}');
      setting = false;
      return events;
    })()`);
    expect(events).toHaveLength(1);
    expect(events[0]).toMatchObject({ type: 'attached', synchronous: true, webContentsId: page.id });
    expect(events[0].attachmentId).toBeTruthy();
    expect(page.isAttached()).toBe(true);
  });

  for (const scenario of ['missing', 'destroyed', 'already attached', 'wrong type']) {
    it(`emits error synchronously for a ${scenario} WebContents id and can recover`, async () => {
      const host = await createHost();
      const page = await createContents();
      let id = Number.MAX_SAFE_INTEGER;
      if (scenario === 'destroyed') {
        const destroyed = await createContents();
        id = destroyed.id;
        destroyed.destroy();
        await waitFor(() => destroyed.isDestroyed());
      } else if (scenario === 'already attached') {
        const otherHost = await createHost();
        const occupied = await createContents();
        id = occupied.id;
        await attach(otherHost, occupied);
      } else if (scenario === 'wrong type') {
        id = host.webContents.id;
      }
      const events = await host.webContents.executeJavaScript(`(() => {
        const element = document.createElement('webview2');
        window.view = element;
        window.attempts = [];
        let setting = true;
        for (const type of ['attached', 'detached', 'error']) element.addEventListener(type,
          event => attempts.push({ type, synchronous: setting, ...event.detail }));
        document.body.append(element);
        element.setAttribute('web-contents-id', '${id}');
        setting = false;
        return attempts;
      })()`);
      expect(events).toHaveLength(1);
      expect(events[0]).toMatchObject({ type: 'error', synchronous: true, webContentsId: id, code: 'attach-failed' });
      expect(events[0].message).toBeTruthy();
      expect(page.isAttached()).toBe(false);
      await host.webContents.executeJavaScript(`view.setAttribute('web-contents-id', '${page.id}')`);
      expect(page.isAttached()).toBe(true);
      expect(await host.webContents.executeJavaScript('attempts.map(event => event.type)')).toEqual([
        'error',
        'attached'
      ]);
      if (scenario === 'already attached') {
        expect(webContents.fromId(id)?.isAttached()).toBe(true);
      }
    });
  }

  for (const value of ['', 'not-an-id', '0', '-1', '1.5', '9007199254740992']) {
    it(`emits error for malformed web-contents-id ${JSON.stringify(value)}`, async () => {
      const host = await createHost();
      const events = await host.webContents.executeJavaScript(`(() => {
        const element = document.createElement('webview2');
        const events = [];
        for (const type of ['attached', 'detached', 'error']) element.addEventListener(type,
          event => events.push({ type, ...event.detail }));
        document.body.append(element);
        element.setAttribute('web-contents-id', ${JSON.stringify(value)});
        return events;
      })()`);
      expect(events).toEqual([
        { type: 'error', webContentsId: null, code: 'invalid-id', message: 'Invalid web-contents-id' }
      ]);
    });
  }

  it('makes one attempt when upgrading a parsed element', async () => {
    const host = await createHost(
      {},
      `data:text/html,<body><webview2 web-contents-id="${Number.MAX_SAFE_INTEGER}"></webview2><script>
      window.attempts = [];
      for (const type of ['attached', 'detached', 'error']) document.querySelector('webview2').addEventListener(type,
        event => attempts.push({ type, ...event.detail }));
    </script>`
    );
    expect(await host.webContents.executeJavaScript('attempts.map(event => event.code)')).toEqual(['attach-failed']);
  });

  it('stays attached when a host navigation does not commit', async () => {
    const server = http.createServer((_request, response) => {
      response.statusCode = 204;
      response.end();
    });
    defer(() => {
      server.closeAllConnections();
      server.close();
    });
    const { url } = await listen(server);
    const page = await createContents();
    const window = await createHost();
    await attach(window, page);
    await window.webContents.loadURL(url).catch(() => {});
    expect(page.isAttached()).toBe(true);
    expect(await window.webContents.executeJavaScript('transitions.map(event => event.name)')).toEqual(['attached']);
    await window.webContents.executeJavaScript('view.remove()');
    expect(page.isAttached()).toBe(false);
  });

  it('retains its session and isolated storage when transferred between hosts with different sessions', async () => {
    const server = http.createServer((_request, response) => {
      response.setHeader('Content-Type', 'text/html');
      response.end('<body><script>window.transitions=[];window.persistentMarker=123</script>');
    });
    defer(() => {
      server.closeAllConnections();
      server.close();
    });
    const { url } = await listen(server);
    const guestSession = session.fromPartition('webview2-cross-session-guest');
    const page = webContents.create({ type: 'webview2', session: guestSession });
    contents.add(page);
    await page.loadURL(url);
    await page.executeJavaScript("localStorage.setItem('sessionMarker', 'guest')");
    await guestSession.cookies.set({ url, name: 'sessionMarker', value: 'guest' });

    const first = await createHost({}, url);
    const second = await createHost({ partition: 'webview2-cross-session-host' }, url);
    for (const [index, host] of [first, second].entries()) {
      expect(host.webContents.session).not.toBe(guestSession);
      expect(await host.webContents.executeJavaScript("localStorage.getItem('sessionMarker')")).toBeNull();
      expect(await host.webContents.session.cookies.get({ url, name: 'sessionMarker' })).toHaveLength(0);
      await host.webContents.executeJavaScript(`localStorage.setItem('sessionMarker', 'host-${index}')`);
      await attach(host, page);
      expect(page.session).toBe(guestSession);
      expect(await page.executeJavaScript('persistentMarker++')).toBe(123 + index);
      expect(await page.executeJavaScript("localStorage.getItem('sessionMarker')")).toBe('guest');
      expect((await page.session.cookies.get({ url, name: 'sessionMarker' }))[0].value).toBe('guest');
      expect(await host.webContents.executeJavaScript("localStorage.getItem('sessionMarker')")).toBe(`host-${index}`);
      expect(await host.webContents.session.cookies.get({ url, name: 'sessionMarker' })).toHaveLength(0);
      if (index === 0) {
        expect(page.detach()).toBe(true);
        await host.webContents.executeJavaScript('view.remove()');
      } else {
        await page.loadURL(`${url}/after-attachment`);
        expect(await page.executeJavaScript("localStorage.getItem('sessionMarker')")).toBe('guest');
        expect(page.isAttached()).toBe(true);
      }
      host.destroy();
      await waitFor(() => !page.isAttached());
      expect(page.isDestroyed()).toBe(false);
    }
    await page.loadURL(url);
    expect(page.session).toBe(guestSession);
    expect(await page.executeJavaScript("localStorage.getItem('sessionMarker')")).toBe('guest');
  });

  it('loads and runs a page without ever attaching to an iframe', async () => {
    const page = webContents.create({ type: 'webview2', sandbox: true, contextIsolation: true });
    contents.add(page);
    expect(page.getType()).toBe('webview2');
    expect(page.isAttached()).toBe(false);
    expect(page.hostWebContents).toBeNull();
    await page.loadURL(
      'data:text/html,' +
        encodeURIComponent(`
      <title>Unattached page</title><div id="value">pending</div>
      <script>
        window.pageState = { count: 7 };
        document.getElementById('value').textContent = 'page script ran';
      </script>
    `)
    );
    expect(page.isAttached()).toBe(false);
    expect(page.getTitle()).toBe('Unattached page');
    expect(await page.executeJavaScript("document.getElementById('value').textContent")).toBe('page script ran');
    expect(await page.executeJavaScript('Promise.resolve(++pageState.count)')).toBe(8);
    expect(await page.executeJavaScript('pageState.count')).toBe(8);
    expect(page.detach()).toBe(false);
    expect(await page.executeJavaScript('pageState.count')).toBe(8);
    expect(page.isAttached()).toBe(false);
    page.destroy();
    await waitFor(() => page.isDestroyed());
  });

  for (const preferences of [
    { sandbox: true, contextIsolation: true },
    { sandbox: false, contextIsolation: true, nodeIntegration: true },
    { sandbox: false, contextIsolation: false, nodeIntegration: true }
  ]) {
    it(`attaches with ${JSON.stringify(preferences)}`, async () => {
      const page = await createContents();
      const window = await createHost(preferences);
      await attach(window, page);
      expect(await page.executeJavaScript('persistentMarker')).toBe(123);
      expect(page.detach()).toBe(true);
      expect(page.detach()).toBe(false);
      await waitFor(() =>
        window.webContents.executeJavaScript("transitions.filter(event => event.name === 'detached').length === 1")
      );
      expect(
        await window.webContents.executeJavaScript("transitions.filter(event => event.name === 'error')")
      ).toHaveLength(0);
      await window.webContents.executeJavaScript('view.remove()');
      expect(page.isDestroyed()).toBe(false);
      expect(await page.executeJavaScript('persistentMarker')).toBe(123);
    });
  }

  it('preserves contents when the iframe is removed first', async () => {
    const page = await createContents();
    const window = await createHost();
    await attach(window, page);
    await window.webContents.executeJavaScript('view.remove()');
    await waitFor(() => !page.isAttached());
    expect(page.detach()).toBe(false);
    expect(await page.executeJavaScript('persistentMarker')).toBe(123);
    await attach(window, page);
    expect(await page.executeJavaScript('persistentMarker')).toBe(123);
  });

  it('preserves the element when the contents is destroyed first', async () => {
    const page = await createContents();
    const window = await createHost();
    await attach(window, page);
    page.destroy();
    await waitFor(() => page.isDestroyed());
    await waitFor(() => window.webContents.executeJavaScript("transitions.some(event => event.name === 'detached')"));
    expect(await window.webContents.executeJavaScript('transitions.map(event => event.name)')).toEqual([
      'attached',
      'detached'
    ]);
    expect(await window.webContents.executeJavaScript('view.isConnected')).toBe(true);
    const replacement = await createContents();
    await window.webContents.executeJavaScript(`view.setAttribute('web-contents-id', '${replacement.id}')`);
    await waitFor(() => replacement.isAttached());
  });

  it('emits detached once when detached contents are destroyed', async () => {
    const page = await createContents();
    const window = await createHost();
    await attach(window, page);
    page.detach();
    page.destroy();
    await waitFor(() => page.isDestroyed());
    await waitFor(() => window.webContents.executeJavaScript("transitions.some(event => event.name === 'detached')"));
    expect(await window.webContents.executeJavaScript('transitions.map(event => event.name)')).toEqual([
      'attached',
      'detached'
    ]);
  });

  it('emits detached once when the element is removed before the notification arrives', async () => {
    const page = await createContents();
    const window = await createHost();
    await attach(window, page);
    const removed = window.webContents.executeJavaScript(`(() => {
      const end = Date.now() + 500;
      while (Date.now() < end);
      view.remove();
    })()`);
    page.destroy();
    await removed;
    expect(await window.webContents.executeJavaScript('transitions.map(event => event.name)')).toEqual([
      'attached',
      'detached'
    ]);
  });

  it('detaches only the given attachment', async () => {
    const page = await createContents();
    const window = await createHost();
    await attach(window, page);
    const attachmentId = await window.webContents.executeJavaScript('transitions[0].attachmentId');
    expect(page.detach('stale')).toBe(false);
    expect(page.isAttached()).toBe(true);
    expect(page.detach(attachmentId)).toBe(true);
    expect(page.isAttached()).toBe(false);
  });

  it('detaches when a new host document commits and can attach again', async () => {
    const page = await createContents();
    const window = await createHost();
    await attach(window, page);
    await window.loadURL('data:text/html,<body>host<script>window.transitions=[]</script>');
    await waitFor(() => !page.isAttached());
    expect(page.isDestroyed()).toBe(false);
    await attach(window, page);
    expect(await page.executeJavaScript('persistentMarker')).toBe(123);
  });

  it('detaches while the old host document is still running unload handlers', async () => {
    const server = http.createServer((_request, response) => {
      response.setHeader('Content-Type', 'text/html');
      response.end(
        '<body><script>window.transitions=[];' +
          'addEventListener("unload",()=>{const end=Date.now()+5000;while(Date.now()<end);})</script>'
      );
    });
    defer(() => {
      server.closeAllConnections();
      server.close();
    });
    const { port } = await listen(server);
    const page = await createContents();
    const window = await createHost({}, `http://127.0.0.1:${port}/`);
    await attach(window, page);
    const { processId, frameToken } = window.webContents.mainFrame;
    // A cross-site navigation leaves the old document pending deletion, and
    // its iframe alive, until its unload handler finishes or times out.
    const navigated = once(window.webContents, 'did-navigate');
    window.loadURL(`http://localhost:${port}/`).catch(() => {});
    await navigated;
    await waitFor(() => !page.isAttached());
    expect(webFrameMain.fromFrameToken(processId, frameToken)?._lifecycleStateForTesting).toBe('RunningUnloadHandlers');
    expect(page.isDestroyed()).toBe(false);
  });

  it('transfers to another window and ignores the old element release', async () => {
    const page = await createContents();
    const first = await createHost();
    const second = await createHost();
    await attach(first, page);
    page.detach();
    await attach(second, page);
    await first.webContents.executeJavaScript('view.remove()');
    first.destroy();
    expect(page.isAttached()).toBe(true);
    expect(await page.executeJavaScript('persistentMarker')).toBe(123);
    second.destroy();
    await waitFor(() => !page.isAttached());
    expect(await page.executeJavaScript('persistentMarker')).toBe(123);
  });

  it('detaches synchronously when an element is removed immediately', async () => {
    const page = await createContents();
    const window = await createHost();
    await window.webContents.executeJavaScript(`(() => {
      const element = document.createElement('webview2');
      element.setAttribute('web-contents-id', '${page.id}');
      document.body.append(element);
      element.remove();
    })()`);
    expect(page.isAttached()).toBe(false);
    expect(page.isDestroyed()).toBe(false);
  });

  it('automatically detaches when a raw iframe is destroyed', async () => {
    const page = await createContents();
    const window = await createHost();
    await window.webContents.executeJavaScript("document.body.append(document.createElement('iframe'))");
    await waitFor(() => window.webContents.mainFrame.frames.length === 1);
    const frame = window.webContents.mainFrame.frames[0];
    const attachmentId = page.attach(window.webContents.mainFrame, frame.frameToken);
    expect(attachmentId).toBeTruthy();
    expect(page.isAttached()).toBe(true);
    await window.webContents.executeJavaScript("document.querySelector('iframe').remove()");
    await waitFor(() => !page.isAttached());
    expect(page.isDestroyed()).toBe(false);
    expect(await page.executeJavaScript('persistentMarker')).toBe(123);
  });

  it('rejects navigated iframe targets without changing either lifetime', async () => {
    const page = await createContents();
    const window = await createHost();
    await window.webContents.executeJavaScript(
      "const frame = document.createElement('iframe'); frame.srcdoc = 'occupied'; document.body.append(frame)"
    );
    await waitFor(() => window.webContents.mainFrame.frames.some((frame) => frame.url === 'about:srcdoc'));
    const frame = window.webContents.mainFrame.frames[0];
    expect(() => page.attach(window.webContents.mainFrame, frame.frameToken)).toThrow(/iframe/);
    expect(page.isAttached()).toBe(false);
    expect(page.isDestroyed()).toBe(false);
    expect(await window.webContents.executeJavaScript("document.querySelector('iframe').isConnected")).toBe(true);
  });

  it('updates native relationships synchronously between consecutive calls', async () => {
    const oldPage = await createContents();
    const newPage = await createContents();
    const window = await createHost();
    await attach(window, oldPage);
    await window.webContents.executeJavaScript("document.body.append(document.createElement('iframe'))");
    await waitFor(() => window.webContents.mainFrame.frames.some((frame) => frame.url === 'about:blank'));
    const frame = window.webContents.mainFrame.frames.find((frame) => frame.url === 'about:blank')!;
    expect(oldPage.detach()).toBe(true);
    const attachmentId = newPage.attach(window.webContents.mainFrame, frame.frameToken);
    expect(oldPage.isAttached()).toBe(false);
    expect(newPage.isAttached()).toBe(true);
    expect(attachmentId).toBeTruthy();
  });

  it('rejects attaching another contents to an occupied iframe', async () => {
    const first = await createContents();
    const second = await createContents();
    const window = await createHost();
    await window.webContents.executeJavaScript("document.body.append(document.createElement('iframe'))");
    const owner = window.webContents.mainFrame;
    const token = owner.frames[0].frameToken;
    first.attach(owner, token);
    expect(() => second.attach(owner, token)).toThrow(/iframe/);
    expect(first.isAttached()).toBe(true);
    expect(second.isAttached()).toBe(false);
    expect(second.isDestroyed()).toBe(false);
  });

  it('rejects a forged frame token', async () => {
    const page = await createContents();
    const window = await createHost();
    expect(() => page.attach(window.webContents.mainFrame, 'forged')).toThrow(/token/);
    expect(page.isAttached()).toBe(false);
  });
});

// These exercise actual frame routing: the host keeps an editable selection
// while two independently owned pages receive debugger input.
describe.runIf(process.platform === 'win32' || process.platform === 'darwin')('webview2 input focus', () => {
  async function evaluate(page: Electron.WebContents, expression: string) {
    const response = await page.debugger.sendCommand('Runtime.evaluate', {
      expression,
      returnByValue: true,
      awaitPromise: true
    });
    if (response.exceptionDetails) throw new Error(response.exceptionDetails.text);
    return response.result.value;
  }

  async function focusHostEditor(host: BrowserWindow) {
    // A DOM focus call alone does not prove ownership of the embedded focus tree.
    for (const type of ['mouseDown', 'mouseUp'] as const) {
      host.webContents.sendInputEvent({ type, x: 20, y: 15, button: 'left', clickCount: 1 });
    }
    await host.webContents.executeJavaScript('hostInput.focus();hostInput.setSelectionRange(1,3);focusEvents.length=0');
    await waitFor(() => host.webContents.focusedFrame?.frameToken === host.webContents.mainFrame.frameToken);
    await waitFor(() => host.webContents.executeJavaScript('document.activeElement === hostInput && document.hasFocus()'));
    await host.webContents.executeJavaScript('hostInput.setSelectionRange(1,3);focusEvents.length=0');
  }

  async function fixture(contextIsolation = true, sandbox = true) {
    const host = await createHost(
      { sandbox, contextIsolation },
      'data:text/html,<input id="hostInput" value="original"><script>window.transitions=[];window.focusEvents=[];for(const name of ["focus","blur"])hostInput.addEventListener(name,()=>focusEvents.push(name))</script>'
    );
    const pages: Electron.WebContents[] = [];
    for (const name of ['a', 'b']) {
      const page = await createContents();
      await page.loadURL('data:text/html,<input id="editor"><div style="height:3000px"></div>');
      await attach(host, page);
      await host.webContents.executeJavaScript(
        `window.${name}=view;view.style.width='320px';view.style.height='200px'`
      );
      page.debugger.attach('1.3');
      await evaluate(page, 'editor.focus()');
      pages.push(page);
    }
    await host.webContents.executeJavaScript('hostInput.focus();hostInput.setSelectionRange(1,3);focusEvents.length=0');
    async function snapshot() {
      return host.webContents.executeJavaScript(`({
        active:document.activeElement.id,
        value:hostInput.value,
        selection:[hostInput.selectionStart,hostInput.selectionEnd],
        events:[...focusEvents]
      })`);
    }
    async function enable() {
      for (const page of pages) {
        await page.debugger.sendCommand('Emulation.setFocusEmulationEnabled', { enabled: true });
      }
    }
    return { host, pages, snapshot, enable };
  }

  for (const sandbox of [true, false]) {
    for (const contextIsolation of [true, false]) {
      it(`routes keyboard, text and composition with sandbox=${sandbox}, contextIsolation=${contextIsolation}`, async () => {
        const {
          pages: [a, b],
          snapshot,
          enable
        } = await fixture(contextIsolation, sandbox);
        const before = await snapshot();
        await enable();
        await a.debugger.sendCommand('Input.dispatchKeyEvent', { type: 'char', text: 'A' });
        await b.debugger.sendCommand('Input.dispatchKeyEvent', { type: 'char', text: 'B' });
        await b.debugger.sendCommand('Input.insertText', { text: 'C' });
        await b.debugger.sendCommand('Input.imeSetComposition', {
          text: '中',
          selectionStart: 0,
          selectionEnd: 1
        });
        await b.debugger.sendCommand('Input.insertText', { text: '文' });
        expect(await evaluate(a, 'editor.value')).toBe('A');
        expect(await evaluate(b, 'editor.value')).toBe('BC文');
        expect(await snapshot()).toEqual(before);
      });
    }
  }

  it('keeps independently targeted host input while guest pages receive input', async () => {
    const {
      host,
      pages: [a, b],
      snapshot,
      enable
    } = await fixture();
    host.show();
    host.focus();
    await waitFor(() => host.isFocused());
    await focusHostEditor(host);
    await host.webContents.executeJavaScript('hostInput.focus();hostInput.setSelectionRange(1,3);focusEvents.length=0');
    await enable();
    await b.debugger.sendCommand('Input.insertText', { text: 'background' });
    host.webContents.debugger.attach('1.3');
    await host.webContents.debugger.sendCommand('Input.dispatchKeyEvent', { type: 'char', text: 'H' });
    expect((await snapshot()).value).toBe('oHginal');
    expect((await snapshot()).active).toBe('hostInput');
    expect(await evaluate(a, 'editor.value')).toBe('');
    expect(await evaluate(b, 'editor.value')).toBe('background');
  });

  it('isolates native-key debugger input with focus emulation', async () => {
    const {
      host,
      pages: [, b],
      snapshot,
      enable
    } = await fixture();
    const previousMenu = Menu.getApplicationMenu();
    let shortcuts = 0;
    Menu.setApplicationMenu(
      Menu.buildFromTemplate([
        {
          label: 'Focus regression',
          submenu: [
            {
              label: 'Unhandled shortcut',
              accelerator: 'CommandOrControl+K',
              click: () => {
                shortcuts++;
              }
            }
          ]
        }
      ])
    );
    defer(() => Menu.setApplicationMenu(previousMenu));
    host.show();
    host.focus();
    await waitFor(() => host.isFocused());
    await focusHostEditor(host);
    await evaluate(
      b,
      `editor.blur();window.keys=[];
      for(const type of ['keydown','keyup'])document.addEventListener(type,
        event=>keys.push({type:event.type,key:event.key}))`
    );
    await host.webContents.executeJavaScript(`hostInput.focus();hostInput.setSelectionRange(1,3);
      focusEvents.length=0;window.hostKeys=[];
      for(const type of ['keydown','keyup'])document.addEventListener(type,
        event=>hostKeys.push(event.type))`);
    const before = await snapshot();
    const browserInputs: string[] = [];
    b.on('before-input-event', (_event, input) => browserInputs.push(input.type));
    await enable();
    const key = {
      key: 'k',
      code: 'KeyK',
      text: 'k',
      unmodifiedText: 'k',
      modifiers: process.platform === 'darwin' ? 4 : 2,
      windowsVirtualKeyCode: 75,
      nativeVirtualKeyCode: process.platform === 'darwin' ? 40 : 75
    };
    for (const type of ['rawKeyDown', 'keyUp']) {
      await b.debugger.sendCommand('Input.dispatchKeyEvent', { type, ...key });
    }
    expect(await evaluate(b, 'keys')).toEqual([
      { type: 'keydown', key: 'k' },
      { type: 'keyup', key: 'k' }
    ]);
    expect(browserInputs).toEqual([]);
    expect(shortcuts).toBe(0);
    expect(await host.webContents.executeJavaScript('hostKeys')).toEqual([]);
    expect(await snapshot()).toEqual(before);

    // Main-process native input retains its browser handling path.
    await host.webContents.executeJavaScript('b.focus()');
    for (const type of ['keyDown', 'keyUp'] as const) {
      b.sendInputEvent({ type, keyCode: 'K', modifiers: [process.platform === 'darwin' ? 'meta' : 'control'] });
    }
    await waitFor(() => browserInputs.includes('keyDown'));
    await waitFor(() => browserInputs.includes('keyUp'));
    // sendInputEvent does not construct a macOS NSEvent for menu dispatch.
  });

  it('routes page and explicit frame input through an out-of-process child', async () => {
    const {
      host,
      pages: [, b],
      snapshot,
      enable
    } = await fixture();
    const server = http.createServer((_request, response) => {
      response.setHeader('Content-Type', 'text/html');
      response.end('<input id="childEditor">');
    });
    defer(() => {
      server.closeAllConnections();
      server.close();
    });
    const { url } = await listen(server);
    await evaluate(
      b,
      `new Promise(resolve=>{const frame=document.createElement('iframe');frame.src=${JSON.stringify(url)};frame.onload=resolve;document.body.prepend(frame)})`
    );
    const child = b.mainFrame.frames[0];
    expect(child.processId).not.toBe(b.mainFrame.processId);
    await child.executeJavaScript('childEditor.focus()');
    await host.webContents.executeJavaScript('hostInput.focus();hostInput.setSelectionRange(1,3);focusEvents.length=0');
    const before = await snapshot();
    await enable();
    await b.debugger.sendCommand('Input.dispatchKeyEvent', { type: 'char', text: '子' });
    await b.debugger.sendCommand('Input.insertText', { text: '页面' });
    expect(await child.executeJavaScript('childEditor.value')).toBe('子页面');
    const { targetInfos } = await b.debugger.sendCommand('Target.getTargets');
    const childTarget = targetInfos.find(
      (target: { type: string; url: string }) => target.type === 'iframe' && target.url === child.url
    );
    expect(childTarget).toBeDefined();
    const { sessionId } = await b.debugger.sendCommand('Target.attachToTarget', {
      targetId: childTarget.targetId,
      flatten: true
    });
    await b.debugger.sendCommand('Emulation.setFocusEmulationEnabled', { enabled: true }, sessionId);
    await evaluate(b, 'editor.focus()');
    await host.webContents.executeJavaScript('hostInput.focus();hostInput.setSelectionRange(1,3);focusEvents.length=0');
    await child.executeJavaScript(
      "window.frameKeys=[];document.addEventListener('keydown',event=>frameKeys.push(event.key))"
    );
    for (const type of ['rawKeyDown', 'keyUp']) {
      await b.debugger.sendCommand(
        'Input.dispatchKeyEvent',
        {
          type,
          key: 'F2',
          code: 'F2',
          windowsVirtualKeyCode: 113
        },
        sessionId
      );
    }
    expect(await child.executeJavaScript('frameKeys')).toEqual(['F2']);
    expect(await child.executeJavaScript('document.hasFocus()')).toBe(true);
    // Selecting a frame does not select an editable DOM element.
    await child.executeJavaScript('childEditor.focus()');
    await host.webContents.executeJavaScript('hostInput.focus();hostInput.setSelectionRange(1,3);focusEvents.length=0');
    for (const type of ['rawKeyDown', 'keyUp']) {
      await b.debugger.sendCommand(
        'Input.dispatchKeyEvent',
        {
          type,
          key: 'End',
          code: 'End',
          windowsVirtualKeyCode: 35
        },
        sessionId
      );
    }
    await b.debugger.sendCommand('Input.insertText', { text: '!' }, sessionId);
    expect(await child.executeJavaScript('childEditor.value')).toBe('子页面!');
    expect(await evaluate(b, 'editor.value')).toBe('');
    expect(await snapshot()).toEqual(before);
  });

  it('restores each page when its emulation ends without changing another page', async () => {
    const {
      pages: [a, b],
      snapshot,
      enable
    } = await fixture();
    const before = await snapshot();
    await enable();
    for (const page of [a, b]) await page.debugger.sendCommand('Input.insertText', { text: 'x' });
    await a.debugger.sendCommand('Emulation.setFocusEmulationEnabled', { enabled: false });
    await waitFor(async () => !(await evaluate(a, 'document.hasFocus()')));
    expect(await evaluate(b, 'document.hasFocus()')).toBe(true);
    await b.debugger.sendCommand('Emulation.setFocusEmulationEnabled', { enabled: false });
    await waitFor(async () => !(await evaluate(b, 'document.hasFocus()')));
    expect(await snapshot()).toEqual(before);
  });

  it('restores focus from the current user choice instead of an old snapshot', async () => {
    const {
      host,
      pages: [, b],
      enable
    } = await fixture();
    await enable();
    host.show();
    host.focus();
    await waitFor(() => host.isFocused());
    await focusHostEditor(host);
    await host.webContents.executeJavaScript('hostInput.focus()');
    await b.debugger.sendCommand('Input.insertText', { text: 'background' });
    await host.webContents.executeJavaScript('b.focus()');
    await waitFor(() => host.webContents.executeJavaScript('document.activeElement === b'));
    await b.debugger.sendCommand('Emulation.setFocusEmulationEnabled', { enabled: false });
    expect(await evaluate(b, 'document.hasFocus()')).toBe(true);
    await host.webContents.executeJavaScript('hostInput.focus()');
    await waitFor(async () => !(await evaluate(b, 'document.hasFocus()')));
    expect(await evaluate(b, 'editor.value')).toBe('background');
  });

  it('routes debugger keys to a fresh page that has never been focused', async () => {
    const host = await createHost(
      { sandbox: true, contextIsolation: true },
      'data:text/html,<input id="hostInput" value="original"><script>window.transitions=[]</script>'
    );
    const page = await createContents();
    await page.loadURL(
      `data:text/html,<script>window.keys=[];document.addEventListener('keydown',event=>keys.push(event.key))</script>`
    );
    await attach(host, page);
    await host.webContents.executeJavaScript('hostInput.focus();hostInput.setSelectionRange(1,3)');
    page.debugger.attach('1.3');
    await page.debugger.sendCommand('Emulation.setFocusEmulationEnabled', { enabled: true });
    for (const type of ['rawKeyDown', 'keyUp']) {
      await page.debugger.sendCommand('Input.dispatchKeyEvent', {
        type,
        key: 'F5',
        code: 'F5',
        windowsVirtualKeyCode: 116
      });
    }
    expect(await evaluate(page, 'keys')).toEqual(['F5']);
    expect(await evaluate(page, 'document.hasFocus()')).toBe(true);
    expect(
      await host.webContents.executeJavaScript(`({active:document.activeElement.id,
      value:hostInput.value,selection:[hostInput.selectionStart,hostInput.selectionEnd]})`)
    ).toEqual({ active: 'hostInput', value: 'original', selection: [1, 3] });
  });

  it('isolates keyboard and composition on a read-only page with focus emulation', async () => {
    const {
      host,
      pages: [a, b],
      snapshot,
      enable
    } = await fixture();
    await enable();
    await b.loadURL('data:text/html,<div id="readOnly">read-only</div>');
    await evaluate(b, `window.keys=[];
      document.addEventListener('keydown',event=>keys.push(event.key))`);
    await host.webContents.executeJavaScript(`window.hostKeys=[];
      document.addEventListener('keydown',event=>hostKeys.push(event.key))`);
    const before = await snapshot();
    for (const type of ['rawKeyDown', 'keyUp']) {
      await b.debugger.sendCommand('Input.dispatchKeyEvent', {
        type,
        key: 'F2',
        code: 'F2',
        windowsVirtualKeyCode: 113
      });
    }
    await b.debugger.sendCommand('Input.imeSetComposition', {
      text: '中',
      selectionStart: 0,
      selectionEnd: 1
    });
    await b.debugger.sendCommand('Input.insertText', { text: 'no-editor' });
    expect(await evaluate(b, 'keys')).toEqual(['F2']);
    expect(await evaluate(b, 'document.body.textContent')).toBe('read-only');
    expect(await host.webContents.executeJavaScript('hostKeys')).toEqual([]);
    expect(await evaluate(b, 'document.hasFocus()')).toBe(true);
    expect(await evaluate(a, 'editor.value')).toBe('');
    expect(await snapshot()).toEqual(before);
  });

  it('keeps debugger keys independent of ordinary keyboard suppression', async () => {
    const { pages: [, b], snapshot, enable } = await fixture();
    await enable();
    const before = await snapshot();
    b.on('before-input-event', (event, input) => {
      if (input.type === 'keyDown' && input.key.toLowerCase() === 'a') event.preventDefault();
    });
    b.sendInputEvent({ type: 'keyDown', keyCode: 'A' });
    await b.debugger.sendCommand('Input.dispatchKeyEvent', { type: 'char', text: 'X' });
    expect(await evaluate(b, 'editor.value')).toBe('X');
    // The debugger key must neither consume nor clear the native suppression.
    b.sendInputEvent({ type: 'char', keyCode: 'Y' });
    expect(await evaluate(b, 'editor.value')).toBe('X');
    b.sendInputEvent({ type: 'keyDown', keyCode: 'Z' });
    b.sendInputEvent({ type: 'char', keyCode: 'Z' });
    b.sendInputEvent({ type: 'keyUp', keyCode: 'Z' });
    await waitFor(() => evaluate(b, 'editor.value === "XZ"'));
    expect(await snapshot()).toEqual(before);
  });

  it('applies debugger edit commands only to the guest editor', async () => {
    const { host, pages: [, b], snapshot, enable } = await fixture();
    await evaluate(b, 'editor.value="guest-text";editor.focus()');
    await host.webContents.executeJavaScript('hostInput.focus();hostInput.setSelectionRange(1,3);focusEvents.length=0');
    const before = await snapshot();
    await enable();
    await b.debugger.sendCommand('Input.dispatchKeyEvent', {
      type: 'rawKeyDown', key: 'F2', code: 'F2', windowsVirtualKeyCode: 113,
      commands: ['selectAll']
    });
    await b.debugger.sendCommand('Input.insertText', { text: 'replacement' });
    expect(await evaluate(b, 'editor.value')).toBe('replacement');
    expect(await snapshot()).toEqual(before);
  });

  it('routes page keyboard input to a same-process child frame', async () => {
    const { host, pages: [, b], snapshot, enable } = await fixture();
    await enable();
    await evaluate(b, `new Promise(resolve=>{const frame=document.createElement('iframe');
      frame.srcdoc='<input id="childEditor">';frame.onload=resolve;document.body.prepend(frame)})`);
    const child = b.mainFrame.frames[0];
    expect(child.processId).toBe(b.mainFrame.processId);
    await child.executeJavaScript('childEditor.focus()');
    await host.webContents.executeJavaScript('hostInput.focus();hostInput.setSelectionRange(1,3);focusEvents.length=0');
    const before = await snapshot();
    await b.debugger.sendCommand('Input.dispatchKeyEvent', { type: 'char', text: 'C' });
    await b.debugger.sendCommand('Input.insertText', { text: '-text' });
    expect(await child.executeJavaScript('childEditor.value')).toBe('C-text');
    expect(await evaluate(b, 'editor.value')).toBe('');
    expect(await snapshot()).toEqual(before);
  });

  for (const emulateFocus of [false, true]) {
    it(`applies debugger mouse focus policy with emulation=${emulateFocus}`, async () => {
      const { host, pages: [a, b], snapshot, enable } = await fixture();
      host.show();
      host.focus();
      await waitFor(() => host.isFocused());
      await focusHostEditor(host);
      if (emulateFocus) await enable();
      for (const page of [a, b]) {
        await evaluate(page, 'editor.blur();window.clicks=0;editor.onclick=()=>clicks++');
      }
      await host.webContents.executeJavaScript('hostInput.focus();hostInput.setSelectionRange(1,3);focusEvents.length=0');
      const before = await snapshot();
      for (const [page, text, tag] of [[a, 'A', 'a'], [b, 'B', 'b']] as const) {
        await focusHostEditor(host);
        for (const type of ['mousePressed', 'mouseReleased']) {
          await page.debugger.sendCommand('Input.dispatchMouseEvent', {
            type, x: 20, y: 15, button: 'left', clickCount: 1
          });
        }
        await waitFor(() => evaluate(page, 'clicks === 1'));
        expect(await evaluate(page, 'document.activeElement.id')).toBe('editor');
        await page.debugger.sendCommand('Input.insertText', { text });
        expect(await evaluate(page, 'editor.value')).toBe(text);
        expect(await evaluate(page, 'document.hasFocus()')).toBe(true);
        if (emulateFocus) {
          expect(host.webContents.focusedFrame?.frameToken).toBe(host.webContents.mainFrame.frameToken);
          expect(await snapshot()).toEqual(before);
        } else {
          await waitFor(() => host.webContents.executeJavaScript(`document.activeElement === ${tag}`));
          expect((await snapshot()).value).toBe(before.value);
          expect((await snapshot()).selection).toEqual(before.selection);
          expect(host.webContents.focusedFrame?.frameToken).toBe(page.mainFrame.frameToken);
          for (const type of ['keyDown', 'char', 'keyUp'] as const) {
            page.sendInputEvent({ type, keyCode: 'Z' });
          }
          await waitFor(() => evaluate(page, `editor.value === '${text}Z'`));
          expect((await snapshot()).value).toBe(before.value);
        }
      }

    });
  }

  it('retains real focus transfer for non-debugger mouse input', async () => {
    const { host, pages: [, b], enable } = await fixture();
    host.show();
    host.focus();
    await waitFor(() => host.isFocused());
    await focusHostEditor(host);
    await enable();
    for (const type of ['mouseDown', 'mouseUp'] as const) {
      b.sendInputEvent({ type, x: 20, y: 15, button: 'left', clickCount: 1 });
    }
    await waitFor(() => host.webContents.executeJavaScript('document.activeElement === b'));
    expect(await evaluate(b, 'document.activeElement.id')).toBe('editor');
  });

  it('keeps native Tab and Shift+Tab traversal inside the guest at document boundaries', async () => {
    const {
      host,
      pages: [, b],
      snapshot,
      enable
    } = await fixture();
    host.show();
    host.focus();
    await waitFor(() => host.isFocused());
    await focusHostEditor(host);
    await enable();
    await evaluate(
      b,
      "document.body.insertAdjacentHTML('beforeend','<input id=second><input id=third>');editor.focus()"
    );
    const before = await snapshot();
    for (const { modifiers, targets } of [
      { modifiers: 0, targets: ['second', 'third', 'editor'] },
      { modifiers: 8, targets: ['third', 'second', 'editor'] }
    ]) {
      for (const expected of targets) {
        for (const type of ['rawKeyDown', 'keyUp']) {
          await b.debugger.sendCommand('Input.dispatchKeyEvent', {
            type,
            key: 'Tab',
            code: 'Tab',
            windowsVirtualKeyCode: 9,
            modifiers
          });
        }
        expect(await evaluate(b, 'document.activeElement.id')).toBe(expected);
        expect(await snapshot()).toEqual(before);
      }
    }
  });

  it('dispatches an isolated left-button sequence and keeps outer input focus', async () => {
    const { host, pages: [, b], snapshot, enable } = await fixture();
    host.show();
    host.focus();
    await waitFor(() => host.isFocused());
    await focusHostEditor(host);
    await enable();
    await b.debugger.sendCommand('Page.captureScreenshot', { format: 'png' });
    const record = `window.mouseEvents = [];
      for (const type of ['mousedown', 'mousemove', 'mouseup', 'click', 'dragstart'])
        document.addEventListener(type, event => mouseEvents.push([type, event.buttons]));`;
    await evaluate(b, record);
    await host.webContents.executeJavaScript(record);
    const before = await snapshot();
    await b.debugger.sendCommand('Input.dispatchMouseEvent', {
      type: 'mousePressed', x: 20, y: 15, button: 'left', buttons: 1, clickCount: 1
    });
    await b.debugger.sendCommand('Input.dispatchMouseEvent', {
      type: 'mouseMoved', x: 45, y: 15, button: 'left', buttons: 1
    });
    await b.debugger.sendCommand('Input.dispatchMouseEvent', {
      type: 'mouseReleased', x: 45, y: 15, button: 'left', buttons: 0, clickCount: 1
    });
    expect(await evaluate(b, 'mouseEvents.map(event => event[0])'))
      .toEqual(['mousedown', 'mousemove', 'mouseup', 'click']);
    await expect(b.debugger.sendCommand('Input.dispatchMouseEvent', {
      type: 'mousePressed', x: -20, y: 15, button: 'left', clickCount: 1
    })).rejects.toThrow(/outside the session view/);
    expect(await host.webContents.executeJavaScript('mouseEvents')).toEqual([]);
    expect(await snapshot()).toEqual(before);
  });

  it('uses the legacy focus handoff for other mouse buttons with emulation enabled', async () => {
    const { host, pages: [, b], enable } = await fixture();
    host.show();
    host.focus();
    await waitFor(() => host.isFocused());
    await focusHostEditor(host);
    await enable();
    for (const type of ['mousePressed', 'mouseReleased']) {
      await b.debugger.sendCommand('Input.dispatchMouseEvent', {
        type, x: 20, y: 15, button: 'middle', clickCount: 1
      });
    }
    await waitFor(() => host.webContents.executeJavaScript('document.activeElement === b'));
  });

  it('preserves the legacy result of wheel and synthetic tap with focus emulation enabled', async () => {
    const run = async (emulateFocus: boolean) => {
      const { host, pages: [, b], snapshot, enable } = await fixture();
      host.show();
      host.focus();
      await waitFor(() => host.isFocused());
      await focusHostEditor(host);
      if (emulateFocus) await enable();
      const record = `window.legacyEvents = [];
        for (const type of ['wheel', 'mousedown', 'mouseup', 'click'])
          document.addEventListener(type, event => legacyEvents.push(event.type), { passive: true });`;
      await evaluate(b, record);
      await host.webContents.executeJavaScript(record);
      await b.debugger.sendCommand('Page.captureScreenshot', { format: 'png' });
      await b.debugger.sendCommand('Input.dispatchMouseEvent', {
        type: 'mouseWheel', x: 40, y: 70, deltaX: 0, deltaY: 100
      });
      await b.debugger.sendCommand('Input.synthesizeTapGesture', {
        x: 20, y: 15, gestureSourceType: 'mouse'
      });
      return {
        inner: await evaluate(b, 'legacyEvents'),
        outer: await host.webContents.executeJavaScript('legacyEvents'),
        focus: await snapshot()
      };
    };
    const legacy = await run(false);
    expect(await run(true)).toEqual(legacy);
  });

  for (const mode of ['legacy-mouse', 'isolated-mouse'] as const) {
    const emulateFocus = mode !== 'legacy-mouse';
    it(`routes debugger OOPIF clicks in ${mode} mode`, async () => {
      const { host, pages: [, b], snapshot, enable } = await fixture();
      const server = http.createServer((_request, response) => {
        response.setHeader('Content-Type', 'text/html');
        response.end('<input id="childEditor"><script>window.clicks=0;childEditor.onclick=()=>clicks++</script>');
      });
      defer(() => { server.closeAllConnections(); server.close(); });
      const { url } = await listen(server);
      await evaluate(b, `new Promise(resolve=>{editor.remove();const frame=document.createElement('iframe');
        frame.style.cssText='border:0;width:300px;height:150px';frame.src=${JSON.stringify(url)};
        frame.onload=resolve;document.body.prepend(frame)})`);
      const child = b.mainFrame.frames[0];
      expect(child.processId).not.toBe(b.mainFrame.processId);
      host.show();
      host.focus();
      await waitFor(() => host.isFocused());
      await focusHostEditor(host);
      if (emulateFocus) await enable();
      await host.webContents.executeJavaScript('hostInput.focus();hostInput.setSelectionRange(1,3);focusEvents.length=0');
      const before = await snapshot();
      // Wait for the child surface to be rendered before testing coordinate routing.
      await b.debugger.sendCommand('Page.captureScreenshot', { format: 'png' });
      for (const type of ['mousePressed', 'mouseReleased']) {
        await b.debugger.sendCommand('Input.dispatchMouseEvent', {
          type, x: 28, y: 23, button: 'left', clickCount: 1
        });
      }
      await waitFor(() => child.executeJavaScript('clicks === 1'));
      expect(await child.executeJavaScript('document.activeElement.id')).toBe('childEditor');
      await b.debugger.sendCommand('Input.insertText', { text: 'child' });
      expect(await child.executeJavaScript('childEditor.value')).toBe('child');
      if (emulateFocus) {
        expect(host.webContents.focusedFrame?.frameToken).toBe(host.webContents.mainFrame.frameToken);
        expect(await snapshot()).toEqual(before);
      } else {
        await waitFor(() => host.webContents.executeJavaScript('document.activeElement === b'));
        // The native keyboard selector uses this shared focused-frame result.
        // sendInputEvent on the host addresses its main widget, not this OOPIF.
        expect(host.webContents.focusedFrame?.frameToken).toBe(child.frameToken);
        expect(await child.executeJavaScript('document.hasFocus()')).toBe(true);
        expect((await snapshot()).value).toBe(before.value);
        expect((await snapshot()).selection).toEqual(before.selection);
      }
    });
  }

  it('continues targeting the same independent page after detaching and reattaching', async () => {
    const {
      host,
      pages: [, b],
      snapshot,
      enable
    } = await fixture();
    const before = await snapshot();
    await enable();
    await b.debugger.sendCommand('Input.insertText', { text: 'before' });
    await host.webContents.executeJavaScript('b.remove()');
    await waitFor(() => !b.isAttached());
    await attach(host, b);
    await host.webContents.executeJavaScript('b=view;hostInput.focus();hostInput.setSelectionRange(1,3)');
    await b.debugger.sendCommand('Input.insertText', { text: '-after' });
    expect(await evaluate(b, 'editor.value')).toBe('before-after');
    expect(await snapshot()).toEqual(before);
  });

  it('uses the new document after navigation and releases focus on debugger detach', async () => {
    const {
      pages: [, b],
      snapshot,
      enable
    } = await fixture();
    const before = await snapshot();
    await enable();
    await b.debugger.sendCommand('Input.insertText', { text: 'old' });
    await b.loadURL('data:text/html,<input id="editor">');
    await evaluate(b, 'editor.focus()');
    await b.debugger.sendCommand('Input.insertText', { text: 'new' });
    expect(await evaluate(b, 'editor.value')).toBe('new');
    b.debugger.detach();
    await waitFor(async () => !(await b.executeJavaScript('document.hasFocus()')));
    expect(await snapshot()).toEqual(before);
  });

  it('delegates tag focus and preserves the guest editor when the tag blurs', async () => {
    const {
      host,
      pages: [a],
      snapshot
    } = await fixture();
    await host.webContents.executeJavaScript(
      `window.tagEvents=[];for(const name of ['focus','blur'])a.addEventListener(name,()=>tagEvents.push(name));a.focus()`
    );
    await waitFor(() => host.webContents.executeJavaScript('document.activeElement === a'));
    await host.webContents.executeJavaScript('a.focus();a.blur()');
    await waitFor(() => host.webContents.executeJavaScript('document.activeElement !== a'));
    await host.webContents.executeJavaScript('hostInput.focus()');
    await waitFor(() => host.webContents.executeJavaScript('document.activeElement === hostInput'));
    expect(await host.webContents.executeJavaScript('tagEvents')).toEqual(['focus', 'blur']);
    expect(await evaluate(a, 'document.activeElement.id')).toBe('editor');
    expect((await snapshot()).active).toBe('hostInput');
  });
});
