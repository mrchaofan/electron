// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_SUPER_LAYOUT_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_SUPER_LAYOUT_VIEW_H_

#include <string>
#include "gin/handle.h"
#include "shell/browser/api/electron_api_view.h"
#include "ui/views/layout/layout_types.h"

namespace electron::super {
class SuperCrLayoutViewDelegates {
 public:
  friend class SuperCrLayoutView;

 protected:
  virtual gfx::Size CalculateCrPreferredSize(
      const views::SizeBounds& available_size) const;
  virtual gfx::Size GetCrMinimumSize() const;
  virtual gfx::Size GetCrMaximumSize() const;
};
}  // namespace electron::super

namespace electron::api {
class SuperLayoutView : public View, public super::SuperCrLayoutViewDelegates {
 public:
  static gin_helper::WrappableBase* New(gin_helper::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);
  SuperLayoutView(const SuperLayoutView&) = delete;
  SuperLayoutView& operator=(const SuperLayoutView&) = delete;

 protected:
  SuperLayoutView();
  ~SuperLayoutView() override;
  gfx::Size CalculateCrPreferredSize(
      const views::SizeBounds& available_size) const override;
  gfx::Size GetCrMinimumSize() const override;
  gfx::Size GetCrMaximumSize() const override;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_SUPER_LAYOUT_VIEW_H_
