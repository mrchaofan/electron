// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_VIEWS_ELECTRON_API_SCROLL_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_VIEWS_ELECTRON_API_SCROLL_VIEW_H_

#include <string>

#include "base/callback_list.h"
#include "shell/browser/api/electron_api_view.h"
#include "ui/views/controls/scroll_view.h"
#include "v8/include/v8-forward.h"

namespace gfx {
class PointF;
class Rect;
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

class ScrollView : public View {
 public:
  static gin_helper::WrappableBase* New(gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void SetContentView(gin_helper::Handle<View> view);
  v8::Local<v8::Value> GetContentView(v8::Isolate* isolate);

  gfx::Rect GetVisibleRect() const;
  gfx::PointF GetScrollOffset() const;
  void ScrollToOffset(int x, int y);
  void ScrollByOffset(int x, int y);
  void ScrollToView(gin_helper::Handle<View> view);

  void SetHorizontalScrollBarMode(const std::string& mode);
  std::string GetHorizontalScrollBarMode() const;
  void SetVerticalScrollBarMode(const std::string& mode);
  std::string GetVerticalScrollBarMode() const;

 protected:
  ScrollView();
  ~ScrollView() override;

  views::ScrollView* scroll_view() const {
    return static_cast<views::ScrollView*>(view());
  }

 private:
  void OnContentsScrolled();

  // Keeps the JS content view alive for as long as it is scrollable content.
  v8::Global<v8::Object> content_view_;
  base::CallbackListSubscription on_contents_scrolled_subscription_;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_VIEWS_ELECTRON_API_SCROLL_VIEW_H_
