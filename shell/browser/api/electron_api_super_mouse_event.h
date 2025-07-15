#ifndef ELECTRON_SHELL_BROWSER_API_SUPER_MOUSE_EVENT_H_
#define ELECTRON_SHELL_BROWSER_API_SUPER_MOUSE_EVENT_H_

#include "gin/handle.h"
#include "shell/common/gin_helper/wrappable.h"
#include "shell/common/gin_helper/wrappable_base.h"
#include "ui/events/event.h"
#include "v8-function.h"

namespace electron::super {
class SuperMouseEvent : public gin_helper::Wrappable<SuperMouseEvent> {
 public:
  static gin_helper::WrappableBase* New(gin::Arguments* args);
  static v8::Local<v8::Function> GetConstructor(v8::Isolate* isolate);
  static gin::Handle<SuperMouseEvent> Create(v8::Isolate* isolate,
                                             const ui::MouseEvent& mouse_event);
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);
  SuperMouseEvent(const SuperMouseEvent&) = delete;
  SuperMouseEvent& operator=(const SuperMouseEvent&) = delete;

  ui::EventType type() const;
  bool IsMouseEvent() const;
  bool IsMouseWheelEvent() const;

  bool IsLeftMouseButton() const;
  bool IsMiddleMouseButton() const;
  bool IsRightMouseButton() const;
  bool IsMouseButtonDown() const;

  const gfx::Point location() const;
  const gfx::Point root_location() const;

  int click_count() const;

  void StopPropagation();
  bool stopped_propagation() const;

  void SetHandled();
  bool handled() const;

 protected:
  explicit SuperMouseEvent(ui::MouseEvent* mouse_event);
  ~SuperMouseEvent() override;
  ui::MouseEvent* mouse_event() const;
  void set_mouse_event(const ui::MouseEvent& mouse_event);

 private:
  ui::MouseEvent* mouse_event_ = nullptr;
};
}  // namespace electron::super

#endif