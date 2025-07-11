// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_SUPER_BOX_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_SUPER_BOX_VIEW_H_

#include <string>
#include "gin/handle.h"
#include "shell/browser/api/electron_api_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/layout/layout_types.h"

namespace electron::api {

class SuperBoxView : public View {
 public:
  static gin_helper::WrappableBase* New(gin_helper::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);
  SuperBoxView(const SuperBoxView&) = delete;
  SuperBoxView& operator=(const SuperBoxView&) = delete;

  void SetOrientation(views::LayoutOrientation orientation);
  views::LayoutOrientation GetOrientation() const;

  void SetMainAxisAlignment(views::LayoutAlignment main_axis_alignment);
  views::LayoutAlignment GetMainAxisAlignment() const;

  void SetCrossAxisAlignment(views::LayoutAlignment cross_axis_alignment);
  views::LayoutAlignment GetCrossAxisAlignment() const;

  void SetInsideBorderInsets(const gfx::Insets& insets);
  const gfx::Insets& GetInsideBorderInsets() const;

  void SetMinimumCrossAxisSize(int size);
  int GetMinimumCrossAxisSize() const;

  void SetBetweenChildSpacing(int spacing);
  int GetBetweenChildSpacing() const;

  void SetCollapseMarginsSpacing(bool collapse_margins_spacing);
  bool GetCollapseMarginsSpacing() const;

  void SetDefaultFlex(int default_flex);
  int GetDefaultFlex() const;

  void SetFlexForView(gin::Handle<View> child,
                      int flex,
                      bool use_min_size = false);
  void ClearFlexForView(gin::Handle<View> child);

 protected:
  SuperBoxView();
  ~SuperBoxView() override;
  views::BoxLayoutView* box_layout_view() const {
    return static_cast<views::BoxLayoutView*>(view());
  }
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_SUPER_BOX_VIEW_H_
