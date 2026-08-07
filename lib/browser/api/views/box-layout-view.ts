import { View } from 'electron/main';

const { BoxLayoutView } = process._linkedBinding('electron_browser_box_layout_view');

Object.setPrototypeOf(BoxLayoutView.prototype, View.prototype);

export default BoxLayoutView;
