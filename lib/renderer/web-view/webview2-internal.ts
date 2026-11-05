import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import { invokeSync } from '@electron/internal/renderer/ipc-renderer-internal-utils';
const { mainFrame } = process._linkedBinding('electron_renderer_web_frame');
export interface Receipt {
  webContentsId: number;
  attachmentId: string;
}
export interface AttachmentError {
  webContentsId: number | null;
  code: 'invalid-id' | 'attach-failed';
  message: string;
}
export function attach(iframe: HTMLIFrameElement, id: number): Receipt {
  const frame = mainFrame._findFrameByWindow(iframe.contentWindow!);
  if (!frame) throw new Error('Invalid webview2 iframe');
  const attachmentId: string = invokeSync(IPC_MESSAGES.WEBVIEW2_ATTACH, id, frame.frameToken);
  return { webContentsId: id, attachmentId };
}
export function release(receipt: Receipt) {
  invokeSync(IPC_MESSAGES.WEBVIEW2_RELEASE, receipt.webContentsId, receipt.attachmentId);
}
export function subscribe(callback: (attachmentId: string) => void) {
  const listener = (_event: Electron.IpcRendererEvent, attachmentId: string) => callback(attachmentId);
  ipcRendererInternal.on(IPC_MESSAGES.WEBVIEW2_DETACHED, listener);
  return () => ipcRendererInternal.removeListener(IPC_MESSAGES.WEBVIEW2_DETACHED, listener);
}
