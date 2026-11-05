import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import { webContents } from 'electron/main';

// The native bridge owns all attachment state; the attachment ID is the only
// handle. Detach notifications are forwarded by the host WebContents.
ipcMainUtils.handleSync(IPC_MESSAGES.WEBVIEW2_ATTACH, (event, id: number, frameToken: string) => {
  if (!('sender' in event) || event.senderFrame !== event.sender.mainFrame) {
    throw new Error('webview2 requires a main frame');
  }
  if (!event.sender.getLastWebPreferences()?.webview2Tag) throw new Error('webview2Tag is disabled');
  const contents = webContents.fromId(id);
  if (!contents || contents.isDestroyed()) throw new Error('WebContents does not exist');
  if (String(contents.getType()) !== 'webview2') throw new Error('WebContents is not webview2');
  return contents.attach(event.senderFrame, frameToken);
});

ipcMainUtils.handleSync(IPC_MESSAGES.WEBVIEW2_RELEASE, (_event, id: number, attachmentId: string) => {
  webContents.fromId(id)?.detach(attachmentId);
});
