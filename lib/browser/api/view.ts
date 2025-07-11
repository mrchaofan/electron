import { EventEmitter } from 'events';

const { View } = process._linkedBinding('electron_browser_view');

Object.setPrototypeOf((View as any).prototype, EventEmitter.prototype);

(View as any).prototype.onMouseEntered = function onMouseEntered () {

};

(View as any).prototype.onMouseExited = function onMouseExited () {

};
(View as any).prototype.onMouseMoved = function onMouseMoved () {

};
(View as any).prototype.onMousePressed = function onMousePressed () {
  return false;
};
(View as any).prototype.onMouseReleased = function onMouseReleased () {

};
(View as any).prototype.onMouseDragged = function onMouseDragged () {
  return false;
};
(View as any).prototype.onMouseCaptureLost = function onMouseCaptureLost () {

};

export default View;
