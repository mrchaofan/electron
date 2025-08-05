#ifndef ELECTRON_SHELL_SUPER_CR_VIEW_H_
#define ELECTRON_SHELL_SUPER_CR_VIEW_H_

#include "ui/events/event.h"
#include "ui/views/view.h"

namespace electron::super {
class SuperCrViewDelegates {
 public:
  // 鼠标事件相关
  virtual void OnCrMouseEntered(const ui::MouseEvent& event) = 0;
  virtual void OnCrMouseExited(const ui::MouseEvent& event) = 0;
  virtual void OnCrMouseMoved(const ui::MouseEvent& event) = 0;
  virtual bool OnCrMousePressed(const ui::MouseEvent& event) = 0;
  virtual void OnCrMouseReleased(const ui::MouseEvent& event) = 0;
  virtual bool OnCrMouseDragged(const ui::MouseEvent& event) = 0;
  virtual void OnCrMouseCaptureLost() = 0;
};

}  // namespace electron::super

namespace electron::super {
template <typename T,
          typename = typename std::enable_if<
              std::is_base_of<views::View, T>::value>::type>
class SuperCrView : public T {
 public:
  explicit SuperCrView(SuperCrViewDelegates* delegates)
      : delegates_(delegates) {}

  void OnMouseEntered(const ui::MouseEvent& event) override {
    delegates_->OnCrMouseEntered(event);
  }
  void OnMouseExited(const ui::MouseEvent& event) override {
    delegates_->OnCrMouseExited(event);
  }
  void OnMouseMoved(const ui::MouseEvent& event) override {
    delegates_->OnCrMouseMoved(event);
  }
  bool OnMousePressed(const ui::MouseEvent& event) override {
    return delegates_->OnCrMousePressed(event);
  }
  void OnMouseReleased(const ui::MouseEvent& event) override {
    delegates_->OnCrMouseReleased(event);
  }
  bool OnMouseDragged(const ui::MouseEvent& event) override {
    return delegates_->OnCrMouseDragged(event);
  }
  void OnMouseCaptureLost() override { delegates_->OnCrMouseCaptureLost(); }

 private:
  SuperCrViewDelegates* delegates_;
};
}  // namespace electron::super

#endif