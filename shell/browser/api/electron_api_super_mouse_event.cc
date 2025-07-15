#include "shell/browser/api/electron_api_super_mouse_event.h"
#include "base/no_destructor.h"
#include "gin/handle.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/wrappable_base.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/types/event_type.h"
#include "v8-local-handle.h"

namespace electron::super {
v8::Local<v8::Function> SuperMouseEvent::GetConstructor(v8::Isolate* isolate) {
  static base::NoDestructor<v8::Global<v8::Function>> constructor;
  if (constructor.get()->IsEmpty()) {
    constructor->Reset(
        isolate, gin_helper::CreateConstructor<SuperMouseEvent>(
                     isolate, base::BindRepeating(&SuperMouseEvent::New)));
  }
  return v8::Local<v8::Function>::New(isolate, *constructor.get());
}

gin_helper::WrappableBase* SuperMouseEvent::New(gin::Arguments* args) {
  gfx::Point location(0, 0);
  gfx::Point root_location(0, 0);
  base::TimeTicks timestamp = base::TimeTicks::Now();

  SuperMouseEvent* mouse_event = new SuperMouseEvent(
      new ui::MouseEvent(ui::EventType::kUnknown, location, root_location,
                         timestamp, ui::EF_NONE, ui::EF_NONE));
  mouse_event->InitWithArgs(args);
  return mouse_event;
}

gin::Handle<SuperMouseEvent> SuperMouseEvent::Create(
    v8::Isolate* isolate,
    const ui::MouseEvent& mouse_event) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Object> obj;
  if (GetConstructor(isolate)->NewInstance(context, 0, nullptr).ToLocal(&obj)) {
    gin::Handle<SuperMouseEvent> super_mouse_event;
    if (gin::ConvertFromV8(isolate, obj, &super_mouse_event))
      super_mouse_event->set_mouse_event(mouse_event);
    return super_mouse_event;
  }
  return gin::Handle<SuperMouseEvent>();
}

void SuperMouseEvent::BuildPrototype(
    v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "SuperMouseEvent"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("isMouseEvent", &SuperMouseEvent::IsMouseEvent)
      .SetMethod("isMouseWheelEvent", &SuperMouseEvent::IsMouseWheelEvent)
      .SetMethod("isLeftMouseButton", &SuperMouseEvent::IsLeftMouseButton)
      .SetMethod("isMiddleMouseButton", &SuperMouseEvent::IsMiddleMouseButton)
      .SetMethod("isRightMouseButton", &SuperMouseEvent::IsRightMouseButton)
      .SetMethod("isLeftMouseButton", &SuperMouseEvent::IsLeftMouseButton)
      .SetMethod("location", &SuperMouseEvent::location)
      .SetMethod("rootLocation", &SuperMouseEvent::root_location)
      .SetMethod("clickCount", &SuperMouseEvent::click_count)
      .SetMethod("stopPropagation", &SuperMouseEvent::StopPropagation)
      .SetMethod("stoppedPropagation", &SuperMouseEvent::stopped_propagation)
      .SetMethod("setHandled", &SuperMouseEvent::SetHandled)
      .SetMethod("handled", &SuperMouseEvent::handled)
      .SetMethod("setLocation", &SuperMouseEvent::set_location)
      .SetMethod("setRootLocation", &SuperMouseEvent::set_root_location);
}

SuperMouseEvent::SuperMouseEvent(ui::MouseEvent* mouse_event)
    : mouse_event_(mouse_event) {}

SuperMouseEvent::~SuperMouseEvent() {
  delete mouse_event_;
}

void SuperMouseEvent::set_mouse_event(const ui::MouseEvent& mouse_event) {
  *mouse_event_ = mouse_event;
}

ui::EventType SuperMouseEvent::type() const {
  return mouse_event()->type();
}

bool SuperMouseEvent::IsMouseEvent() const {
  return mouse_event()->IsMouseEvent();
}

bool SuperMouseEvent::IsMouseWheelEvent() const {
  return mouse_event()->IsMouseWheelEvent();
}

bool SuperMouseEvent::IsLeftMouseButton() const {
  return mouse_event()->IsLeftMouseButton();
}
bool SuperMouseEvent::IsMiddleMouseButton() const {
  return mouse_event()->IsMiddleMouseButton();
}
bool SuperMouseEvent::IsRightMouseButton() const {
  return mouse_event()->IsRightMouseButton();
}

const gfx::Point SuperMouseEvent::location() const {
  return mouse_event()->location();
}
const gfx::Point SuperMouseEvent::root_location() const {
  return mouse_event()->root_location();
}

int SuperMouseEvent::click_count() const {
  return mouse_event()->GetClickCount();
}

void SuperMouseEvent::StopPropagation() {
  mouse_event()->StopPropagation();
}
bool SuperMouseEvent::stopped_propagation() const {
  return mouse_event()->stopped_propagation();
}

void SuperMouseEvent::SetHandled() {
  mouse_event()->SetHandled();
}
bool SuperMouseEvent::handled() const {
  return mouse_event()->handled();
}

ui::MouseEvent* SuperMouseEvent::mouse_event() const {
  return mouse_event_;
}

void SuperMouseEvent::set_location(const gfx::Point& location) {
  mouse_event()->set_location(location);
}
void SuperMouseEvent::set_root_location(const gfx::Point& root_location) {
  mouse_event()->set_root_location(root_location);
}

}  // namespace electron::super