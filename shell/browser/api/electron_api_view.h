// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_VIEW_H_

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "shell/browser/ui/layout_event_forwarding_view.h"
#include "shell/browser/ui/mouse_event_forwarding_view.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_helper/event_emitter.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"
#include "v8/include/v8-value.h"

namespace gin_helper {
template <typename T>
class Handle;
}  // namespace gin_helper

namespace gfx {
class Point;
}  // namespace gfx

namespace ui {
class Layer;
class MouseEvent;
class MouseWheelEvent;
}  // namespace ui

namespace views {
class BorderShadowLayerDelegate;
class SizeBounds;
}  // namespace views

namespace gin {
class Arguments;
}  // namespace gin

namespace electron::api {

class View : public gin_helper::EventEmitter<View>,
             public electron::MouseEventForwardingDelegate,
             public electron::LayoutEventForwardingDelegate,
             private views::ViewObserver {
 public:
  static gin_helper::WrappableBase* New(gin::Arguments* args);
  static gin_helper::Handle<View> Create(v8::Isolate* isolate);

  // Return the cached constructor function.
  static v8::Local<v8::Function> GetConstructor(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void AddChildViewAt(gin_helper::Handle<View> child,
                      std::optional<size_t> index);
  void RemoveChildView(gin_helper::Handle<View> child);

  void SetBounds(const gfx::Rect& bounds, gin::Arguments* args);
  gfx::Rect GetBounds() const;
  gfx::Rect GetLocalBounds() const;
  void SetLayout(v8::Isolate* isolate, v8::Local<v8::Object> value);
  void InvalidateLayout();
  void SchedulePaint();
  void PreferredSizeChanged();
  std::vector<v8::Local<v8::Value>> GetChildren();
  void SetBackgroundColor(std::optional<WrappedSkColor> color);
  void SetBorderRadius(int radius);
  void SetMasksToBounds(bool masks_to_bounds);
  void SetBorder(gin::Arguments* args);
  void SetBackgroundBlur(int blur_radius);
  void SetVisible(bool visible);
  bool GetVisible() const;
  gfx::Size GetPreferredSize() const;
  void SetPreferredSize(std::optional<gfx::Size> size);
  void SizeToContents();
  void SetTranslation(float x, std::optional<float> y);
  void SetScale(float x, std::optional<float> y);
  void SetRotation(float degrees);
  void ResetTransform();
  void SetShadow(gin::Arguments* args);
  void ClearShadow();
  void SetLayerOpacity(float opacity);

  views::View* view() const { return view_; }
  std::optional<int> border_radius() const { return border_radius_; }

  // disable copy
  View(const View&) = delete;
  View& operator=(const View&) = delete;

 protected:
  explicit View(views::View* view);
  View();
  ~View() override;

  // Should delete the |view_| in destructor.
  void set_delete_view(bool should) { delete_view_ = should; }

  ui::Layer* GetLayer();

 private:
  struct BoxShadowOptions {
    SkColor color = SkColorSetARGB(64, 0, 0, 0);
    int offset_x = 0;
    int offset_y = 0;
    int blur_radius = 0;
    int spread_radius = 0;
  };

  using ChildPair = std::pair<raw_ptr<views::View>, v8::Global<v8::Object>>;

  void InitializeView(views::View* view);
  void ApplyBorderRadius();
  void UpdateShadowLayer();
  void ReorderChildView(gin_helper::Handle<View> child, size_t index);

  void EmitEventObject(std::string_view name);
  void EmitMouseEvent(std::string_view name, const ui::MouseEvent& event);
  bool EmitMouseEventAndReturnValue(std::string_view name,
                                    const ui::MouseEvent& event,
                                    bool default_return_value);
  bool EmitMouseWheelEventAndReturnValue(std::string_view name,
                                         const ui::MouseWheelEvent& event);
  bool EmitHitTestPointEventAndReturnValue(std::string_view name,
                                           const gfx::Point& point);

  // electron::MouseEventForwardingDelegate
  bool OnMousePressedFromNative(const ui::MouseEvent& event) override;
  bool OnMouseDraggedFromNative(const ui::MouseEvent& event) override;
  void OnMouseReleasedFromNative(const ui::MouseEvent& event) override;
  void OnMouseCaptureLostFromNative() override;
  void OnMouseMovedFromNative(const ui::MouseEvent& event) override;
  void OnMouseEnteredFromNative(const ui::MouseEvent& event) override;
  void OnMouseExitedFromNative(const ui::MouseEvent& event) override;
  bool OnMouseWheelFromNative(const ui::MouseWheelEvent& event) override;
  bool OnHitTestPointFromNative(const gfx::Point& point) override;

  // electron::LayoutEventForwardingDelegate
  std::optional<gfx::Size> OnCalculatePreferredSizeFromNative(
      const views::SizeBounds& available_size) override;
  void OnLayoutFromNative() override;

  // views::ViewObserver
  void OnViewBoundsChanged(views::View* observed_view) override;
  void OnViewIsDeleting(views::View* observed_view) override;
  void OnChildViewRemoved(views::View* observed_view,
                          views::View* child) override;

  std::vector<ChildPair> child_views_;
  std::optional<int> border_radius_;
  std::optional<BoxShadowOptions> shadow_options_;
  std::unique_ptr<ui::Layer> shadow_layer_;
  std::unique_ptr<views::BorderShadowLayerDelegate> shadow_delegate_;
  int shadow_corner_radius_ = 0;

  bool delete_view_ = true;
  raw_ptr<views::View> view_ = nullptr;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_VIEW_H_
