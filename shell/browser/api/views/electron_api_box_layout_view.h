// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_VIEWS_ELECTRON_API_BOX_LAYOUT_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_VIEWS_ELECTRON_API_BOX_LAYOUT_VIEW_H_

#include <string>

#include "shell/browser/api/electron_api_view.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/box_layout_view.h"

namespace gfx {
class Insets;
}  // namespace gfx

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
class WrappableBase;
template <typename T>
class Handle;
}  // namespace gin_helper

namespace electron::api {

class BoxLayoutView : public View {
 public:
  static gin_helper::WrappableBase* New(gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void SetOrientation(const std::string& orientation);
  std::string GetOrientation() const;

  void SetMainAxisAlignment(const std::string& alignment);
  std::string GetMainAxisAlignment() const;

  void SetCrossAxisAlignment(const std::string& alignment);
  std::string GetCrossAxisAlignment() const;

  void SetInsideBorderInsets(const gfx::Insets& insets);
  gfx::Insets GetInsideBorderInsets() const;

  void SetBetweenChildSpacing(int spacing);
  int GetBetweenChildSpacing() const;

  void SetCollapseMarginsSpacing(bool collapse_margins_spacing);
  bool GetCollapseMarginsSpacing() const;

  void SetDefaultFlex(int default_flex);
  int GetDefaultFlex() const;

  void SetFlexForView(gin_helper::Handle<View> view,
                      int flex,
                      gin::Arguments* args);
  void ClearFlexForView(gin_helper::Handle<View> view);

 protected:
  BoxLayoutView();
  ~BoxLayoutView() override;

  views::BoxLayoutView* box_layout_view() const {
    return static_cast<views::BoxLayoutView*>(view());
  }
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_VIEWS_ELECTRON_API_BOX_LAYOUT_VIEW_H_
