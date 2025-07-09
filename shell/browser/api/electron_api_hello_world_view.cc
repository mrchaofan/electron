// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_hello_world_view.h"
#include <string>

#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace gin {

// template <>
// struct Converter<std::u16string> {
//   static bool FromV8(v8::Isolate* isolate,
//                      v8::Local<v8::Value> val,
//                      std::u16string* out) {
//     if (val.IsEmpty() || !val->IsString()) {
//       *out = base::string16();  // 返回空字符串或处理错误
//     } else {
//       *out = gin::V8StringToUTF16(isolate, val.As<v8::String>());
//     }
//     return true;
//   }
// };

// template <>
// struct Converter<views::SizeBounds> {
//   static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
//                                    const views::SizeBounds& in) {
//     return gin::DataObjectBuilder(isolate)
//         .Set("width", in.width())
//         .Set("height", in.height())
//         .Build();
//   }
// };
}  // namespace gin

namespace electron::api {

HelloWorldView::HelloWorldView() : View(new views::Label()) {
  view()->set_owned_by_client();
}

void HelloWorldView::SetText(const std::u16string text) {
  label_view()->SetText(text);
}

HelloWorldView::~HelloWorldView() = default;

// static
gin_helper::WrappableBase* HelloWorldView::New(gin_helper::Arguments* args) {
  // Constructor call.
  auto* view = new HelloWorldView();
  view->InitWithArgs(args);
  return view;
}

// static
void HelloWorldView::BuildPrototype(v8::Isolate* isolate,
                                    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "HelloWorldView"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("setText", &HelloWorldView::SetText);
}

}  // namespace electron::api

namespace {

using electron::api::HelloWorldView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("HelloWorldView",
           gin_helper::CreateConstructor<HelloWorldView>(
               isolate, base::BindRepeating(&HelloWorldView::New)));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_hello_world_view, Initialize)
