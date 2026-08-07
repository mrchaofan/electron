import { EventEmitter } from 'events';

const { View } = process._linkedBinding('electron_browser_view');

Object.setPrototypeOf((View as any).prototype, EventEmitter.prototype);

// Mirrors the no-layout-manager branch of views::View::CalculatePreferredSize():
// return an empty size unless user code overrides this method.
(View as any).prototype.calculatePreferredSize = function (availableSize: { width: number, height: number }) {
  return { width: 0, height: 0 };
};

export default View;
