#include "shell/browser/api/electron_api_super_view.h"
#include "base/logging.h"
#include "shell/browser/api/electron_api_view.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "ui/views/layout/layout_manager.h"

namespace electron::super {
class SuperCrView : public views::View {
 public:
  explicit SuperCrView(SuperCrViewDelegates* delegates)
      : delegates_(delegates) {}

  void OnMouseEntered(const ui::MouseEvent& event) override {
    delegates_->OnMouseEntered(event);
  }
  void OnMouseExited(const ui::MouseEvent& event) override {
    delegates_->OnMouseExited(event);
  }
  void OnMouseMoved(const ui::MouseEvent& event) override {
    delegates_->OnMouseMoved(event);
  }
  bool OnMousePressed(const ui::MouseEvent& event) override {
    return delegates_->OnMousePressed(event);
  }
  void OnMouseReleased(const ui::MouseEvent& event) override {
    return delegates_->OnMouseReleased(event);
  }
  bool OnMouseDragged(const ui::MouseEvent& event) override {
    return delegates_->OnMouseDragged(event);
  }
  void OnMouseCaptureLost() override {
    return delegates_->OnMouseCaptureLost();
  }

 private:
  SuperCrViewDelegates* delegates_;
};
}  // namespace electron::super

namespace electron::api {
SuperView::SuperView() : View(new super::SuperCrView(this)) {
  view()->set_owned_by_client();
}

SuperView::~SuperView() = default;

// static
gin_helper::WrappableBase* SuperView::New(gin_helper::Arguments* args) {
  // Constructor call.
  auto* view = new SuperView();
  view->InitWithArgs(args);
  return view;
}

// static
void SuperView::BuildPrototype(v8::Isolate* isolate,
                               v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "SuperView"));
}

void SuperView::OnMouseEntered(const ui::MouseEvent& event) {
  LOG(INFO) << "OnMouseEntered";
}
void SuperView::OnMouseExited(const ui::MouseEvent& event) {
  LOG(INFO) << "OnMouseExited";
}
void SuperView::OnMouseMoved(const ui::MouseEvent& event) {
  LOG(INFO) << "OnMouseMoved";
}
bool SuperView::OnMousePressed(const ui::MouseEvent& event) {
  LOG(INFO) << "OnMousePressed";
  return true;
}
void SuperView::OnMouseReleased(const ui::MouseEvent& event) {
  LOG(INFO) << "OnMouseReleased";
}
bool SuperView::OnMouseDragged(const ui::MouseEvent& event) {
  LOG(INFO) << "OnMouseDragged";
  return false;
}
void SuperView::OnMouseCaptureLost() {
  LOG(INFO) << "OnMouseCaptureLost";
}

}  // namespace electron::api

namespace {

using electron::api::SuperView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("SuperView", gin_helper::CreateConstructor<SuperView>(
                            isolate, base::BindRepeating(&SuperView::New)));
}
}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_super_view, Initialize)
