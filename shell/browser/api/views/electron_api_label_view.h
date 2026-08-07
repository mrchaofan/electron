// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_VIEWS_ELECTRON_API_LABEL_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_VIEWS_ELECTRON_API_LABEL_VIEW_H_

#include <string>

#include "shell/browser/api/electron_api_view.h"
#include "ui/views/controls/label.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
class WrappableBase;
}  // namespace gin_helper

namespace electron::api {

class LabelView : public View {
 public:
  static gin_helper::WrappableBase* New(gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void SetText(const std::string& text);
  std::string GetText() const;
  void SetFontFamily(const std::string& font_family);
  std::string GetFontFamily() const;
  void SetFontSize(int font_size);
  int GetFontSize() const;
  void SetBold(bool bold);
  bool GetBold() const;
  void SetFontWeight(int font_weight);
  int GetFontWeight() const;
  void SetTextColor(const std::string& color);
  std::string GetTextColor() const;
  void SetHorizontalAlignment(const std::string& alignment);
  std::string GetHorizontalAlignment() const;
  void SetVerticalAlignment(const std::string& alignment);
  std::string GetVerticalAlignment() const;

 protected:
  LabelView();
  ~LabelView() override;

  views::Label* label_view() const {
    return static_cast<views::Label*>(view());
  }
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_VIEWS_ELECTRON_API_LABEL_VIEW_H_
