// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/views/electron_api_box_layout_view.h"

#include <optional>

#include "base/strings/string_util.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace electron::api {

namespace {

std::optional<views::BoxLayout::Orientation> ParseOrientation(
    const std::string& orientation) {
  const std::string normalized = base::ToLowerASCII(orientation);
  if (normalized == "horizontal")
    return views::BoxLayout::Orientation::kHorizontal;
  if (normalized == "vertical")
    return views::BoxLayout::Orientation::kVertical;
  return std::nullopt;
}

std::string OrientationToString(views::BoxLayout::Orientation orientation) {
  switch (orientation) {
    case views::BoxLayout::Orientation::kHorizontal:
      return "horizontal";
    case views::BoxLayout::Orientation::kVertical:
      return "vertical";
  }
  return "horizontal";
}

std::optional<views::BoxLayout::MainAxisAlignment> ParseMainAxisAlignment(
    const std::string& alignment) {
  const std::string normalized = base::ToLowerASCII(alignment);
  if (normalized == "start")
    return views::BoxLayout::MainAxisAlignment::kStart;
  if (normalized == "center")
    return views::BoxLayout::MainAxisAlignment::kCenter;
  if (normalized == "end")
    return views::BoxLayout::MainAxisAlignment::kEnd;
  if (normalized == "stretch")
    return views::BoxLayout::MainAxisAlignment::kStretch;
  if (normalized == "baseline")
    return views::BoxLayout::MainAxisAlignment::kBaseline;
  return std::nullopt;
}

std::string MainAxisAlignmentToString(
    views::BoxLayout::MainAxisAlignment alignment) {
  switch (alignment) {
    case views::BoxLayout::MainAxisAlignment::kStart:
      return "start";
    case views::BoxLayout::MainAxisAlignment::kCenter:
      return "center";
    case views::BoxLayout::MainAxisAlignment::kEnd:
      return "end";
    case views::BoxLayout::MainAxisAlignment::kStretch:
      return "stretch";
    case views::BoxLayout::MainAxisAlignment::kBaseline:
      return "baseline";
  }
  return "start";
}

std::optional<views::BoxLayout::CrossAxisAlignment> ParseCrossAxisAlignment(
    const std::string& alignment) {
  const std::string normalized = base::ToLowerASCII(alignment);
  if (normalized == "start")
    return views::BoxLayout::CrossAxisAlignment::kStart;
  if (normalized == "center")
    return views::BoxLayout::CrossAxisAlignment::kCenter;
  if (normalized == "end")
    return views::BoxLayout::CrossAxisAlignment::kEnd;
  if (normalized == "stretch")
    return views::BoxLayout::CrossAxisAlignment::kStretch;
  if (normalized == "baseline")
    return views::BoxLayout::CrossAxisAlignment::kBaseline;
  return std::nullopt;
}

std::string CrossAxisAlignmentToString(
    views::BoxLayout::CrossAxisAlignment alignment) {
  switch (alignment) {
    case views::BoxLayout::CrossAxisAlignment::kStart:
      return "start";
    case views::BoxLayout::CrossAxisAlignment::kCenter:
      return "center";
    case views::BoxLayout::CrossAxisAlignment::kEnd:
      return "end";
    case views::BoxLayout::CrossAxisAlignment::kStretch:
      return "stretch";
    case views::BoxLayout::CrossAxisAlignment::kBaseline:
      return "baseline";
  }
  return "stretch";
}

}  // namespace

BoxLayoutView::BoxLayoutView() : View(new views::BoxLayoutView()) {}

BoxLayoutView::~BoxLayoutView() = default;

void BoxLayoutView::SetOrientation(const std::string& orientation) {
  auto parsed = ParseOrientation(orientation);
  if (!parsed.has_value()) {
    gin_helper::ErrorThrower(isolate()).ThrowError(
        "orientation must be 'horizontal' or 'vertical'");
    return;
  }
  box_layout_view()->SetOrientation(parsed.value());
}

std::string BoxLayoutView::GetOrientation() const {
  return OrientationToString(box_layout_view()->GetOrientation());
}

void BoxLayoutView::SetMainAxisAlignment(const std::string& alignment) {
  auto parsed = ParseMainAxisAlignment(alignment);
  if (!parsed.has_value()) {
    gin_helper::ErrorThrower(isolate()).ThrowError(
      "mainAxisAlignment must be 'start', 'center', 'end', 'stretch', "
      "or 'baseline'");
    return;
  }
  box_layout_view()->SetMainAxisAlignment(parsed.value());
}

std::string BoxLayoutView::GetMainAxisAlignment() const {
  return MainAxisAlignmentToString(box_layout_view()->GetMainAxisAlignment());
}

