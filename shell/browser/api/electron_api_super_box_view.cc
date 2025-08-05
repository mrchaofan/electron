// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_super_box_view.h"
#include <string>

#include "shell/browser/api/super_cr_view.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/layout/layout_types.h"
#include "v8-local-handle.h"
#include "v8-primitive.h"

namespace gin {

template <>
struct Converter<views::LayoutOrientation> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::LayoutOrientation* out) {
    std::string orientation = base::ToLowerASCII(gin::V8ToString(isolate, val));
    if (orientation == "horizontal") {
      *out = views::LayoutOrientation::kHorizontal;
    } else if (orientation == "vertical") {
      *out = views::LayoutOrientation::kVertical;
    } else {
      return false;
    }
    return true;
  }

  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const views::LayoutOrientation& in) {
    std::string layout_name;
    switch (in) {
      case views::LayoutOrientation::kHorizontal:
        return StringToV8(isolate, "horizontal");
      case views::LayoutOrientation::kVertical:
        return StringToV8(isolate, "vertical");
    }
  }
};

template <>
struct Converter<views::LayoutAlignment> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::LayoutAlignment* out) {
    std::string orientation = base::ToLowerASCII(gin::V8ToString(isolate, val));
    if (orientation == "start") {
      *out = views::LayoutAlignment::kStart;
    } else if (orientation == "center") {
      *out = views::LayoutAlignment::kCenter;
    } else if (orientation == "end") {
      *out = views::LayoutAlignment::kEnd;
    } else if (orientation == "stretch") {
      *out = views::LayoutAlignment::kStretch;
    } else if (orientation == "baseline") {
      *out = views::LayoutAlignment::kBaseline;
    } else {
      return false;
    }
    return true;
  }

  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const views::LayoutAlignment& in) {
    std::string layout_name;
    switch (in) {
      case views::LayoutAlignment::kBaseline:
        return StringToV8(isolate, "baseline");
      case views::LayoutAlignment::kCenter:
        return StringToV8(isolate, "center");
      case views::LayoutAlignment::kEnd:
        return StringToV8(isolate, "end");
      case views::LayoutAlignment::kStart:
        return StringToV8(isolate, "start");
      case views::LayoutAlignment::kStretch:
        return StringToV8(isolate, "stretch");
    }
  }
};
}  // namespace gin

namespace electron::api {

SuperBoxView::SuperBoxView()
    : View(new super::SuperCrView<views::BoxLayoutView>(this)) {
  view()->set_owned_by_client();
}

SuperBoxView::~SuperBoxView() = default;

// static
gin_helper::WrappableBase* SuperBoxView::New(gin_helper::Arguments* args) {
  // Constructor call.
  auto* view = new SuperBoxView();
  view->InitWithArgs(args);
  return view;
}

// static
void SuperBoxView::BuildPrototype(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "SuperBoxView"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("setOrientation", &SuperBoxView::SetOrientation)
      .SetMethod("getOrientation", &SuperBoxView::GetOrientation)
      .SetMethod("setMainAxisAlignment", &SuperBoxView::SetMainAxisAlignment)
      .SetMethod("getMainAxisAlignment", &SuperBoxView::GetMainAxisAlignment)
      .SetMethod("setCrossAxisAlignment", &SuperBoxView::SetCrossAxisAlignment)
      .SetMethod("getCrossAxisAlignment", &SuperBoxView::GetCrossAxisAlignment)
      .SetMethod("setInsideBorderInsets", &SuperBoxView::SetInsideBorderInsets)
      .SetMethod("getInsideBorderInsets", &SuperBoxView::GetInsideBorderInsets)
      .SetMethod("setMinimumCrossAxisSize",
                 &SuperBoxView::SetMinimumCrossAxisSize)
      .SetMethod("getMinimumCrossAxisSize",
                 &SuperBoxView::GetMinimumCrossAxisSize)
      .SetMethod("setBetweenChildSpacing",
                 &SuperBoxView::SetBetweenChildSpacing)
      .SetMethod("setCollapseMarginsSpacing",
                 &SuperBoxView::SetCollapseMarginsSpacing)
      .SetMethod("getCollapseMarginsSpacing",
                 &SuperBoxView::GetCollapseMarginsSpacing)
      .SetMethod("setDefaultFlex", &SuperBoxView::SetDefaultFlex)
      .SetMethod("getDefaultFlex", &SuperBoxView::GetDefaultFlex)
      .SetMethod("setFlexForView", &SuperBoxView::SetFlexForView)
      .SetMethod("clearFlexForView", &SuperBoxView::ClearFlexForView);
}

void SuperBoxView::SetOrientation(views::LayoutOrientation orientation) {
  box_layout_view()->SetOrientation(orientation);
}
views::LayoutOrientation SuperBoxView::GetOrientation() const {
  return box_layout_view()->GetOrientation();
}

void SuperBoxView::SetMainAxisAlignment(
    views::LayoutAlignment main_axis_alignment) {
  box_layout_view()->SetMainAxisAlignment(main_axis_alignment);
}
views::LayoutAlignment SuperBoxView::GetMainAxisAlignment() const {
  return box_layout_view()->GetMainAxisAlignment();
}

void SuperBoxView::SetCrossAxisAlignment(
    views::LayoutAlignment cross_axis_alignment) {
  box_layout_view()->SetCrossAxisAlignment(cross_axis_alignment);
}
views::LayoutAlignment SuperBoxView::GetCrossAxisAlignment() const {
  return box_layout_view()->GetCrossAxisAlignment();
}

void SuperBoxView::SetInsideBorderInsets(const gfx::Insets& insets) {
  box_layout_view()->SetInsideBorderInsets(insets);
}
const gfx::Insets& SuperBoxView::GetInsideBorderInsets() const {
  return box_layout_view()->GetInsideBorderInsets();
}

void SuperBoxView::SetMinimumCrossAxisSize(int size) {
  box_layout_view()->SetMinimumCrossAxisSize(size);
}
int SuperBoxView::GetMinimumCrossAxisSize() const {
  return box_layout_view()->GetMinimumCrossAxisSize();
}

void SuperBoxView::SetBetweenChildSpacing(int spacing) {
  box_layout_view()->SetBetweenChildSpacing(spacing);
}
int SuperBoxView::GetBetweenChildSpacing() const {
  return box_layout_view()->GetBetweenChildSpacing();
}

void SuperBoxView::SetCollapseMarginsSpacing(bool collapse_margins_spacing) {
  box_layout_view()->SetCollapseMarginsSpacing(collapse_margins_spacing);
}
bool SuperBoxView::GetCollapseMarginsSpacing() const {
  return box_layout_view()->GetCollapseMarginsSpacing();
}

void SuperBoxView::SetDefaultFlex(int default_flex) {
  box_layout_view()->SetDefaultFlex(default_flex);
}
int SuperBoxView::GetDefaultFlex() const {
  return box_layout_view()->GetDefaultFlex();
}

void SuperBoxView::SetFlexForView(gin::Handle<View> child,
                                  int flex,
                                  bool use_min_size) {
  box_layout_view()->SetFlexForView(child->view(), flex, use_min_size);
}
void SuperBoxView::ClearFlexForView(gin::Handle<View> child) {
  box_layout_view()->ClearFlexForView(child->view());
}

}  // namespace electron::api

namespace {

using electron::api::SuperBoxView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("SuperBoxView",
           gin_helper::CreateConstructor<SuperBoxView>(
               isolate, base::BindRepeating(&SuperBoxView::New)));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_super_box_view, Initialize)
