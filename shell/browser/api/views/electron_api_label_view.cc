// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/views/electron_api_label_view.h"

#include <optional>
#include <string_view>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/text_constants.h"

namespace electron::api {

namespace {

std::optional<gfx::HorizontalAlignment> ParseHorizontalAlignment(
    std::string_view alignment) {
  if (alignment == "left")
    return gfx::ALIGN_LEFT;
  if (alignment == "center")
    return gfx::ALIGN_CENTER;
  if (alignment == "right")
    return gfx::ALIGN_RIGHT;
  if (alignment == "start")
    return gfx::ALIGN_TO_HEAD;
  return std::nullopt;
}

std::string HorizontalAlignmentToString(gfx::HorizontalAlignment alignment) {
  switch (alignment) {
    case gfx::ALIGN_LEFT:
      return "left";
    case gfx::ALIGN_CENTER:
      return "center";
    case gfx::ALIGN_RIGHT:
      return "right";
    case gfx::ALIGN_TO_HEAD:
      return "start";
  }
  return "start";
}

std::optional<gfx::VerticalAlignment> ParseVerticalAlignment(
    std::string_view alignment) {
  if (alignment == "top")
    return gfx::ALIGN_TOP;
  if (alignment == "center")
    return gfx::ALIGN_MIDDLE;
  if (alignment == "bottom")
    return gfx::ALIGN_BOTTOM;
  return std::nullopt;
}

std::string VerticalAlignmentToString(gfx::VerticalAlignment alignment) {
  switch (alignment) {
    case gfx::ALIGN_TOP:
      return "top";
    case gfx::ALIGN_MIDDLE:
      return "center";
    case gfx::ALIGN_BOTTOM:
      return "bottom";
  }
  return "center";
}

}  // namespace

LabelView::LabelView() : View(new views::Label()) {}

LabelView::~LabelView() = default;

void LabelView::SetText(const std::string& text) {
  label_view()->SetText(base::UTF8ToUTF16(text));
}

std::string LabelView::GetText() const {
  return base::UTF16ToUTF8(label_view()->GetText());
}

void LabelView::SetFontFamily(const std::string& font_family) {
  const gfx::FontList& current_font_list = label_view()->font_list();
  label_view()->SetFontList(gfx::FontList(
      std::vector<std::string>{font_family}, current_font_list.GetFontStyle(),
      current_font_list.GetFontSize(), current_font_list.GetFontWeight()));
}

std::string LabelView::GetFontFamily() const {
  return label_view()->font_list().GetPrimaryFont().GetFontName();
}

void LabelView::SetFontSize(int font_size) {
  if (font_size < 1)
    font_size = 1;

  const gfx::FontList& current_font_list = label_view()->font_list();
  label_view()->SetFontList(current_font_list.DeriveWithSizeDelta(
      font_size - current_font_list.GetFontSize()));
}

int LabelView::GetFontSize() const {
  return label_view()->font_list().GetFontSize();
}

void LabelView::SetBold(bool bold) {
  label_view()->SetFontList(label_view()->font_list().DeriveWithWeight(
      bold ? gfx::Font::Weight::BOLD : gfx::Font::Weight::NORMAL));
}

bool LabelView::GetBold() const {
  return static_cast<int>(label_view()->font_list().GetFontWeight()) >=
         static_cast<int>(gfx::Font::Weight::BOLD);
}

void LabelView::SetFontWeight(int font_weight) {
  label_view()->SetFontList(label_view()->font_list().DeriveWithWeight(
      gfx::FontWeightFromInt(font_weight)));
}

int LabelView::GetFontWeight() const {
  return static_cast<int>(label_view()->font_list().GetFontWeight());
}

void LabelView::SetTextColor(const std::string& color) {
  std::optional<SkColor> parsed_color = electron::ParseCSSColor(color);
  if (!parsed_color.has_value()) {
    gin_helper::ErrorThrower(isolate()).ThrowError("Invalid color string");
    return;
  }
  label_view()->SetEnabledColor(*parsed_color);
}

std::string LabelView::GetTextColor() const {
  return electron::ToRGBAHex(label_view()->GetEnabledColor());
}

void LabelView::SetHorizontalAlignment(const std::string& alignment) {
  auto parsed = ParseHorizontalAlignment(alignment);
  if (!parsed.has_value()) {
    gin_helper::ErrorThrower(isolate()).ThrowError(
        "horizontalAlignment must be 'left', 'center', 'right', or 'start'");
    return;
  }
  label_view()->SetHorizontalAlignment(*parsed);
}

std::string LabelView::GetHorizontalAlignment() const {
  return HorizontalAlignmentToString(label_view()->GetHorizontalAlignment());
}

void LabelView::SetVerticalAlignment(const std::string& alignment) {
  auto parsed = ParseVerticalAlignment(alignment);
  if (!parsed.has_value()) {
    gin_helper::ErrorThrower(isolate()).ThrowError(
        "verticalAlignment must be 'top', 'center', or 'bottom'");
    return;
  }
  label_view()->SetVerticalAlignment(*parsed);
}

std::string LabelView::GetVerticalAlignment() const {
  return VerticalAlignmentToString(label_view()->GetVerticalAlignment());
}

// static
gin_helper::WrappableBase* LabelView::New(gin::Arguments* args) {
  auto* view = new LabelView();
  view->InitWithArgs(args);
  return view;
}

// static
void LabelView::BuildPrototype(v8::Isolate* isolate,
                               v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "LabelView"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("setText", &LabelView::SetText)
      .SetMethod("getText", &LabelView::GetText)
      .SetMethod("setFontFamily", &LabelView::SetFontFamily)
      .SetMethod("getFontFamily", &LabelView::GetFontFamily)
      .SetMethod("setFontSize", &LabelView::SetFontSize)
      .SetMethod("getFontSize", &LabelView::GetFontSize)
      .SetMethod("setBold", &LabelView::SetBold)
      .SetMethod("getBold", &LabelView::GetBold)
      .SetMethod("setFontWeight", &LabelView::SetFontWeight)
      .SetMethod("getFontWeight", &LabelView::GetFontWeight)
      .SetMethod("setTextColor", &LabelView::SetTextColor)
      .SetMethod("getTextColor", &LabelView::GetTextColor)
      .SetMethod("setHorizontalAlignment", &LabelView::SetHorizontalAlignment)
      .SetMethod("getHorizontalAlignment", &LabelView::GetHorizontalAlignment)
      .SetMethod("setVerticalAlignment", &LabelView::SetVerticalAlignment)
      .SetMethod("getVerticalAlignment", &LabelView::GetVerticalAlignment);
}

}  // namespace electron::api

namespace {

using electron::api::LabelView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.Set("LabelView", gin_helper::CreateConstructor<LabelView>(
                            isolate, base::BindRepeating(&LabelView::New)));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_label_view, Initialize)
