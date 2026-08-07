// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/views/electron_api_scroll_view.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/views/layout/fill_layout.h"

namespace electron::api {

namespace {

// A plain container that always fills itself to its single child's preferred
// size, used to give views::ScrollView ownership of contents without
// requiring the JS-owned View passed to SetContentView() to give up its own
// ownership (which views::ScrollView::SetContents() would otherwise require).
class ContentsContainer : public views::View {
 public:
  ContentsContainer() {
    SetLayoutManager(std::make_unique<views::FillLayout>());
  }
};

std::optional<views::ScrollView::ScrollBarMode> ParseScrollBarMode(
    const std::string& mode) {
  const std::string normalized = base::ToLowerASCII(mode);
  if (normalized == "disabled")
    return views::ScrollView::ScrollBarMode::kDisabled;
  if (normalized == "hidden")
    return views::ScrollView::ScrollBarMode::kHiddenButEnabled;
  if (normalized == "enabled")
    return views::ScrollView::ScrollBarMode::kEnabled;
  return std::nullopt;
}

std::string ScrollBarModeToString(views::ScrollView::ScrollBarMode mode) {
  switch (mode) {
    case views::ScrollView::ScrollBarMode::kDisabled:
      return "disabled";
    case views::ScrollView::ScrollBarMode::kHiddenButEnabled:
      return "hidden";
    case views::ScrollView::ScrollBarMode::kEnabled:
      return "enabled";
  }
  return "enabled";
}

}  // namespace

ScrollView::ScrollView() : View(new views::ScrollView()) {
  on_contents_scrolled_subscription_ =
      scroll_view()->AddContentsScrolledCallback(base::BindRepeating(
          &ScrollView::OnContentsScrolled, base::Unretained(this)));
}

ScrollView::~ScrollView() = default;

void ScrollView::SetContentView(gin_helper::Handle<View> view) {
  if (view.IsEmpty() || !view->view()) {
    gin_helper::ErrorThrower(isolate()).ThrowError(
        "Must pass a View instance");
    return;
  }

  auto container = std::make_unique<ContentsContainer>();
  container->AddChildViewAt(view->view(), 0);
  scroll_view()->SetContents(std::move(container));

  content_view_.Reset(isolate(), view->GetWrapper());
}

v8::Local<v8::Value> ScrollView::GetContentView(v8::Isolate* isolate) {
  if (content_view_.IsEmpty())
    return v8::Null(isolate);
  return content_view_.Get(isolate);
}

gfx::Rect ScrollView::GetVisibleRect() const {
  return scroll_view()->GetVisibleRect();
}

gfx::PointF ScrollView::GetScrollOffset() const {
  return scroll_view()->CurrentOffset();
}

void ScrollView::ScrollToOffset(int x, int y) {
  scroll_view()->ScrollToOffset(gfx::PointF(x, y));
}

void ScrollView::ScrollByOffset(int x, int y) {
  scroll_view()->ScrollByOffset(gfx::PointF(x, y));
}

void ScrollView::ScrollToView(gin_helper::Handle<View> view) {
  if (view.IsEmpty() || !view->view())
    return;
  view->view()->ScrollRectToVisible(view->view()->GetLocalBounds());
}

void ScrollView::OnContentsScrolled() {
  EmitWithoutEvent("scroll");
}

void ScrollView::SetHorizontalScrollBarMode(const std::string& mode) {
  auto parsed = ParseScrollBarMode(mode);
  if (!parsed.has_value()) {
    gin_helper::ErrorThrower(isolate()).ThrowError(
        "mode must be 'disabled', 'hidden', or 'enabled'");
    return;
  }
  scroll_view()->SetHorizontalScrollBarMode(parsed.value());
}

std::string ScrollView::GetHorizontalScrollBarMode() const {
  return ScrollBarModeToString(scroll_view()->GetHorizontalScrollBarMode());
}

void ScrollView::SetVerticalScrollBarMode(const std::string& mode) {
  auto parsed = ParseScrollBarMode(mode);
  if (!parsed.has_value()) {
    gin_helper::ErrorThrower(isolate()).ThrowError(
        "mode must be 'disabled', 'hidden', or 'enabled'");
    return;
  }
  scroll_view()->SetVerticalScrollBarMode(parsed.value());
}

std::string ScrollView::GetVerticalScrollBarMode() const {
  return ScrollBarModeToString(scroll_view()->GetVerticalScrollBarMode());
}

// static
gin_helper::WrappableBase* ScrollView::New(gin::Arguments* args) {
  auto* view = new ScrollView();
  view->InitWithArgs(args);
  return view;
}

// static
void ScrollView::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "ScrollView"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("setContentView", &ScrollView::SetContentView)
      .SetMethod("getContentView", &ScrollView::GetContentView)
      .SetMethod("getVisibleRect", &ScrollView::GetVisibleRect)
      .SetMethod("getScrollOffset", &ScrollView::GetScrollOffset)
      .SetMethod("scrollToOffset", &ScrollView::ScrollToOffset)
      .SetMethod("scrollByOffset", &ScrollView::ScrollByOffset)
      .SetMethod("scrollToView", &ScrollView::ScrollToView)
      .SetMethod("setHorizontalScrollBarMode",
                 &ScrollView::SetHorizontalScrollBarMode)
      .SetMethod("getHorizontalScrollBarMode",
                 &ScrollView::GetHorizontalScrollBarMode)
      .SetMethod("setVerticalScrollBarMode",
                 &ScrollView::SetVerticalScrollBarMode)
      .SetMethod("getVerticalScrollBarMode",
                 &ScrollView::GetVerticalScrollBarMode);
}

}  // namespace electron::api

namespace {

using electron::api::ScrollView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.Set("ScrollView",
           gin_helper::CreateConstructor<ScrollView>(
               isolate, base::BindRepeating(&ScrollView::New)));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_scroll_view, Initialize)
