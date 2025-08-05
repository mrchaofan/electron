#include "shell/browser/api/electron_api_super_layout_view.h"
#include <iostream>
#include "shell/browser/api/super_cr_view.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/layout/layout_types.h"
#include "v8-local-handle.h"
#include "v8-value.h"

namespace electron::super {

class SuperCrLayoutView : public SuperCrView<views::View> {
 public:
  explicit SuperCrLayoutView(SuperCrViewDelegates* viewDelegates,
                             SuperCrLayoutViewDelegates* layoutDelegates)
      : SuperCrView<views::View>(viewDelegates), delegates_(layoutDelegates) {}
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override {
    return delegates_->CalculateCrPreferredSize(available_size);
  }
  gfx::Size GetMinimumSize() const override {
    return delegates_->GetCrMinimumSize();
  }
  gfx::Size GetMaximumSize() const override {
    return delegates_->GetCrMaximumSize();
  }

  void Layout(PassKey) override { delegates_->CrLayout(); }

 private:
  SuperCrLayoutViewDelegates* delegates_;
};
}  // namespace electron::super

namespace electron::api {

SuperLayoutView::SuperLayoutView()
    : View(new electron::super::SuperCrLayoutView(this, this)) {
  view()->set_owned_by_client();
}

SuperLayoutView::~SuperLayoutView() = default;

// static
gin_helper::WrappableBase* SuperLayoutView::New(gin_helper::Arguments* args) {
  // Constructor call.
  auto* view = new SuperLayoutView();
  view->InitWithArgs(args);
  return view;
}

// static
void SuperLayoutView::BuildPrototype(
    v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "SuperLayoutView"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .Build();
}

gfx::Size SuperLayoutView::CalculateCrPreferredSize(
    const views::SizeBounds& available_size) const {
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Object> wrapper = GetWrapper();
  if (wrapper.IsEmpty()) {
    return gfx::Size();
  }
  v8::Local<v8::Function> handler =
      wrapper
          ->Get(isolate()->GetCurrentContext(),
                v8::String::NewFromUtf8(isolate(), "calculatePreferredSize")
                    .ToLocalChecked())
          .ToLocalChecked()
          .As<v8::Function>();
  v8::Local<v8::Value> result;
  if (handler->Call(isolate()->GetCurrentContext(), wrapper, 0, {})
          .ToLocal(&result)) {
    gfx::Size size;
    if (gin::Converter<gfx::Size>::FromV8(isolate(), result, &size)) {
      return size;
    }
  }
  return gfx::Size();
}

gfx::Size SuperLayoutView::GetCrMinimumSize() const {
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Object> wrapper = GetWrapper();
  if (wrapper.IsEmpty())
    return gfx::Size();
  v8::Local<v8::Function> handler =
      wrapper
          ->Get(isolate()->GetCurrentContext(),
                v8::String::NewFromUtf8(isolate(), "getMinimumSize")
                    .ToLocalChecked())
          .ToLocalChecked()
          .As<v8::Function>();
  v8::Local<v8::Value> result;
  if (handler->Call(isolate()->GetCurrentContext(), wrapper, 0, {})
          .ToLocal(&result)) {
    gfx::Size size;
    if (gin::Converter<gfx::Size>::FromV8(isolate(), result, &size)) {
      return size;
    }
  }
  return gfx::Size();
}

gfx::Size SuperLayoutView::GetCrMaximumSize() const {
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Object> wrapper = GetWrapper();
  if (wrapper.IsEmpty())
    return gfx::Size();
  v8::Local<v8::Function> handler =
      wrapper
          ->Get(isolate()->GetCurrentContext(),
                v8::String::NewFromUtf8(isolate(), "getMaximumSize")
                    .ToLocalChecked())
          .ToLocalChecked()
          .As<v8::Function>();
  v8::Local<v8::Value> result;
  if (handler->Call(isolate()->GetCurrentContext(), wrapper, 0, {})
          .ToLocal(&result)) {
    gfx::Size size;
    if (gin::Converter<gfx::Size>::FromV8(isolate(), result, &size)) {
      return size;
    }
  }
  return gfx::Size();
}

void SuperLayoutView::CrLayout() {
  v8::HandleScope handle_scope(isolate());
  v8::Local<v8::Object> wrapper = GetWrapper();
  if (wrapper.IsEmpty())
    return;
  v8::Local<v8::Function> handler =
      wrapper
          ->Get(isolate()->GetCurrentContext(),
                v8::String::NewFromUtf8(isolate(), "layout").ToLocalChecked())
          .ToLocalChecked()
          .As<v8::Function>();
  v8::TryCatch try_catch(isolate());
  handler->Call(isolate()->GetCurrentContext(), wrapper, 0, {}).IsEmpty();
}
}  // namespace electron::api

namespace {

using electron::api::SuperLayoutView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("SuperLayoutView",
           gin_helper::CreateConstructor<SuperLayoutView>(
               isolate, base::BindRepeating(&SuperLayoutView::New)));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_super_layout_view,
                                  Initialize)