void BoxLayoutView::SetCrossAxisAlignment(const std::string& alignment) {
  auto parsed = ParseCrossAxisAlignment(alignment);
  if (!parsed.has_value()) {
    gin_helper::ErrorThrower(isolate()).ThrowError(
      "crossAxisAlignment must be 'start', 'center', 'end', 'stretch', "
      "or 'baseline'");
    return;
  }
  box_layout_view()->SetCrossAxisAlignment(parsed.value());
}

std::string BoxLayoutView::GetCrossAxisAlignment() const {
  return CrossAxisAlignmentToString(box_layout_view()->GetCrossAxisAlignment());
}

void BoxLayoutView::SetInsideBorderInsets(const gfx::Insets& insets) {
  box_layout_view()->SetInsideBorderInsets(insets);
}

gfx::Insets BoxLayoutView::GetInsideBorderInsets() const {
  return box_layout_view()->GetInsideBorderInsets();
}

void BoxLayoutView::SetBetweenChildSpacing(int spacing) {
  box_layout_view()->SetBetweenChildSpacing(spacing);
}

int BoxLayoutView::GetBetweenChildSpacing() const {
  return box_layout_view()->GetBetweenChildSpacing();
}

void BoxLayoutView::SetCollapseMarginsSpacing(bool collapse_margins_spacing) {
  box_layout_view()->SetCollapseMarginsSpacing(collapse_margins_spacing);
}

bool BoxLayoutView::GetCollapseMarginsSpacing() const {
  return box_layout_view()->GetCollapseMarginsSpacing();
}

void BoxLayoutView::SetDefaultFlex(int default_flex) {
  box_layout_view()->SetDefaultFlex(default_flex);
}

int BoxLayoutView::GetDefaultFlex() const {
  return box_layout_view()->GetDefaultFlex();
}

void BoxLayoutView::SetFlexForView(gin_helper::Handle<View> view,
                                   int flex,
                                   gin::Arguments* args) {
  if (view.IsEmpty() || !view->view()) {
    args->ThrowTypeError("Must pass a View instance");
    return;
  }
  bool use_min_size = false;
  args->GetNext(&use_min_size);
  box_layout_view()->SetFlexForView(view->view(), flex, use_min_size);
}

void BoxLayoutView::ClearFlexForView(gin_helper::Handle<View> view) {
  if (view.IsEmpty() || !view->view())
    return;
  box_layout_view()->ClearFlexForView(view->view());
}

// static
gin_helper::WrappableBase* BoxLayoutView::New(gin::Arguments* args) {
  auto* view = new BoxLayoutView();
  view->InitWithArgs(args);
  return view;
}

// static
void BoxLayoutView::BuildPrototype(v8::Isolate* isolate,
                                   v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "BoxLayoutView"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("setOrientation", &BoxLayoutView::SetOrientation)
      .SetMethod("getOrientation", &BoxLayoutView::GetOrientation)
      .SetMethod("setMainAxisAlignment", &BoxLayoutView::SetMainAxisAlignment)
      .SetMethod("getMainAxisAlignment", &BoxLayoutView::GetMainAxisAlignment)
      .SetMethod("setCrossAxisAlignment", &BoxLayoutView::SetCrossAxisAlignment)
      .SetMethod("getCrossAxisAlignment", &BoxLayoutView::GetCrossAxisAlignment)
      .SetMethod("setInsideBorderInsets", &BoxLayoutView::SetInsideBorderInsets)
      .SetMethod("getInsideBorderInsets", &BoxLayoutView::GetInsideBorderInsets)
      .SetMethod("setBetweenChildSpacing",
             &BoxLayoutView::SetBetweenChildSpacing)
      .SetMethod("getBetweenChildSpacing",
             &BoxLayoutView::GetBetweenChildSpacing)
      .SetMethod("setCollapseMarginsSpacing",
             &BoxLayoutView::SetCollapseMarginsSpacing)
      .SetMethod("getCollapseMarginsSpacing",
             &BoxLayoutView::GetCollapseMarginsSpacing)
      .SetMethod("setDefaultFlex", &BoxLayoutView::SetDefaultFlex)
      .SetMethod("getDefaultFlex", &BoxLayoutView::GetDefaultFlex)
      .SetMethod("setFlexForView", &BoxLayoutView::SetFlexForView)
      .SetMethod("clearFlexForView", &BoxLayoutView::ClearFlexForView);
}

}  // namespace electron::api

namespace {

using electron::api::BoxLayoutView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.Set("BoxLayoutView",
           gin_helper::CreateConstructor<BoxLayoutView>(
               isolate, base::BindRepeating(&BoxLayoutView::New)));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_box_layout_view, Initialize)
