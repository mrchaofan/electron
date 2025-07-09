#ifndef ELECTRON_SHELL_BROWSER_API_SUPER_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_SUPER_VIEW_H_

#include "shell/browser/api/electron_api_view.h"
#include "shell/common/gin_helper/wrappable_base.h"

namespace electron::super {
class SuperCrViewDelegates {
 public:
  friend class SuperCrView;

 protected:
  // 鼠标事件相关
  virtual void OnMouseEntered(const ui::MouseEvent& event) = 0;
  virtual void OnMouseExited(const ui::MouseEvent& event) = 0;
  virtual void OnMouseMoved(const ui::MouseEvent& event) = 0;
  virtual bool OnMousePressed(const ui::MouseEvent& event) = 0;
  virtual void OnMouseReleased(const ui::MouseEvent& event) = 0;
  virtual bool OnMouseDragged(const ui::MouseEvent& event) = 0;
  virtual void OnMouseCaptureLost() = 0;
};

}  // namespace electron::super

namespace electron::api {
class SuperView : public View, super::SuperCrViewDelegates {
 public:
  static gin_helper::WrappableBase* New(gin_helper::Arguments* args);
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);
  SuperView(const SuperView&) = delete;
  SuperView& operator=(const SuperView&) = delete;

 protected:
  SuperView();
  ~SuperView() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnMouseMoved(const ui::MouseEvent& event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseCaptureLost() override;
};
}  // namespace electron::api

#endif
