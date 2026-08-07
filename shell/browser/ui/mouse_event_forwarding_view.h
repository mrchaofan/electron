// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_MOUSE_EVENT_FORWARDING_VIEW_H_
#define ELECTRON_SHELL_BROWSER_UI_MOUSE_EVENT_FORWARDING_VIEW_H_

#include "base/memory/raw_ptr.h"

namespace gfx {
class Point;
}  // namespace gfx

namespace ui {
class MouseEvent;
class MouseWheelEvent;
}  // namespace ui

namespace electron {

class MouseEventForwardingDelegate {
 public:
  virtual ~MouseEventForwardingDelegate() = default;

  virtual bool OnMousePressedFromNative(const ui::MouseEvent& event) = 0;
  virtual bool OnMouseDraggedFromNative(const ui::MouseEvent& event) = 0;
  virtual void OnMouseReleasedFromNative(const ui::MouseEvent& event) = 0;
  virtual void OnMouseCaptureLostFromNative() = 0;
  virtual void OnMouseMovedFromNative(const ui::MouseEvent& event) = 0;
  virtual void OnMouseEnteredFromNative(const ui::MouseEvent& event) = 0;
  virtual void OnMouseExitedFromNative(const ui::MouseEvent& event) = 0;
  virtual bool OnMouseWheelFromNative(const ui::MouseWheelEvent& event) = 0;
  virtual bool OnHitTestPointFromNative(const gfx::Point& point) = 0;
};

class MouseEventForwardingView {
 public:
  explicit MouseEventForwardingView(MouseEventForwardingDelegate* delegate)
      : delegate_(delegate) {}
  virtual ~MouseEventForwardingView() = default;

 protected:
  bool HasMouseDelegate() const { return delegate_ != nullptr; }

  bool ForwardMousePressed(const ui::MouseEvent& event) const {
    return delegate_->OnMousePressedFromNative(event);
  }

  bool ForwardMouseDragged(const ui::MouseEvent& event) const {
    return delegate_->OnMouseDraggedFromNative(event);
  }

  void ForwardMouseReleased(const ui::MouseEvent& event) const {
    delegate_->OnMouseReleasedFromNative(event);
  }

  void ForwardMouseCaptureLost() const {
    delegate_->OnMouseCaptureLostFromNative();
  }

  void ForwardMouseMoved(const ui::MouseEvent& event) const {
    delegate_->OnMouseMovedFromNative(event);
  }

  void ForwardMouseEntered(const ui::MouseEvent& event) const {
    delegate_->OnMouseEnteredFromNative(event);
  }

  void ForwardMouseExited(const ui::MouseEvent& event) const {
    delegate_->OnMouseExitedFromNative(event);
  }

  bool ForwardMouseWheel(const ui::MouseWheelEvent& event) const {
    return delegate_->OnMouseWheelFromNative(event);
  }

  bool ForwardHitTestPoint(const gfx::Point& point) const {
    return delegate_->OnHitTestPointFromNative(point);
  }

 private:
  raw_ptr<MouseEventForwardingDelegate> delegate_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_MOUSE_EVENT_FORWARDING_VIEW_H_
