// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_VIEWS_HELLO_WORLD_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_VIEWS_HELLO_WORLD_VIEW_H_

#include <string>
#include "gin/handle.h"
#include "shell/browser/api/electron_api_view.h"
#include "ui/views/controls/label.h"

namespace electron::api {

class HelloWorldView : public View {
 public:
  static gin_helper::WrappableBase* New(gin_helper::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void SetText(const std::u16string text);

 protected:
  HelloWorldView();
  ~HelloWorldView() override;
  views::Label* label_view() const {
    return static_cast<views::Label*>(view());
  }
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_VIEWS_HELLO_WORLD_VIEW_H_
