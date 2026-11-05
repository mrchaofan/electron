import type { WebViewImplHooks } from '@electron/internal/renderer/web-view/web-view-impl';
import type * as internal from '@electron/internal/renderer/web-view/webview2-internal';
export interface WebView2Hooks {
  webview2Internal: typeof internal;
  allowGuestViewElementDefinition: WebViewImplHooks['allowGuestViewElementDefinition'];
  setIsWebView: WebViewImplHooks['setIsWebView'];
}
export function setupWebView2(hooks: WebView2Hooks) {
  const register = () => {
    if (customElements.get('webview2')) return;
    const states = new WeakMap<HTMLElement, State>();
    class State {
      connected = false;
      receipt?: internal.Receipt;
      unsubscribe?: () => void;
      readonly root: ShadowRoot;
      constructor(readonly element: HTMLElement) {
        this.root = element.attachShadow({ mode: 'closed' });
        const style = document.createElement('style');
        style.textContent =
          ':host{display:flex}iframe{width:100%;height:100%;flex:1;border:0;min-width:0;min-height:0}';
        this.root.append(style);
      }
      emit(name: string, detail: internal.Receipt | internal.AttachmentError) {
        this.element.dispatchEvent(new CustomEvent(name, { detail }));
      }
      stop() {
        const receipt = this.receipt;
        this.receipt = undefined;
        this.unsubscribe?.();
        this.unsubscribe = undefined;
        if (receipt) hooks.webview2Internal.release(receipt);
        this.root.querySelector('iframe')?.remove();
        if (receipt) this.emit('detached', receipt);
      }
      start() {
        if (!this.connected || this.receipt) return;
        const value = this.element.getAttribute('web-contents-id');
        if (value === null) return;
        if (!/^[1-9][0-9]*$/.test(value) || !Number.isSafeInteger(Number(value))) {
          this.emit('error', { webContentsId: null, code: 'invalid-id', message: 'Invalid web-contents-id' });
          return;
        }
        const id = Number(value);
        const iframe = document.createElement('iframe');
        hooks.setIsWebView(iframe);
        this.root.append(iframe);
        try {
          this.receipt = hooks.webview2Internal.attach(iframe, id);
        } catch (error) {
          iframe.remove();
          this.emit('error', {
            webContentsId: id,
            code: 'attach-failed',
            message: error instanceof Error ? error.message : String(error)
          });
          console.error('webview2 attachment failed:', error);
          return;
        }
        // Notifications for other or already released attachments never match.
        this.unsubscribe = hooks.webview2Internal.subscribe((attachmentId) => {
          if (this.receipt?.attachmentId === attachmentId) this.stop();
        });
        this.emit('attached', this.receipt);
      }
    }
    class WebView2 extends HTMLElement {
      static get observedAttributes() {
        return ['web-contents-id'];
      }
      constructor() {
        super();
        states.set(this, new State(this));
      }
      // Upgrades run attributeChangedCallback before connectedCallback; only
      // the latter may start, so each connection makes one attempt.
      connectedCallback() {
        const state = states.get(this)!;
        state.connected = true;
        state.start();
      }
      disconnectedCallback() {
        const state = states.get(this)!;
        state.connected = false;
        state.stop();
      }
      attributeChangedCallback(_name: string, oldValue: string | null, value: string | null) {
        if (oldValue === value) return;
        const state = states.get(this)!;
        state.stop();
        state.start();
      }
    }
    hooks.allowGuestViewElementDefinition(window, () => {
      customElements.define('webview2', WebView2);
      delete (WebView2.prototype as any).connectedCallback;
      delete (WebView2.prototype as any).disconnectedCallback;
      delete (WebView2.prototype as any).attributeChangedCallback;
      delete (WebView2 as any).observedAttributes;
    });
  };
  if (document.readyState !== 'loading') register();
  else document.addEventListener('DOMContentLoaded', register, { once: true });
}
