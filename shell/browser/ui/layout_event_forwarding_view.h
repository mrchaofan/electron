// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_LAYOUT_EVENT_FORWARDING_VIEW_H_
#define ELECTRON_SHELL_BROWSER_UI_LAYOUT_EVENT_FORWARDING_VIEW_H_

#include <optional>

#include "base/memory/raw_ptr.h"
#include "ui/gfx/geometry/size.h"

namespace views {
class SizeBounds;
}  // namespace views

namespace electron {

class LayoutEventForwardingDelegate {
 public:
  virtual ~LayoutEventForwardingDelegate() = default;

  virtual std::optional<gfx::Size> OnCalculatePreferredSizeFromNative(
      const views::SizeBounds& available_size) = 0;
  virtual void OnLayoutFromNative() = 0;
};

class LayoutEventForwardingView {
 public:
  explicit LayoutEventForwardingView(LayoutEventForwardingDelegate* delegate)
      : delegate_(delegate) {}
  virtual ~LayoutEventForwardingView() = default;

 protected:
  bool HasLayoutDelegate() const { return delegate_ != nullptr; }

  std::optional<gfx::Size> ForwardCalculatePreferredSize(
      const views::SizeBounds& available_size) const {
    return delegate_->OnCalculatePreferredSizeFromNative(available_size);
  }

  void OnLayout() const { delegate_->OnLayoutFromNative(); }

 private:
  raw_ptr<LayoutEventForwardingDelegate> delegate_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_LAYOUT_EVENT_FORWARDING_VIEW_H_
