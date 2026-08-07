import { View } from 'electron/main';

const { LabelView } = process._linkedBinding('electron_browser_label_view');

Object.setPrototypeOf(LabelView.prototype, View.prototype);

export default LabelView;
