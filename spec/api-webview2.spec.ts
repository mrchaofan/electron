import { BrowserWindow, session, webContents, webFrameMain } from 'electron/main';

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
